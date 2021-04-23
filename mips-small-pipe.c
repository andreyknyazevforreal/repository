#include "mips-small-pipe.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define bool _Bool


void helpID(Pstate state, int instruction, int extraCycles, state_t new);
int OPCheck(int instruction, Pstate state);

/************************************************************/
int main(int argc, char *argv[]) {
  short i;
  char line[MAXLINELENGTH];
  state_t state;
  FILE *filePtr;

  if (argc != 2) {
    printf("error: usage: %s <machine-code file>\n", argv[0]);
    return 1;
  }

  memset(&state, 0, sizeof(state_t));

  state.pc = state.cycles = 0;
  state.IFID.instr = state.IDEX.instr = state.EXMEM.instr = state.MEMWB.instr =
      state.WBEND.instr = NOPINSTRUCTION; /* nop */

  /* read machine-code file into instruction/data memory (starting at address 0)
   */

  filePtr = fopen(argv[1], "r");
  if (filePtr == NULL) {
    printf("error: can't open file %s\n", argv[1]);
    perror("fopen");
    exit(1);
  }

  for (state.numMemory = 0; fgets(line, MAXLINELENGTH, filePtr) != NULL;
       state.numMemory++) {
    if (sscanf(line, "%x", &state.dataMem[state.numMemory]) != 1) {
      printf("error in reading address %d\n", state.numMemory);
      exit(1);
    }
    state.instrMem[state.numMemory] = state.dataMem[state.numMemory];
    printf("memory[%d]=%x\n", state.numMemory, state.dataMem[state.numMemory]);
  }

  printf("%d memory words\n", state.numMemory);

  printf("\tinstruction memory:\n");
  for (i = 0; i < state.numMemory; i++) {
    printf("\t\tinstrMem[ %d ] = ", i);
    printInstruction(state.instrMem[i]);
  }

  run(&state);

  return 0;
}


//helper for ID stage
void helpID(Pstate state, int instruction, int extraCycles, state_t new) {
  new.IDEX.instr = NOPINSTRUCTION;
	new.IDEX.pcPlus1 = 0;
	new.IDEX.readRegA = new.IDEX.readRegB = 0;
	new.IDEX.offset = offset(NOPINSTRUCTION);

	new.IFID.instr = state->IFID.instr;
	instruction = new.IFID.instr;
	new.pc -= 4;
	new.IFID.pcPlus1 = new.pc;

	extraCycles += 1;
}

//checker for ID stage
//1 is true, 0 is false
int OPCheck(int instruction, Pstate state){

  if (((opcode(instruction) == REG_REG_OP) &&
	(opcode(state->IDEX.instr) == LW_OP))) {

    if((field_r1(instruction) == field_r2(state->IDEX.instr)) ||
	  (field_r2(instruction) == field_r2(state->IDEX.instr))){

      return 1;
    }
    return 0;

    }

  else if((opcode(instruction) != HALT_OP) && ((opcode(state->IDEX.instr) == LW_OP))){
	     if((field_r1(instruction) == field_r2(state->IDEX.instr))){
         return 1;
       }
       return 0;
  }
  else return 0;
}


void executeFunction(int instruction, state_t new, int funct, int readRegA, int readRegB){

  if (instruction == NOPINSTRUCTION) {
	new.EXMEM.aluResult = 0;
	new.EXMEM.readRegB = 0;
      }

  switch (funct)
  {
  case ADD_FUNC:
    new.EXMEM.aluResult = readRegA + readRegB;
	  new.EXMEM.readRegB =  readRegB;
    break;

  case SLL_FUNC:
    new.EXMEM.aluResult = readRegA << readRegB;
	  new.EXMEM.readRegB = readRegB;
    break;

  case SRL_FUNC:
    new.EXMEM.aluResult = ((unsigned int) readRegA) >> readRegB;
	  new.EXMEM.readRegB = readRegB;
    break;

  case SUB_FUNC:
    new.EXMEM.aluResult = readRegA - readRegB;
	  new.EXMEM.readRegB = readRegB;
    break;

  case AND_FUNC:
    new.EXMEM.aluResult = readRegA & readRegB;
	  new.EXMEM.readRegB = readRegB;
    break;

  case OR_FUNC:
    new.EXMEM.aluResult = readRegA | readRegA;
	  new.EXMEM.readRegB = readRegB;
    break;
  
  default:
    break;
  }
  

}

  /*
  lw   rd      rs1     imm     Reg[rd] <- Mem[Reg[rs1] + imm]
  sw   rd      rs1     imm     Reg[rd] -> Mem[Reg[rs1] + imm]
  beqz rd      rs1     imm     if (Reg[rs1] == 0) PC <- PC+4+imm
  addi rd      rs1     imm     Reg[rd] <- Reg[rs1] + imm
  add  rd      rs1     rs2     Reg[rd] <- Reg[rs1] + Reg[rs2]
  sub  rd      rs1     rs2     Reg[rd] <- Reg[rs1] - Reg[rs2]
  sll  rd      rs1     rs2     Reg[rd] <- Reg[rs1] << Reg[rs2]
  srl  rd      rs1     rs2     Reg[rd] <- Reg[rs1] >> Reg[rs2]
  and  rd      rs1     rs2     Reg[rd] <- Reg[rs1] & Reg[rs2]
  or   rd      rs1     rs2     Reg[rd] <- Reg[rs1] | Reg[rs2]
  halt                         stop simulation

  */

/************************************************************/

/************************************************************/
void run(Pstate state) {

  //this is the instruction
  int instruction;
  //offset
  int ofst;

  //new stuff below
  int readRegA;
  int readRegB;

  int funct;
  int s1;
  int s2;
  int prev1, prev2, prev3;
  int opPrev1, opPrev2, opPrev3;
  //extra cycles
  int extraCycles = 0;

  int newVal;

  state_t new;
  memset(&new, 0, sizeof(state_t));

  while (1) {

    printState(state);

    /* copy everything so all we have to do is make changes.
       (this is primarily for the memory and reg arrays) */
    memcpy(&new, state, sizeof(state_t));

    new.cycles++;

    //stage should be completed
    /* --------------------- IF stage --------------------- */

    new.IFID.instr = new.instrMem[new.pc/4];
    instruction = new.IFID.instr;
    new.pc = new.pc + 4;

    if((opcode(new.IFID.instr) != BEQZ_OP)){
      new.IFID.pcPlus1 = new.pc;
    }

    else if((opcode(new.IFID.instr) == BEQZ_OP)){
      ofst = offset(instruction);

      //if the offset is less than zero
      //need to add the offset
      if(ofst < 0){
        new.pc = new.pc + ofst;
        newVal = new.pc;
        newVal -= ofst;
        new.IFID.pcPlus1 = newVal;
      }

      //if the offset is greater than 0
      //set to new.pc
      else {
        new.IFID.pcPlus1 = new.pc;
      }

    }



    /* --------------------- ID stage --------------------- */
    //reference the pointer
    new.IDEX.instr = state -> IFID.instr;
    instruction = new.IDEX.instr;


    //assign 
    new.IDEX.offset = offset(instruction);
    new.IDEX.pcPlus1 = state -> IFID.pcPlus1;
    new.IDEX.readRegA = new.reg[field_r1(instruction)];
    new.IDEX.readRegA = new.reg[field_r2(instruction)];


    //helpID(state, instruction, extraCycles, new);

    if(OPCheck(instruction, state) == 1){
      helpID(state, instruction, extraCycles, new);
    }

    /* --------------------- EX stage --------------------- */

    new.EXMEM.instr = state->IDEX.instr;
    instruction = new.EXMEM.instr;

  //this is the forwarding check
  //remodel this later
    funct = func(instruction);
    
    prev1 = state->EXMEM.instr;
    opPrev1 = opcode(prev1);
    prev2 = state->MEMWB.instr;
    opPrev2 = opcode(prev2);
    prev3 = state->WBEND.instr;
    opPrev3 = opcode(prev3);

    readRegA = new.reg[field_r1(instruction)];
    readRegB = new.reg[field_r2(instruction)];


    s1 = field_r1(instruction);
    s2 = field_r2(instruction);

    switch(opPrev3){
      case REG_REG_OP:
        if (s1!=0 && s1 == field_r3(prev3) && opPrev3 != opcode(NOPINSTRUCTION)) {
	readRegA = state->WBEND.writeData;
      }
      if (s2!=0 && s2 == field_r3(prev3)  && opPrev3 != opcode(NOPINSTRUCTION)) {
	readRegB = state->WBEND.writeData;
      }
      break;

      case LW_OP:
        if (s1!=0 && s1 == field_r2(prev3) && opPrev3 != opcode(NOPINSTRUCTION)) {
	  readRegA = state->WBEND.writeData;
	}
	if (s2!=0 && s2 == field_r2(prev3) && opPrev3 != opcode(NOPINSTRUCTION)) {
	  readRegB = state->WBEND.writeData;
	}
      break;

      case ADDI_OP:

        if (s1!=0 && s1 == field_r2(prev3) && opPrev3 != opcode(NOPINSTRUCTION)) {
	  readRegA = state->WBEND.writeData;
	}
	if (s2!=0 && s2 == field_r2(prev3) && opPrev3 != opcode(NOPINSTRUCTION)) {
	  readRegB = state->WBEND.writeData;
	}
      break;

      case BEQZ_OP:

        if (s1!=0 && s1 == field_r2(prev3) && opPrev3 != opcode(NOPINSTRUCTION)) {
	  readRegA = state->WBEND.writeData;
	}
	if (s2!=0 && s2 == field_r2(prev3) && opPrev3 != opcode(NOPINSTRUCTION)) {
	  readRegB = state->WBEND.writeData;
	}
      break;

      case SW_OP:

        if (s2!=0 && s2 == field_r2(prev3) && opPrev3 != opcode(NOPINSTRUCTION)) {
	readRegB = state->EXMEM.aluResult;
      }
      break;
    }

    switch(opPrev2){
      case REG_REG_OP:
        if (s1!=0 && s1 == field_r3(prev2) && opPrev2 != opcode(NOPINSTRUCTION)) {
	readRegA = state->MEMWB.writeData;
      }
      if (s2!=0 && s2 == field_r3(prev2) && opPrev2 != opcode(NOPINSTRUCTION)) {
	readRegB = state->MEMWB.writeData;
      }
      break;

      case LW_OP:

        if (s1!=0 && s1 == field_r2(prev2) && opPrev2 != opcode(NOPINSTRUCTION)) {
	  readRegA = state->MEMWB.writeData;
	}
	if (s2!=0 && s2 == field_r2(prev2) && opPrev2 != opcode(NOPINSTRUCTION)) {
	  readRegB = state->MEMWB.writeData;
	}
      break;

      case ADDI_OP:

        if (s1!=0 && s1 == field_r2(prev2) && opPrev2 != opcode(NOPINSTRUCTION)) {
	  readRegA = state->MEMWB.writeData;
	}
	if (s2!=0 && s2 == field_r2(prev2) && opPrev2 != opcode(NOPINSTRUCTION)) {
	  readRegB = state->MEMWB.writeData;
	}
      break;

      case BEQZ_OP:

      if (s1!=0 && s1 == field_r2(prev2) && opPrev2 != opcode(NOPINSTRUCTION)) {
	  readRegA = state->MEMWB.writeData;
	}
	if (s2!=0 && s2 == field_r2(prev2) && opPrev2 != opcode(NOPINSTRUCTION)) {
	  readRegB = state->MEMWB.writeData;
	}
      break;
    }


    switch(opPrev1){

      case REG_REG_OP:
        if (s1!=0 && s1 == field_r3(prev1)) {
	readRegA = state->EXMEM.aluResult;
      }
      if (s2!=0 && s2 == field_r3(prev1)) {
	readRegB = state->EXMEM.aluResult;
      }
      break;

      case LW_OP:

        if (s1!=0 && s1 == field_r2(prev1) && opPrev1 != opcode(NOPINSTRUCTION)) {
	  readRegA = state->EXMEM.aluResult;
	}
	if (s2!=0 && s2 == field_r2(prev1) && opPrev1 != opcode(NOPINSTRUCTION) &&
	    opcode(instruction) != opPrev1) {
	  readRegB = state->EXMEM.aluResult;
	}
      break;

      case ADDI_OP:

      if (s1!=0 && s1 == field_r2(prev1) && opPrev1 != opcode(NOPINSTRUCTION)) {
	  readRegA = state->EXMEM.aluResult;
	}
	if (s2!=0 && s2 == field_r2(prev1) && opPrev1 != opcode(NOPINSTRUCTION) &&
	    opcode(instruction) != opPrev1) {
	  readRegB = state->EXMEM.aluResult;
	}
      break;

      case BEQZ_OP:

      if (s1!=0 && s1 == field_r2(prev1) && opPrev1 != opcode(NOPINSTRUCTION)) {
	  readRegA = state->EXMEM.aluResult;
	}
	if (s2!=0 && s2 == field_r2(prev1) && opPrev1 != opcode(NOPINSTRUCTION) &&
	    opcode(instruction) != opPrev1) {
	  readRegB = state->EXMEM.aluResult;
	}
      break;
  }


  //EXECUTION STAGE
  switch(opcode(instruction)){
    case REG_REG_OP:
      executeFunction(instruction, new, funct, readRegA, readRegB);
      break;

    case ADDI_OP:
      new.EXMEM.aluResult = readRegA + offset(instruction);
      new.EXMEM.readRegB = state->IDEX.readRegB;
      break;

    case LW_OP:
      new.EXMEM.aluResult = readRegA + field_imm(instruction);
      new.EXMEM.readRegB = new.reg[field_r2(instruction)];
      break;

    case SW_OP:
      new.EXMEM.readRegB = readRegB;
      new.EXMEM.aluResult = readRegA + field_imm(instruction);
      break;

    case BEQZ_OP:
      if ((state->IDEX.offset > 0 && readRegA==0) ||
        (state->IDEX.offset < 0 && readRegA!=0)) {
          new.IFID.instr = NOPINSTRUCTION;
          new.IDEX.instr = NOPINSTRUCTION;
          new.pc = state->IDEX.offset + state->IDEX.pcPlus1;
          
          new.IFID.pcPlus1 = new.IDEX.pcPlus1 = 0;
          new.IDEX.readRegA = new.IDEX.readRegB = 0;
          new.IDEX.offset = 32;

          new.EXMEM.aluResult = state->IDEX.pcPlus1 + offset(instruction);
          new.EXMEM.readRegB = new.reg[field_r2(instruction)];
          break;
        } 
          
      else {
        new.EXMEM.readRegB = new.reg[field_r2(instruction)];
        new.EXMEM.aluResult = state->IDEX.pcPlus1 + offset(instruction);
        break;
      }
      //break;

    default:
      new.EXMEM.aluResult = 0;
      new.EXMEM.readRegB = 0;


  }


    /* --------------------- MEM stage --------------------- */

    new.MEMWB.instr = state->EXMEM.instr;
    instruction = new.MEMWB.instr;

   switch(opcode(instruction)){

    case REG_REG_OP:
      new.MEMWB.writeData = state->EXMEM.aluResult;
      break;


    case LW_OP:
      new.MEMWB.writeData = state->dataMem[state->EXMEM.aluResult/4];
    
    case SW_OP:
      new.MEMWB.writeData = state->EXMEM.readRegB;
      new.dataMem[(field_imm(instruction) + new.reg[field_r1(instruction)])/4] =
	    new.MEMWB.writeData;
      break;

    case ADDI_OP:
      new.MEMWB.writeData = state->EXMEM.aluResult;
      break;

    case BEQZ_OP:
      new.MEMWB.writeData = state->EXMEM.aluResult;
      break;

    case HALT_OP:
      new.MEMWB.writeData = 0;
      break;



   }

    /* --------------------- WB stage --------------------- */

    new.WBEND.instr = state->MEMWB.instr;
    instruction = new.WBEND.instr;

    switch(opcode(instruction)){

      case REG_REG_OP:
        new.reg[field_r3(instruction)] = state->MEMWB.writeData;
        new.WBEND.writeData = state->MEMWB.writeData;

      case LW_OP:
        new.WBEND.writeData = state->MEMWB.writeData;
        new.reg[field_r2(instruction)] = new.WBEND.writeData;

      case SW_OP:
        new.WBEND.writeData = state->MEMWB.writeData;

      case ADDI_OP:
        new.WBEND.writeData = state->MEMWB.writeData;
        new.reg[field_r2(instruction)] = new.WBEND.writeData;

      case BEQZ_OP:
        new.WBEND.writeData = state->MEMWB.writeData;

      case HALT_OP:
        printf("machine halted\n");
        printf("total of %d cycles executed\n", state->cycles) ;
        exit(0);
    }


    /* --------------------- end stage --------------------- */
    readRegA = 0;
    readRegB = 0;
    /* transfer new state into current state */
    memcpy(state, &new, sizeof(state_t));
  }
}
/************************************************************/

/************************************************************/
int opcode(int instruction) { return (instruction >> OP_SHIFT) & OP_MASK; }
/************************************************************/

/************************************************************/
int func(int instruction) { return (instruction & FUNC_MASK); }
/************************************************************/

/************************************************************/
int field_r1(int instruction) { return (instruction >> R1_SHIFT) & REG_MASK; }
/************************************************************/

/************************************************************/
int field_r2(int instruction) { return (instruction >> R2_SHIFT) & REG_MASK; }
/************************************************************/

/************************************************************/
int field_r3(int instruction) { return (instruction >> R3_SHIFT) & REG_MASK; }
/************************************************************/

/************************************************************/
int field_imm(int instruction) { return (instruction & IMMEDIATE_MASK); }
/************************************************************/

/************************************************************/
int offset(int instruction) {
  /* only used for lw, sw, beqz */
  return convertNum(field_imm(instruction));
}
/************************************************************/

/************************************************************/
int convertNum(int num) {
  /* convert a 16 bit number into a 32-bit Sun number */
  if (num & 0x8000) {
    num -= 65536;
  }
  return (num);
}
/************************************************************/

/************************************************************/
void printState(Pstate state) {
  short i;
  printf("@@@\nstate before cycle %d starts\n", state->cycles);
  printf("\tpc %d\n", state->pc);

  printf("\tdata memory:\n");
  for (i = 0; i < state->numMemory; i++) {
    printf("\t\tdataMem[ %d ] %d\n", i, state->dataMem[i]);
  }
  printf("\tregisters:\n");
  for (i = 0; i < NUMREGS; i++) {
    printf("\t\treg[ %d ] %d\n", i, state->reg[i]);
  }
  printf("\tIFID:\n");
  printf("\t\tinstruction ");
  printInstruction(state->IFID.instr);
  printf("\t\tpcPlus1 %d\n", state->IFID.pcPlus1);
  printf("\tIDEX:\n");
  printf("\t\tinstruction ");
  printInstruction(state->IDEX.instr);
  printf("\t\tpcPlus1 %d\n", state->IDEX.pcPlus1);
  printf("\t\treadRegA %d\n", state->IDEX.readRegA);
  printf("\t\treadRegB %d\n", state->IDEX.readRegB);
  printf("\t\toffset %d\n", state->IDEX.offset);
  printf("\tEXMEM:\n");
  printf("\t\tinstruction ");
  printInstruction(state->EXMEM.instr);
  printf("\t\taluResult %d\n", state->EXMEM.aluResult);
  printf("\t\treadRegB %d\n", state->EXMEM.readRegB);
  printf("\tMEMWB:\n");
  printf("\t\tinstruction ");
  printInstruction(state->MEMWB.instr);
  printf("\t\twriteData %d\n", state->MEMWB.writeData);
  printf("\tWBEND:\n");
  printf("\t\tinstruction ");
  printInstruction(state->WBEND.instr);
  printf("\t\twriteData %d\n", state->WBEND.writeData);
}
/************************************************************/

/************************************************************/
void printInstruction(int instr) {

  if (opcode(instr) == REG_REG_OP) {

    if (func(instr) == ADD_FUNC) {
      print_rtype(instr, "add");
    } else if (func(instr) == SLL_FUNC) {
      print_rtype(instr, "sll");
    } else if (func(instr) == SRL_FUNC) {
      print_rtype(instr, "srl");
    } else if (func(instr) == SUB_FUNC) {
      print_rtype(instr, "sub");
    } else if (func(instr) == AND_FUNC) {
      print_rtype(instr, "and");
    } else if (func(instr) == OR_FUNC) {
      print_rtype(instr, "or");
    } else {
      printf("data: %d\n", instr);
    }

  } else if (opcode(instr) == ADDI_OP) {
    print_itype(instr, "addi");
  } else if (opcode(instr) == LW_OP) {
    print_itype(instr, "lw");
  } else if (opcode(instr) == SW_OP) {
    print_itype(instr, "sw");
  } else if (opcode(instr) == BEQZ_OP) {
    print_itype(instr, "beqz");
  } else if (opcode(instr) == HALT_OP) {
    printf("halt\n");
  } else {
    printf("data: %d\n", instr);
  }
}
/************************************************************/

/************************************************************/
void print_rtype(int instr, const char *name) {
  printf("%s %d %d %d\n", name, field_r3(instr), field_r1(instr),
         field_r2(instr));
}
/************************************************************/

/************************************************************/
void print_itype(int instr, const char *name) {
  printf("%s %d %d %d\n", name, field_r2(instr), field_r1(instr),
         offset(instr));
}
/************************************************************/

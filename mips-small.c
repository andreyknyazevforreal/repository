/*
 *     $Author: yeung $
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define NUMREGS 32 /* number of machine registers */
#define MAXLINELENGTH 1000
#define MEMSIZE 0x7fff	/* maximum number of memory words */

#define OP_SHIFT   26
#define R1_SHIFT  21
#define R2_SHIFT  16
#define R3_SHIFT   11

#define OP_MASK    0x3F
#define REG_MASK   0x1F
#define FUNC_MASK  0x7FF
#define IMMEDIATE_MASK 0xFFFF

#define LW_OP       0x23
#define SW_OP       0x2B
#define ADDI_OP     0x8
#define REG_REG_OP  0x0
#define BEQZ_OP     0x4
#define HALT_OP     0x3F

#define ADD_FUNC  0x20
#define SLL_FUNC  0x4
#define SRL_FUNC  0x6
#define SUB_FUNC  0x22
#define AND_FUNC  0x24
#define OR_FUNC   0x25


typedef struct stateStruct {
    int pc;
    unsigned int mem[MEMSIZE];
    unsigned int reg[NUMREGS];
    int numMemory;
} stateType, *Pstate;

void printState(Pstate);
void run(Pstate);
int convertNum(int);

int Instructions=0;

/************************************************************/
int main(int argc, char **argv) {
    int i;
    char line[MAXLINELENGTH];
    stateType state;
    FILE *filePtr;
    
    if (argc != 2) {
        printf("error: usage: %s <machine-code file>\n", argv[0]);
        exit(1);
    }
    
    /* initialize memories and registers */
    for (i=0; i<MEMSIZE; i++) {
        state.mem[i] = 0;
    }
    for (i=0; i<NUMREGS; i++) {
        state.reg[i] = 0;
    }
    
    state.pc=0;
    
    /* read machine-code file into instruction/data memory (starting at
     address 0) */
    filePtr = fopen(argv[1], "r");
    if (filePtr == NULL) {
        printf("error: can't open file %s\n", argv[1]);
        perror("fopen");
        exit(1);
    }
    
    for (state.numMemory=0; fgets(line, MAXLINELENGTH, filePtr) != NULL;
         state.numMemory++) {
        if (sscanf(line, "%x", &(state.mem[state.numMemory])) != 1) {
            printf("error in reading address %d\n", state.numMemory);
            exit(1);
        }
        printf("memory[%d]=%08x\n", state.numMemory,
               state.mem[state.numMemory]);
        fflush(stdout);
    }
    
    printf("\n");
    
    run(&state);
    
    printState(&state);

    return 0;
}
/************************************************************/

/************************************************************/
void run(Pstate state) {
    int arg0, arg1, arg2, addressField, func;
    int opcode;
    int word_pc;
    
    for (; 1; Instructions++) { /* infinite loop, exits when it executes
                                 halt */
        
        /* decode the instruction */
        word_pc = state->pc >> 2;
        opcode = (state->mem[word_pc] >> OP_SHIFT) & OP_MASK;
        arg0 = (state->mem[word_pc] >> R1_SHIFT) & REG_MASK;
        arg1 = (state->mem[word_pc] >> R2_SHIFT) & REG_MASK;
        /* only for R-type */
        arg2 = (state->mem[word_pc] >> R3_SHIFT) & REG_MASK;
        func = state->mem[word_pc] & FUNC_MASK;
        /* only for beqz, lw, sw */
        addressField = convertNum(state->mem[word_pc] & IMMEDIATE_MASK);
        
        /* execute the instruction */
        state->pc += 4;
        if (opcode == REG_REG_OP) {
            
            if (func == ADD_FUNC) {
                state->reg[arg2] = state->reg[arg0] + state->reg[arg1];
            } else if (func == SLL_FUNC) {
                state->reg[arg2] = state->reg[arg0] << state->reg[arg1];
            } else if (func == SRL_FUNC) {
                state->reg[arg2] = ( (unsigned int) state->reg[arg0]) >> state->reg[arg1];
            } else if (func == SUB_FUNC) {
                state->reg[arg2] = state->reg[arg0] - state->reg[arg1];
            } else if (func == AND_FUNC) {
                state->reg[arg2] = state->reg[arg0] & state->reg[arg1];
            } else if (func == OR_FUNC) {
                state->reg[arg2] = state->reg[arg0] | state->reg[arg1];
            }
            
        } else if (opcode == ADDI_OP) {
            state->reg[arg1] = state->reg[arg0] + addressField;
        } else if (opcode == LW_OP) {
            state->reg[arg1] =
            state->mem[(state->reg[arg0] + addressField) >> 2];
        } else if (opcode == SW_OP) {
            state->mem[(state->reg[arg0] + addressField) >> 2] =
            state->reg[arg1];
        } else if (opcode == BEQZ_OP) {
            if (state->reg[arg0] == 0) {
                state->pc += addressField;
            }
        } else if (opcode == HALT_OP) {
            printf("machine halted\n");
            printf("total of %d instructions executed\n", Instructions+1);
            printState(state);
            exit(0);
        } else {
            printf("error: illegal opcode %x\n", opcode);
            exit(1);
        }
        
        state->reg[0]= 0;
        
        printState(state);
    }
}
/************************************************************/

/************************************************************/
void printState(Pstate state) {
    short i;
    printf("state after cycle %d:\n", Instructions);
    printf("\tpc=%d\n", state->pc);
    printf("\tmemory:\n");
    for (i=0; i<state->numMemory; i++) {
        printf("\t\tmem[%d] 0x%x\t(%d)\n",
               i, state->mem[i], state->mem[i]);
    }
    printf("\tregisters:\n");
    for (i=0; i<NUMREGS; i++) {
        printf("\t\treg[%d] 0x%x\t(%d)\n",
               i, state->reg[i], state->reg[i]);
    }
    printf("\n");
    fflush(stdout);
}
/************************************************************/

/************************************************************/
int convertNum(int num) {
    /* convert a 16 bit number into a 32-bit Sun number */
    if (num & 0x8000) {
        num -= 65536;
    }
    return(num);
}
/************************************************************/

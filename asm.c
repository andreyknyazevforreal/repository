/*
 * Author: Yeung
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXLINELENGTH 1000
#define MAXNUMLABELS 10000
#define MAXLABELLENGTH 10
#define VALUE_32K  32768
#define IMMEDIATE_MASK 0xFFFF

#define OP_SHIFT 26
#define A_SHIFT  21
#define B_SHIFT  16
#define D_SHIFT  11

#define LW_OP     0x23
#define SW_OP     0x2B
#define ADDI_OP   0x8
#define ADD_OP    0x0
#define SLL_OP    0x0
#define SRL_OP    0x0
#define SUB_OP    0x0
#define AND_OP    0x0
#define OR_OP     0x0
#define BEQZ_OP   0x4
#define JALR_OP   0x13
#define HALT_OP   0x3F

#define ADD_FUNC  0x20
#define SLL_FUNC  0x4
#define SRL_FUNC  0x6
#define SUB_FUNC  0x22
#define AND_FUNC  0x24
#define OR_FUNC   0x25

char * readAndParse(FILE *, char *, char **, char **, char **, char **, char **);
int isNumber(char *);

char Labels[MAXNUMLABELS][MAXLABELLENGTH];
int Addresses[MAXNUMLABELS];
int NumValidLabels=0;

int get_label_address(char* s) {
    int i;
    
    for (i=0; i<NumValidLabels; i++) {
        if (strlen(Labels[i]) == 0) {
            return -1;
        }
        if (strcmp(Labels[i], s) == 0) {
            return Addresses[i];
        }
    }
    return -1;
}

int main(int argc, char *argv[]) {
    char *inFileString, *outFileString;
    FILE *inFilePtr, *outFilePtr;
    int address;
    char *label, *opcode, *arg0, *arg1, *arg2;
    char lineString[MAXLINELENGTH+1];
    int i;
    int num;
    int immediateValue;
    
    if (argc != 3) {
        printf("error: usage: %s <assembly-code-file> <machine-code-file> \n",
               argv[0]);
        return 1;
    }
    
    inFileString = argv[1];
    outFileString = argv[2];
    
    inFilePtr = fopen(inFileString, "r");
    if (inFilePtr == NULL) {
        printf("error in opening %s\n", inFileString);
        return 1;
    }
    outFilePtr = fopen(outFileString, "w");
    if (outFilePtr == NULL) {
        printf("error in opening %s\n", outFileString);
        return 1;
    }
    
    /* PASS ONE -- map symbols to addresses */
    
    /* assume address start at 0 */
    address = 0;
    
    while(readAndParse(inFilePtr, lineString, &label, &opcode, &arg0,
                       &arg1, &arg2) != NULL) {
        
        if (label != NULL) {
            /* look for duplicate label */
            if ((i = get_label_address(label)) < 0) {
                /* label not found -- a good sign */
                /* but first -- make sure we don't overrun buffer */
                if (NumValidLabels >= MAXNUMLABELS) {
                    /* we will exceed the size of the array */
                    printf("error: too many labels (label=%s)\n", label);
                    return 1;
                }
                strcpy(Labels[NumValidLabels], label);
                Addresses[NumValidLabels] = address;
                NumValidLabels++;
            } else {
                /* duplicate label -- terminate */
                printf("error: duplicate label %s \n", label);
                return 1;
            }
        }
        
        address += 4;
    }
    
    /* PASS TWO -- print machine code, with symbols filled in as addresses */
    
    rewind(inFilePtr);
    address = 0;
    
    while(readAndParse(inFilePtr, lineString, &label, &opcode, &arg0,
                       &arg1, &arg2) != NULL) {
        
        /* could just as easily have done this in the first pass ... */
        if ( (!strcmp(opcode, "halt") && strcmp(opcode, ".fill")
              && !strcmp(opcode, "jalr") && arg2==NULL) ||
            (!strcmp(opcode, "jalr") && arg1==NULL) ||
            (!strcmp(opcode, ".fill") && arg0==NULL)) {
            printf("error at address %d: not enough arguments\n", address);
            return 1;
        }
        
        if (!strcmp(opcode, "add")) {
            num = (ADD_OP << OP_SHIFT) | (atoi(arg1) << A_SHIFT) |
            (atoi(arg2) << B_SHIFT) | (atoi(arg0) << D_SHIFT) |
            ADD_FUNC;
        } else if (!strcmp(opcode, "sll")) {
            num = (SLL_OP << OP_SHIFT) | (atoi(arg1) << A_SHIFT) |
            (atoi(arg2) << B_SHIFT) | (atoi(arg0) << D_SHIFT) |
            SLL_FUNC;
        } else if (!strcmp(opcode, "srl")) {
            num = (SLL_OP << OP_SHIFT) | (atoi(arg1) << A_SHIFT) |
            (atoi(arg2) << B_SHIFT) | (atoi(arg0) << D_SHIFT) |
            SRL_FUNC;
        } else if (!strcmp(opcode, "sub")) {
            num = (SLL_OP << OP_SHIFT) | (atoi(arg1) << A_SHIFT) |
            (atoi(arg2) << B_SHIFT) | (atoi(arg0) << D_SHIFT) |
            SUB_FUNC;
        } else if (!strcmp(opcode, "and")) {
            num = (SLL_OP << OP_SHIFT) | (atoi(arg1) << A_SHIFT) |
            (atoi(arg2) << B_SHIFT) | (atoi(arg0) << D_SHIFT) |
            AND_FUNC;
        } else if (!strcmp(opcode, "or")) {
            num = (SLL_OP << OP_SHIFT) | (atoi(arg1) << A_SHIFT) |
            (atoi(arg2) << B_SHIFT) | (atoi(arg0) << D_SHIFT) |
            OR_FUNC;
        } else if (!strcmp(opcode, "jalr")) {
            num = (JALR_OP << OP_SHIFT) | (atoi(arg0) << A_SHIFT);
        } else if (!strcmp(opcode, "halt")) {
            num = (HALT_OP << OP_SHIFT);
        } else if (!strcmp(opcode, "addi")) {
            if (isNumber(arg2)) {
                immediateValue = atoi(arg2);
            } else {
                immediateValue = get_label_address(arg2);
            }
            if (immediateValue < -VALUE_32K || immediateValue >= VALUE_32K) {
                printf("error: immediate %u (%u words) for addi out of range\n",
                       immediateValue, immediateValue >> 2);
                return 1;
            }
            /* truncate the offset field, in case it's negative */
            immediateValue = immediateValue & IMMEDIATE_MASK;
            num = (ADDI_OP << OP_SHIFT) | (atoi(arg1) << A_SHIFT) |
            (atoi(arg0) << B_SHIFT) | immediateValue;
        } else if (!strcmp(opcode, "lw") || !strcmp(opcode, "sw")) {
            /* if arg2 is symbolic, then translate into an address */
            if (isNumber(arg2)) {
                immediateValue = atoi(arg2);
            } else {
                immediateValue = get_label_address(arg2);
            }
            if (immediateValue < -VALUE_32K || immediateValue >= VALUE_32K) {
                printf("error: offset %u (%u words) for ld-st out of range\n",
                       immediateValue, immediateValue >> 2);
                return 1;
            }
            /* truncate the offset field, in case it's negative */
            immediateValue = immediateValue & IMMEDIATE_MASK;
            if (!strcmp(opcode, "lw")) {
                num = (LW_OP << OP_SHIFT) | (atoi(arg1) << A_SHIFT) |
                (atoi(arg0) << B_SHIFT) | immediateValue;
            } else {
                num = (SW_OP << OP_SHIFT) | (atoi(arg1) << A_SHIFT) |
                (atoi(arg0) << B_SHIFT) | immediateValue;
            }
        } else if (!strcmp(opcode, "beqz")) {
            /* if arg2 is symbolic, then translate into an address */
            if (isNumber(arg2)) {
                immediateValue = atoi(arg2);
            } else {
                immediateValue = get_label_address(arg2) - address - 4;
            }
            if (immediateValue < -VALUE_32K || immediateValue >= VALUE_32K) {
                printf("error: offset %u (%u words) for beqz out of range\n",
                       immediateValue, immediateValue >> 2);
                return 1;
            }
            /* truncate the offset field, in case it's negative */
            immediateValue = immediateValue & IMMEDIATE_MASK;
            num = (BEQZ_OP << OP_SHIFT) | (atoi(arg0) << B_SHIFT) |
            (atoi(arg1) << A_SHIFT) | immediateValue;
        } else if (!strcmp(opcode, ".fill")) {
            if (!isNumber(arg0)) {
                num = get_label_address(arg0);
            } else {
                num = atoi(arg0);
            }
        } else {
            printf("error: unrecognized opcode %s at address %d\n",
                   opcode, address);
            return 1;
        }
        
        fprintf(outFilePtr, "%08x\n", num);
        
        address += 4;
    }
    
    return 0;
}

char *readAndParse(FILE *inFilePtr, char *lineString, char **labelPtr,
                   char **opcodePtr, char **arg0Ptr, char **arg1Ptr,
                   char **arg2Ptr) {
    char *statusString;
    
    statusString = fgets(lineString, MAXLINELENGTH, inFilePtr);
    if (statusString != NULL) {
        if (lineString[0] == '\t') {
            *labelPtr = NULL;
            *opcodePtr = strtok(lineString, "\t\n");
        } else {
            *labelPtr = strtok(lineString, "\t\n");
            *opcodePtr = strtok(NULL, "\t\n");
        }
        *arg0Ptr = strtok(NULL, "\t\n");
        *arg1Ptr = strtok(NULL, "\t\n");
        *arg2Ptr = strtok(NULL, "\t\n");
    }
    return(statusString);
}

int isNumber(char *string) {
    /* return 1 if string is a number */
    int i;
    return( (sscanf(string, "%d", &i)) == 1);
}


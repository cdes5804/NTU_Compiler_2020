#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "header.h"
#include "symbolTable.h"
#include "codeGeneration.h"

#define SYS_ERR_EXIT(msg) { perror(msg); exit(127); }

static const char OUTPUT_FILE_NAME[] = "output.s";
FILE *fout;

void codeGeneration(AST_NODE* program)
{
    fout = fopen(OUTPUT_FILE_NAME, "wt");
    if (!fout)
        SYS_ERR_EXIT("Error when open output file");
    genProgram(program);
    fclose(fout);
}

void genProgram(AST_NODE* programNode)
{
    AST_NODE *globalDeclarationNode = programNode->child;
    while (globalDeclarationNode) {
        AST_TYPE nodeType = globalDeclarationNode->nodeType;
        if (nodeType == VARIABLE_DECL_LIST_NODE) { // variable declaration
            genGeneralNode(globalDeclarationNode);
        } else if (nodeType == DECLARATION_NODE) { // function declaration
            genDeclarationNode(globalDeclarationNode);
        }
        globalDeclarationNode = globalDeclarationNode->rightSibling;
    }
}

/******************************
 * Register Management
 ******************************/

int temporaryRegisters[] = {5, 6, 7, 28, 29, 30, 31};
int savedRegisters[] = {9, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27};
int allocatableRegisters[] = {9, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 5, 6, 7, 28, 29, 30, 31};
int argumentRegisters[] = {10, 11, 12, 13, 14, 15, 16, 17};

bool regAvailable[32];   // if a register is available
bool regUsed[32];        // if a register has been used (for callee-saved registers)

void initReg()
{
    int numReg = sizeof(allocatableRegisters) / sizeof(allocatableRegisters[0]);
    for (int i = 0; i < numReg; i++) {
        int reg = allocatableRegisters[i];
        regAvailable[reg] = true;
        regUsed[reg] = false;
    }
}

int getReg()
{
    int numReg = sizeof(allocatableRegisters) / sizeof(allocatableRegisters[0]);
    for (int i = 0; i < numReg; i++) {
        int reg = allocatableRegisters[i];
        if (regAvailable[reg]) {
            regAvailable[reg] = false;
            regUsed[reg] = true;
            return reg;
        }
    }
    fprintf(stderr, "Error: Out of Registers\n");
    exit(1);
}

void freeReg(int reg)
{
    regAvailable[reg] = true;
}

bool isSavedRegister(int reg)
{
    for (int i = 0; i < sizeof(savedRegisters) / sizeof(savedRegisters[0]); i++)
        if (savedRegisters[i] == reg)
            return true;
    return false;
}

bool isTempRegister(int reg)
{
    for (int i = 0; i < sizeof(temporaryRegisters) / sizeof(temporaryRegisters[0]); i++)
        if (temporaryRegisters[i] == reg)
            return true;
    return false;
}

/******************************
 * Stack Management
 ******************************/

long long curFrameSize;

long long pushStack(long long size)
{
    curFrameSize += size;
    return curFrameSize;
}

long long getFrameSize()
{
    return curFrameSize;
}

#include <stdio.h>
#include <stdlib.h>

#include "codeGeneration.h"
#include "regAllocation.h"

extern FILE* fout;

AssociationTableEntry associationTable[32];

int temporaryRegisters[] = {5, 6, 7, 28, 29, 30, 31};
int savedRegisters[] = {9, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27};
int allocatableRegisters[] = {9, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 5, 6, 7, 28, 29, 30, 31};
int argumentRegisters[] = {10, 11, 12, 13, 14, 15, 16, 17};

void initAssociationTable()
{
    for (int i = 0; i < sizeof(allocatableRegisters) / sizeof(allocatableRegisters[0]); i++) {
        int reg = allocatableRegisters[i];
        associationTable[reg].status = AVAILABLE;
    }
}

AssociationType getAssociationType(AST_NODE* node)
{
    switch (node->nodeType) {
        case EXPR_NODE:
        case STMT_NODE:
        case CONST_VALUE_NODE:
            return TEMPORARY;
            break;
        case IDENTIFIER_NODE:
            return SYMBOL;
            break;
        default:
            fprintf(stderr, "getAssociationType: strange node type\n");
            return TEMPORARY;
            break;
    }
}

SymbolTableEntry* getSymtabEntry(AST_NODE* node)
{
    if (node->nodeType != IDENTIFIER_NODE) {
        fprintf(stderr, "getSymtabEntry: only id node has symbol table entry\n");
        return NULL;
    }
    if (!node->semantic_value.identifierSemanticValue.symbolTableEntry) {
        fprintf(stderr, "Bug deteted: symbol table entry is not in AST_NODE\n");
        return NULL;
    }
    return node->semantic_value.identifierSemanticValue.symbolTableEntry;
}

void storeReg(int reg)
{
    switch (associationTable[reg].type) {
        case TEMPORARY:
            associationTable[reg].name.astNode->offset = pushStack32(reg);
            break;
        case SYMBOL:
            fprintf(fout, "sw x%d, %lld\n", reg, associationTable[reg].name.symtabEntry->offset);
            break;
    }
}

int getReg(AST_NODE* node)
{
    AssociationType type = getAssociationType(node);

    int num_reg = sizeof(allocatableRegisters) / sizeof(allocatableRegisters[0]);
    AssociationStatus costOrder[] = {AVAILABLE, NS, S};
    for (int cost = 0; cost <= 2; cost++) {
        AssociationStatus status = costOrder[cost];
        for (int i = 0; i < num_reg; i++) {
            int reg = allocatableRegisters[i];
            if (associationTable[reg].status == AVAILABLE) {
                if (status == S)
                    storeReg(reg);
                switch (type) {
                    case TEMPORARY:
                        associationTable[reg].status = S;
                        associationTable[reg].name.astNode = node;
                        node->place = reg;
                        break;
                    case SYMBOL:
                        associationTable[reg].status = NS;
                        associationTable[reg].name.symtabEntry = getSymtabEntry(node);
                        associationTable[reg].name.symtabEntry->place = reg;
                        break;
                }
                return reg;
            }
        }
    }
    fprintf(stderr, "Error: this line of code should not be reached\n");
}

int useReg(AST_NODE* node)
{
    AssociationType type = getAssociationType(node);
    int reg = node->place;
    switch (type) {
        case TEMPORARY:
            if (associationTable[reg].name.astNode != node) {
                reg = getReg(node);
                fprintf(fout, "lw x%d, -%lld(fp)\n", reg, node->offset);
            }
            break;
        case SYMBOL:
            if (associationTable[reg].name.symtabEntry != getSymtabEntry(node)) {
                reg = getReg(node);
                if (getSymtabEntry(node)->isGlobal) {
                    // TODO
                } else {
                    fprintf(fout, "lw x%d, -%lld(fp)\n", reg, getSymtabEntry(node)->offset);
                }
            }
            break;
    }
    return reg;
}

void freeReg(AST_NODE* node)
{
    AssociationType type = getAssociationType(node);
    int reg = node->place;
    switch (type) {
        case TEMPORARY:
            if (associationTable[reg].name.astNode == node) {
                associationTable[reg].status = AVAILABLE;
            }
            break;
        case SYMBOL:
            if (associationTable[reg].name.symtabEntry == getSymtabEntry(node)) {
                if (associationTable[reg].status == S)
                    fprintf(fout, "sw x%d, -%lld(fp)\n", reg, getSymtabEntry(node)->offset);
                associationTable[reg].status = AVAILABLE;
            }
            break;
    }
}

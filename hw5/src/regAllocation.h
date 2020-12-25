#ifndef __REG_ALLOCATION_H__
#define __REG_ALLOCATION_H__

#include "symbolTable.h"
#include "header.h"

typedef enum {
    TEMPORARY, SYMBOL
} AssociationType;

typedef union {
    SymbolTableEntry* symtabEntry;
    AST_NODE* astNode;
} AssociationName;

typedef enum {
    S, NS, AVAILABLE
} AssociationStatus;

typedef struct {
    AssociationType type;
    AssociationName name;
    AssociationStatus status;
} AssociationTableEntry;

int getReg(AST_NODE* name);

#endif
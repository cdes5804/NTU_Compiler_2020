#include "symbolTable.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
// This file is for reference only, you are not required to follow the implementation. //

int HASH(char* str) {
	int idx = 0;
	while (*str) {
		idx = idx << 1;
		idx += *str;
		str++;
	}
	return (idx & (HASH_TABLE_SIZE - 1));
}

SymbolTable symbolTable;

SymbolTableEntry* newSymbolTableEntry(int nestingLevel)
{
    SymbolTableEntry* symbolTableEntry = (SymbolTableEntry*)malloc(sizeof(SymbolTableEntry));
    symbolTableEntry->nextInHashChain = NULL;
    symbolTableEntry->prevInHashChain = NULL;
    symbolTableEntry->nextInSameLevel = NULL;
    symbolTableEntry->sameNameInOuterLevel = NULL;
    symbolTableEntry->attribute = NULL;
    symbolTableEntry->name = NULL;
    symbolTableEntry->nestingLevel = nestingLevel;
    return symbolTableEntry;
}

void removeFromHashChain(int hashIndex, SymbolTableEntry* entry)
{
    if (entry->prevInHashChain) {
        entry->prevInHashChain->nextInHashChain = entry->nextInHashChain;
    } else {
        symbolTable.hashTable[hashIndex] = entry->nextInHashChain;
    }
    
    if (entry->nextInHashChain) {
        entry->nextInHashChain->prevInHashChain = entry->prevInHashChain;
    }

    /*
        Cannot free the entry yet. The entry may be needed when the program returned to the scope it resides in.
        The entry will be freed when the scope it resides in is closed.
     */
    entry->nextInHashChain = entry->prevInHashChain = NULL;
}

void insertIntoHashChain(int hashIndex, SymbolTableEntry* entry)
{
    SymbolTableEntry* head = symbolTable.hashTable[hashIndex];
    if (head)
        head->prevInHashChain = entry;
    entry->nextInHashChain = head;
    symbolTable.hashTable[hashIndex] = entry;
}

SymbolAttribute* createTypeSymbol(DATA_TYPE dataType)
{
    SymbolAttribute* typeAttribute = (SymbolAttribute*)malloc(sizeof(SymbolAttribute));
    typeAttribute->attributeKind = TYPE_ATTRIBUTE;
    typeAttribute->attr.typeDescriptor = (TypeDescriptor*)malloc(sizeof(TypeDescriptor));
    typeAttribute->attr.typeDescriptor->kind = SCALAR_TYPE_DESCRIPTOR;
    typeAttribute->attr.typeDescriptor->properties.dataType = dataType;
    return typeAttribute;
}

SymbolAttribute* createSysLibFunctionAttribute(DATA_TYPE dataType)
{
    SymbolAttribute* functionAttribute = (SymbolAttribute*)malloc(sizeof(SymbolAttribute));
    functionAttribute->attributeKind = FUNCTION_SIGNATURE;
    functionAttribute->attr.functionSignature = (FunctionSignature*)malloc(sizeof(FunctionSignature));
    functionAttribute->attr.functionSignature->parametersCount = 0;
    functionAttribute->attr.functionSignature->parameterList = NULL;
    functionAttribute->attr.functionSignature->returnType = dataType;
    return functionAttribute;
}

ScopeStack* getNewScope()
{
    ScopeStack* newScope = (ScopeStack*)malloc(sizeof(ScopeStack));
    newScope->prevScope = NULL;
    newScope->scopeStart = NULL;
    return newScope;
}

void initializeSymbolTable()
{
    symbolTable.currentLevel = 0;
    for (int i = 0; i < HASH_TABLE_SIZE; ++i)
        symbolTable.hashTable[i] = NULL;
    
    symbolTable.scopeStack = getNewScope();

    // insert basic type symbols so processTypeNode can validate basic type
    SymbolAttribute* intAttribute = createTypeSymbol(INT_TYPE);
    insertSymbol(SYMBOL_TABLE_INT_NAME, intAttribute);

    SymbolAttribute* floatAttribute = createTypeSymbol(FLOAT_TYPE);
    insertSymbol(SYMBOL_TABLE_FLOAT_NAME, floatAttribute);

    SymbolAttribute* voidAttribute = createTypeSymbol(VOID_TYPE);
    insertSymbol(SYMBOL_TABLE_VOID_NAME, voidAttribute);

    // insert system functions read and fread
    SymbolAttribute* sysLibReadAttribute = createSysLibFunctionAttribute(INT_TYPE);
    insertSymbol(SYMBOL_TABLE_SYS_LIB_READ, sysLibReadAttribute);

    SymbolAttribute* sysLibFreadAttribute = createSysLibFunctionAttribute(FLOAT_TYPE);
    insertSymbol(SYMBOL_TABLE_SYS_LIB_FREAD, sysLibFreadAttribute);
}

void symbolTableEnd()
{
    for (int i = 0; i < HASH_TABLE_SIZE; ++i) {
        SymbolTableEntry* head = symbolTable.hashTable[i];
        while (head) {
            SymbolTableEntry* next = head->nextInHashChain;
            free(head);
            head = next;
        }
    }
}

SymbolTableEntry* retrieveSymbol(char* symbolName)
{
    int hashIndex = HASH(symbolName);
    SymbolTableEntry* head = symbolTable.hashTable[hashIndex];
    
    while (head) {
        if (strcmp(symbolName, head->name) == 0)
            return head;
        head = head->nextInHashChain;
    }

    return NULL; // not found
}

SymbolTableEntry* insertSymbol(char* symbolName, SymbolAttribute* attribute)
{
    int hashIndex = HASH(symbolName);
    SymbolTableEntry* head = symbolTable.hashTable[hashIndex];
    SymbolTableEntry* newEntry = newSymbolTableEntry(symbolTable.currentLevel);
    newEntry->name = symbolName;
    newEntry->attribute = attribute;

    while (head) {
        if (strcmp(symbolName, head->name) == 0) {
            if (symbolTable.currentLevel == head->nestingLevel) { // already declared in the same scope
                free(newEntry);
                return NULL;
            } else {
                removeFromHashChain(hashIndex, head);
                newEntry->sameNameInOuterLevel = head;
                break;
            }
        }
        head = head->nextInHashChain;
    }

    insertIntoHashChain(hashIndex, newEntry);
    newEntry->nextInSameLevel = symbolTable.scopeStack->scopeStart;
    symbolTable.scopeStack->scopeStart = newEntry;
    return newEntry;
}

//remove the symbol from the current scope
void removeSymbol(char* symbolName)
{
    int hashIndex = HASH(symbolName);
    SymbolTableEntry* head = symbolTable.hashTable[hashIndex];

    while (head) {
        if (strcmp(symbolName, head->name) == 0) {
            if (symbolTable.currentLevel != head->nestingLevel)
                return;
            removeFromHashChain(hashIndex, head);
            if (head->sameNameInOuterLevel)
                insertIntoHashChain(hashIndex, head->sameNameInOuterLevel);
            break;
        }
        head = head->nextInHashChain;
    }

    if (!head) {
        fprintf(stderr, "%s not found.\n", symbolName);
        return;
    }

    head = symbolTable.scopeStack->scopeStart;
    SymbolTableEntry* prev = NULL;

    // remove from scopeStack
    while (head) {
        if (strcmp(symbolName, head->name) == 0) {
            if (!prev) {
                symbolTable.scopeStack->scopeStart = head->nextInSameLevel;
            } else {
                prev->nextInSameLevel = head->nextInSameLevel;
            }
            break;
        }
        prev = head;
        head = head->nextInSameLevel;
    }
}

int declaredLocally(char* symbolName)
{
    int hashIndex = HASH(symbolName);
    SymbolTableEntry* head = symbolTable.hashTable[hashIndex];
    
    while (head) {
        if (strcmp(symbolName, head->name) == 0) {
            if (symbolTable.currentLevel == head->nestingLevel)
                return 1;
            else
                return 0;
        }
        head = head->nextInHashChain;
    }
    return 0;
}

void openNewScope()
{
    symbolTable.currentLevel += 1;
    ScopeStack* newScope = getNewScope();
    newScope->prevScope = symbolTable.scopeStack;
    symbolTable.scopeStack = newScope;
}

void closeCurrentScope()
{
    if (symbolTable.currentLevel == 0) {
        fprintf(stderr, "No opened scope can be closed.\n");
        return;
    }
    SymbolTableEntry* head = symbolTable.scopeStack->scopeStart;

    while (head) {
        int hashIndex = HASH(head->name);
        removeFromHashChain(hashIndex, head);
        if (head->sameNameInOuterLevel)
            insertIntoHashChain(hashIndex, head->sameNameInOuterLevel);
        SymbolTableEntry* next = head->nextInSameLevel;
        head = next;
    }

    ScopeStack* prevScope = symbolTable.scopeStack->prevScope; // return to the previous scope
    symbolTable.scopeStack = prevScope;
    symbolTable.currentLevel -= 1;
}

int isCurrentScopeGlobal()
{
    return symbolTable.currentLevel == 0;
}

#ifndef __SYMBOL_TABLE_H__
#define __SYMBOL_TABLE_H__

#include "header.h"
// This file is for reference only, you are not required to follow the implementation. //


//SYMBOL_TABLE_PREINSERT_NAME
#define SYMBOL_TABLE_INT_NAME "int"
#define SYMBOL_TABLE_FLOAT_NAME "float"
#define SYMBOL_TABLE_VOID_NAME "void"
#define SYMBOL_TABLE_SYS_LIB_READ "read"
#define SYMBOL_TABLE_SYS_LIB_FREAD "fread"
#define HASH_TABLE_SIZE 256


typedef enum SymbolAttributeKind
{
    VARIABLE_ATTRIBUTE,
    TYPE_ATTRIBUTE,
    FUNCTION_SIGNATURE
} SymbolAttributeKind;

typedef enum TypeDescriptorKind
{
    SCALAR_TYPE_DESCRIPTOR,
    ARRAY_TYPE_DESCRIPTOR,
} TypeDescriptorKind;

typedef struct ArrayProperties
{
    int dimension;
    int sizeInEachDimension[MAX_ARRAY_DIMENSION];
    //point to a TypeDescriptor in the symbol table;
    DATA_TYPE elementType;
} ArrayProperties;

typedef struct TypeDescriptor
{
    TypeDescriptorKind kind;
    union
    {
        DATA_TYPE dataType;//kind: SCALAR_TYPE_DESCRIPTOR
        ArrayProperties arrayProperties;//kind: ARRAY_TYPE_DESCRIPTOR
    } properties;
} TypeDescriptor;

typedef struct Parameter
{
    //point to a TypeDescriptor in the symbol table;
    struct Parameter* next;
    TypeDescriptor* type;
    char* parameterName;
} Parameter;

typedef struct FunctionSignature
{
    int parametersCount;
    Parameter* parameterList;
    DATA_TYPE returnType;
} FunctionSignature;

typedef struct SymbolAttribute
{
    SymbolAttributeKind attributeKind;

    union
    {
        TypeDescriptor* typeDescriptor;
        FunctionSignature* functionSignature;
    } attr;
} SymbolAttribute;

typedef struct SymbolTableEntry
{
    struct SymbolTableEntry* nextInHashChain;
    struct SymbolTableEntry* prevInHashChain;
    struct SymbolTableEntry* nextInSameLevel;
    struct SymbolTableEntry* sameNameInOuterLevel;

    char* name;
    SymbolAttribute* attribute;
    int nestingLevel;

    // for code generation
    int isGlobal;
    int place;
    long long offset;
} SymbolTableEntry;

typedef struct ScopeStack
{
    struct ScopeStack* prevScope;
    SymbolTableEntry* scopeStart;
} ScopeStack;

typedef struct SymbolTable
{
    /*
        Scope is maintained by scopeStack.
        There is no concept of scope in hashTable. hashTable only keep tracks of all entries that are available in the 
        current scope.
        hashTable does not care about which scope the entries lives in. If there is a name conflict with the outer scope,
        remove the entry from the outer scope in hashTable and insert the entry in the inner scope so that the hashTable uses
        the entry in the inner scope.
        The entry from the outer scope will not be deleted (tracked by scopeStack) and will return to hashTable when the inner
        scope is closed and the program returns to the outer scope.
    */
    SymbolTableEntry* hashTable[HASH_TABLE_SIZE];
    ScopeStack* scopeStack;
    int currentLevel;
} SymbolTable;


void initializeSymbolTable();
void symbolTableEnd();
SymbolTableEntry* retrieveSymbol(char* symbolName);
SymbolTableEntry* insertSymbol(char* symbolName, SymbolAttribute* attribute);
void removeSymbol(char* symbolName);
int declaredLocally(char* symbolName);
void openNewScope();
void closeCurrentScope();
int isCurrentScopeGlobal();

#endif

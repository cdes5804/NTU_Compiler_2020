#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

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
            genGeneralNode(globalDeclarationNode, NULL);
        } else if (nodeType == DECLARATION_NODE) { // function declaration
            genDeclarationNode(globalDeclarationNode);
        }
        globalDeclarationNode = globalDeclarationNode->rightSibling;
    }
}

void genGeneralNode(AST_NODE* node, char* endLabel)
{
    AST_TYPE nodeType = node->nodeType;
    AST_NODE* childNode = node->child;
    switch (nodeType) {
        case VARIABLE_DECL_LIST_NODE:
            while (childNode) {
                genDeclarationNode(childNode);
                childNode = childNode->rightSibling;
            }
            break;
        case STMT_LIST_NODE:
            while (childNode) {
                genStmtNode(childNode, endLabel);
                childNode = childNode->rightSibling;
            }
            break;
        case NONEMPTY_ASSIGN_EXPR_LIST_NODE:
            while (childNode) {
                //genAssignOrExpr(childNode);
                childNode = childNode->rightSibling;
            }
            break;
        case NONEMPTY_RELOP_EXPR_LIST_NODE:
            while (childNode) {
                //genExprRelatedNode(childNode);
                childNode = childNode->rightSibling;
            }
            break;
        case NUL_NODE:
            break;
        default:
            fprintf(stderr, "Invalid node type in general node\n");
            exit(1);
    }
}

void genDeclarationNode(AST_NODE* declarationNode)
{
    DECL_KIND declarationType = declarationNode->semantic_value.declSemanticValue.kind;
    switch (declarationType) {
        case VARIABLE_DECL:
            genVarDecl(declarationNode);
            break;
        case TYPE_DECL:
            break;
        case FUNCTION_DECL:
            genFuncDecl(declarationNode);
            break;
        case FUNCTION_PARAMETER_DECL:
            //genFuncParamDecl(declarationNode);
            break;
        default:
            fprintf(stderr, "Invalid declaration type in declaration node\n");
            exit(1);
    }
}

int getSymbolSize(SymbolTableEntry* symtabEntry)
{
    TypeDescriptor *typeDescriptor = symtabEntry->attribute->attr.typeDescriptor;
    if (typeDescriptor->kind == SCALAR_TYPE_DESCRIPTOR) {
        return 1;
    } else {
        int size = 1;
        for (int i = 0; i < typeDescriptor->properties.arrayProperties.dimension; i++)
            size *= typeDescriptor->properties.arrayProperties.sizeInEachDimension[i];
        return size;
    }
}

void genGlobalVar(SymbolTableEntry* symtabEntry)
{
    char label[128];
    snprintf(label, 128, "_%s", symtabEntry->name);
    symtabEntry->globalLabel = strdup(label);
    fprintf(fout, ".data\n");
    fprintf(fout, "%s: .word ", label);
    int numElement = getSymbolSize(symtabEntry);
    for (int i = 0; i < numElement; i++)
        fprintf(fout, "0 ");
    fprintf(fout, "\n");
}

void genLocalVar(SymbolTableEntry* symtabEntry)
{
    int size = 4 * getSymbolSize(symtabEntry);
    symtabEntry->offset = pushStack(size);
}

void genVarDecl(AST_NODE* declarationNode)
{
    AST_NODE *idNode = declarationNode->child->rightSibling;
    for (; idNode; idNode = idNode->rightSibling) {
        SymbolTableEntry *symtabEntry = idNode->semantic_value.identifierSemanticValue.symbolTableEntry;
        if (!symtabEntry) {
            fprintf(stderr, "genVarDecl: Symbol Table Entry of ID Node is NULL\n");
            exit(1);
        }
        TypeDescriptor *typeDescriptor = symtabEntry->attribute->attr.typeDescriptor;
        if (symtabEntry->nestingLevel == 0) {
            genGlobalVar(symtabEntry);
        } else {
            genLocalVar(symtabEntry);
        }
    }
}

void genFuncHead(char* funcName)
{
    fprintf(fout, ".text\n"
                  "%s:\n", funcName);
}

char* genPrologue(char* funcName)
{
    fprintf(fout, "\taddi sp, sp, -16\n"
                  "\tsd ra, 8(sp)\n"
                  "\tsd fp, 0(sp)\n"
                  "\tmv fp, sp\n"
                  "\tla ra, _frameSize_%s\n"
                  "\tlw ra, 0(ra)\n"
                  "\tsub sp, sp, ra\n", funcName);
    storeCalleeSavedRegisters();
}

void genEpilogue(char* funcName)
{   
    fprintf(fout, "_end_%s:\n", funcName);
    restoreCalleeSavedRegisters();
    fprintf(fout, "\tld ra, 8(fp)\n"
                  "\taddi sp, fp, -8\n"
                  "\tld fp, 0(fp)\n"
                  "\tjr ra\n");
}

void genFuncDecl(AST_NODE* declarationNode)
{
    AST_NODE *returnTypeNode = declarationNode->child;
    AST_NODE *funcNameNode = returnTypeNode->rightSibling;
    AST_NODE *paramListNode = funcNameNode->rightSibling;
    AST_NODE *blockNode = paramListNode->rightSibling;
    char *funcName = funcNameNode->semantic_value.identifierSemanticValue.identifierName;
    
    initReg();
    initStack();

    genFuncHead(funcName);
    genPrologue(funcName);
    
    char endLabel[128];
    snprintf(endLabel, 128, "_end_%s", funcName);
    genBlockNode(blockNode, endLabel);

    fprintf(fout, "\tj %s\n", endLabel);
    genEpilogue(funcName);

    long long frameSize = getFrameSize();
    fprintf(fout, ".data\n"
                  "_frameSize_%s: .word %lld\n", funcName, frameSize);
}

void genBlockNode(AST_NODE* blockNode, char* endLabel)
{
    AST_NODE* node = blockNode->child;
    while (node) {
        genGeneralNode(node, endLabel);
        node = node->rightSibling;
    }
}

void genStmtNode(AST_NODE* stmtNode, char* endLabel)
{
    if (stmtNode->nodeType == NUL_NODE)
        return;
    if (stmtNode->nodeType == BLOCK_NODE)
        return genBlockNode(stmtNode, endLabel);

    switch (stmtNode->semantic_value.stmtSemanticValue.kind) {
        case WHILE_STMT:
            genWhileStmt(stmtNode, endLabel);
            break;
        case FOR_STMT:
            genForStmt(stmtNode, endLabel);
            break;
        case ASSIGN_STMT:
            genAssignmentStmt(stmtNode);
            break;
        case IF_STMT:
            genIfStmt(stmtNode, endLabel);
            break;
        case FUNCTION_CALL_STMT:
            genFunctionCall(stmtNode);
            break;
        case RETURN_STMT:
            genReturnStmt(stmtNode, endLabel);
            break;
    }
}

void genWhileStmt(AST_NODE* whileNode, char* endLabel)
{
    int labelNo = getLabel();
    fprintf(fout, "_Test%d:\n", labelNo);
    // genAssignOrExpr(whileNode);
    genStmtNode(whileNode->child->rightSibling, endLabel);
}

void genForStmt(AST_NODE* stmtNode, char* endLabel)
{
    
}

void genIfStmt(AST_NODE* stmtNode, char* endLabel)
{
    
}

void genAssignmentStmt(AST_NODE* stmtNode)
{
    
}

void genFunctionCall(AST_NODE* stmtNode)
{

}

void genReturnStmt(AST_NODE* stmtNode, char* endLabel)
{

}

/******************************
 * Register Management
 ******************************/

int temporaryRegisters[] = {5, 6, 7, 28, 29, 30, 31};
int savedRegisters[] = {9, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27};
int allocatableRegisters[] = {9, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 5, 6, 7, 28, 29, 30, 31};
int argumentRegisters[] = {10, 11, 12, 13, 14, 15, 16, 17};

int floatTemporaryRegisters[] = {26, 27, 28, 29, 30, 31};
int floatSavedRegisters[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
int floatAllocatableRegisters[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 26, 27, 28, 29, 30, 31};
int floatArgumentRegisters[] = {18, 19, 20, 21, 22, 23, 24, 25};

bool regAvailable[32], floatRegAvailable[32];   // if a register is available

void initReg()
{
    int numReg = sizeof(allocatableRegisters) / sizeof(allocatableRegisters[0]);
    for (int i = 0; i < numReg; i++) {
        int reg = allocatableRegisters[i];
        regAvailable[reg] = true;
    }

    numReg = sizeof(floatAllocatableRegisters) / sizeof(floatAllocatableRegisters[0]);
    for (int i = 0; i < numReg; i++) {
        int reg = floatAllocatableRegisters[i];
        floatRegAvailable[reg] = true;
    }
}

int getReg(char type)
{
    if (type == 'i') {
        int numReg = sizeof(allocatableRegisters) / sizeof(allocatableRegisters[0]);
        for (int i = 0; i < numReg; i++) {
            int reg = allocatableRegisters[i];
            if (regAvailable[reg]) {
                regAvailable[reg] = false;
                return reg;
            }
        }
        fprintf(stderr, "Error: Out of Registers\n");
        exit(1);
    } else if (type == 'f') {
        int numReg = sizeof(floatAllocatableRegisters) / sizeof(floatAllocatableRegisters[0]);
        for (int i = 0; i < numReg; i++) {
            int reg = floatAllocatableRegisters[i];
            if (floatRegAvailable[reg]) {
                floatRegAvailable[reg] = false;
                return reg;
            }
        }
        fprintf(stderr, "Error: Out of Registers\n");
        exit(1);
    } else {
        fprintf(stderr, "Invalid type in getReg\n");
        exit(1);
    }
}

void freeReg(int reg, char type)
{
    if (type == 'i') {
        regAvailable[reg] = true;
    } else if (type == 'f') {
        floatRegAvailable[reg] = true;
    } else {
        fprintf(stderr, "Invalid type in freeReg\n");
        exit(1);
    }
}

long long savedRegOffset[32], floatSavedRegOffset[32];
void storeCalleeSavedRegisters()
{
    int numReg = sizeof(savedRegisters) / sizeof(savedRegisters[0]);
    for (int i = 0; i < numReg; i++) {
        int reg = savedRegisters[i];
        savedRegOffset[reg] = pushStack(8);
        fprintf(fout, "\tsd x%d, -%lld(fp)\n", reg, savedRegOffset[reg]);
    }
    numReg = sizeof(floatSavedRegisters) / sizeof(floatSavedRegisters[0]);
    for (int i = 0; i < numReg; i++) {
        int reg = floatSavedRegisters[i];
        floatSavedRegOffset[reg] = pushStack(8);
        fprintf(fout, "\tfsd f%d, -%lld(fp)\n", reg, floatSavedRegOffset[reg]);
    }
}

void restoreCalleeSavedRegisters()
{
    int numReg = sizeof(savedRegisters) / sizeof(savedRegisters[0]);
    for (int i = 0; i < numReg; i++) {
        int reg = savedRegisters[i];
        fprintf(fout, "\tld x%d, -%lld(fp)\n", reg, savedRegOffset[reg]);
    }
    numReg = sizeof(floatSavedRegisters) / sizeof(floatSavedRegisters[0]);
    for (int i = 0; i < numReg; i++) {
        int reg = floatSavedRegisters[i];
        fprintf(fout, "\tfld f%d, -%lld(fp)\n", reg, floatSavedRegOffset[reg]);
    }
}

/******************************
 * Stack Management
 ******************************/

long long curFrameSize;

void initStack()
{
    curFrameSize = 0;
}

long long pushStack(long long size)
{
    curFrameSize += size;
    return curFrameSize;
}

long long getFrameSize()
{
    return curFrameSize;
}

/******************************
 * Label Management
 ******************************/
int getLabel()
{
    static int labelNo = 0;
    return labelNo++;
}
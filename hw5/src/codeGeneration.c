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
            genGeneralNode(globalDeclarationNode);
        } else if (nodeType == DECLARATION_NODE) { // function declaration
            genDeclarationNode(globalDeclarationNode);
        }
        globalDeclarationNode = globalDeclarationNode->rightSibling;
    }
}

void genGeneralNode(AST_NODE* node)
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
                //genStmtNode(childNode);
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

void genPrologue(char* funcName)
{
    fprintf(fout, "addi sp, sp, -16\n"
                  "sd ra, 8(sp)\n"
                  "sd fp, 0(sp)\n"
                  "mv fp, sp\n"
                  "la ra, _frameSize_%s\n"
                  "lw ra, 0(ra)\n"
                  "sub sp, sp, ra\n", funcName);
    storeCalleeSavedRegisters();
}

void genEpilogue(char* funcName)
{   
    fprintf(fout, "_end_%s:\n", funcName);
    restoreCalleeSavedRegisters();
    fprintf(fout, "ld ra, 8(fp)\n"
                  "addi sp, fp, -8\n"
                  "ld fp, 0(fp)\n"
                  "jr ra\n");
}

void genFuncDecl(AST_NODE* declarationNode)
{
    AST_NODE *returnTypeNode = declarationNode->child;
    AST_NODE *funcNameNode = returnTypeNode->rightSibling;
    AST_NODE *paramListNode = funcNameNode->rightSibling;
    AST_NODE *blockNode = paramListNode->rightSibling;
    char *funcName = funcNameNode->semantic_value.identifierSemanticValue.identifierName;

    genFuncHead(funcName);
    genPrologue(funcName);

    initReg();
    initStack();
    
    genBlockNode(blockNode);

    fprintf(fout, "j _end_%s\n", funcName);
    genEpilogue(funcName);

    long long frameSize = getFrameSize();
    fprintf(fout, ".data\n"
                  "_frameSize_%s: .word %lld\n", funcName, frameSize);
}

void genBlockNode(AST_NODE* blockNode)
{
    AST_NODE* node = blockNode->child;
    while (node) {
        genGeneralNode(node);
        node = node->rightSibling;
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

void initReg()
{
    int numReg = sizeof(allocatableRegisters) / sizeof(allocatableRegisters[0]);
    for (int i = 0; i < numReg; i++) {
        int reg = allocatableRegisters[i];
        regAvailable[reg] = true;
    }
}

int getReg()
{
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
}

void freeReg(int reg)
{
    regAvailable[reg] = true;
}

long long savedRegOffset[32];
void storeCalleeSavedRegisters()
{
    int numReg = sizeof(savedRegisters) / sizeof(savedRegisters[0]);
    for (int i = 0; i < numReg; i++) {
        int reg = savedRegisters[i];
        savedRegOffset[reg] = pushStack(8);
        fprintf(fout, "sd x%d, -%lld(fp)\n", reg, savedRegOffset[reg]);
    }
}

void restoreCalleeSavedRegisters()
{
    int numReg = sizeof(savedRegisters) / sizeof(savedRegisters[0]);
    for (int i = 0; i < numReg; i++) {
        int reg = savedRegisters[i];
        fprintf(fout, "ld x%d, -%lld(fp)\n", reg, savedRegOffset[reg]);
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

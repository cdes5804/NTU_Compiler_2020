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
                genAssignOrExpr(childNode);
                childNode = childNode->rightSibling;
            }
            break;
        case NONEMPTY_RELOP_EXPR_LIST_NODE:
            while (childNode) {
                genExprRelatedNode(childNode);
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

void genGlobalVar(AST_NODE* idNode, SymbolTableEntry* symtabEntry)
{
    char label[128];
    snprintf(label, 128, "_%d", getLabel());
    symtabEntry->globalLabel = strdup(label);
    fprintf(fout, ".data\n");
    fprintf(fout, "%s: .word ", label);
    if (isArrayId(idNode)) {
        int numElement = getSymbolSize(symtabEntry);
        for (int i = 0; i < numElement; i++)
            fprintf(fout, "0 ");
        fprintf(fout, "\n");
    } else {
        if (idNode->semantic_value.identifierSemanticValue.kind == WITH_INIT_ID) {
            if (idNode->dataType == FLOAT_TYPE)
                fprintf(fout, "%u\n", getFloatRepr(idNode->child->semantic_value.exprSemanticValue.constEvalValue.fValue));
            else
                fprintf(fout, "%d\n", idNode->child->semantic_value.exprSemanticValue.constEvalValue.iValue);
        } else {
            fprintf(fout, "0\n");
        }
    }
    fprintf(fout, ".text\n");
}

void genLocalVar(AST_NODE* idNode, SymbolTableEntry* symtabEntry)
{
    int size = 4 * getSymbolSize(symtabEntry);
    idNode->offset = symtabEntry->offset = allocFrame(size);
    if (idNode->semantic_value.identifierSemanticValue.kind == WITH_INIT_ID) {
        genExprRelatedNode(idNode->child);
        AssignNode(idNode, idNode->child);
    }
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
            genGlobalVar(idNode, symtabEntry);
        } else {
            genLocalVar(idNode, symtabEntry);
        }
    }
}

void genFuncHead(char* funcName)
{
    fprintf(fout, ".text\n"
                  ".global %s\n"
                  "%s:\n", funcName, funcName);
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
    initFrameSize();

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
    genAssignOrExpr(whileNode);
    genStmtNode(whileNode->child->rightSibling, endLabel);
}

void genAssignOrExpr(AST_NODE* node)
{
    if (node->nodeType == STMT_NODE) {
        switch (node->semantic_value.stmtSemanticValue.kind) {
            case ASSIGN_STMT:
                genAssignmentStmt(node);
                break;
            case FUNCTION_CALL_STMT:
                genFunctionCall(node);
                break;
        }
    } else {
        genExprRelatedNode(node);
    }
}

void genForStmt(AST_NODE* stmtNode, char* endLabel)
{
    
}

void genIfStmt(AST_NODE* stmtNode, char* endLabel)
{
    AST_NODE* boolExprNode = stmtNode->child;
    AST_NODE* ifBodyNode = boolExprNode->rightSibling;
    AST_NODE* elseBodyNode = ifBodyNode->rightSibling;

    genAssignOrExpr(boolExprNode);

    switch (boolExprNode->dataType) {
        case INT_TYPE:
            int reg = getReg('i');
            loadNode(boolExprNode, reg);
            freeReg(reg, 'i');
            break;
        case FLOAT_TYPE:
            break;
        default:
            fprintf(stderr, "genIfStmt: Invalid boolExprNode->dataType\n");
            break;
    }
    genStmtNode(ifBodyNode);
    genStmtNode(elseBodyNode);
}

void genAssignmentStmt(AST_NODE* stmtNode)
{
    AST_NODE* idNode = stmtNode->child;
    AST_NODE* exprNode = idNode->rightSibling;

    genVariable(idNode);
    genExprRelated(exprNode);

    AssignNode(idNode, exprNode);
}

void genFunctionCall(AST_NODE* stmtNode)
{

}

void genReturnStmt(AST_NODE* stmtNode, char* endLabel)
{

}

void genExprRelatedNode(AST_NODE* exprRelatedNode)
{
    switch (exprRelatedNode->nodeType) {
        case EXPR_NODE:
            genExprNode(exprRelatedNode);
            break;
        case STMT_NODE:
            genFunctionCall(exprRelatedNode);
            break;
        case IDENTIFIER_NODE:
            genVariable(exprRelatedNode);
            break;
        case CONST_VALUE_NODE:
            genConst(exprRelatedNode);
            break;
    }
}

void genVariable(AST_NODE* idNode)
{
    idNode->offset = getSymtabEntry(idNode)->offset;
}

void genExprNode(AST_NODE* exprNode)
{

}

void genConst(AST_NODE* constNode)
{
    char label[128];
    switch (constNode->dataType) {
        case INT_TYPE:
            break;
        case FLOAT_TYPE:
            snprintf(label, 128, "_%d", getLabel());
            constNode->globalLabel = strdup(label);
            fprintf(fout, ".data\n");
            fprintf(fout, "%s: .word %u\n", label,
                    getFloatRepr(constNode->semantic_value.const1->const_u.fval));
            break;
        case CONST_STRING_TYPE:
            snprintf(label, 128, "_%d", getLabel());
            constNode->globalLabel = strdup(label);
            fprintf(fout, ".data\n"
                          ".align 3\n");
            fprintf(fout, "%s: .string \"%s\"\n", label,
                    constNode->semantic_value.const1->const_u.sc);
            break;
    }
    fprintf(fout, ".text\n");
}
void genWriteCall(AST_NODE* paramListNode)
{
    AST_NODE* paramNode = paramListNode->child;
    genExprRelated(paramNode);
    int reg = getReg('i');
    switch (paramNode->dataType) {
        case INT_TYPE:
            fprintf(fout, "lw a0, some memory location");
            fprintf(fout, "la %d, _write_int", reg);
            fprintf(fout, "call %d", reg);
            break;
        case FLOAT_TYPE:
            fprintf(fout, "flw fa0, some memory location");
            fprintf(fout, "la %d, _write_float", reg);
            fprintf(fout, "call %d", reg);
            break;
        case CONST_STRING_TYPE:
            fprintf(fout, "lw a0, some memory location");
            fprintf(fout, "la %d, _write_str", reg);
            fprintf(fout, "call %d", reg);
            break;
        default:
            break;
    }
}

void genReadCall()
{

}

void genFredCall()
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
        savedRegOffset[reg] = allocFrame(8);
        fprintf(fout, "\tsd x%d, -%lld(fp)\n", reg, savedRegOffset[reg]);
    }
    numReg = sizeof(floatSavedRegisters) / sizeof(floatSavedRegisters[0]);
    for (int i = 0; i < numReg; i++) {
        int reg = floatSavedRegisters[i];
        floatSavedRegOffset[reg] = allocFrame(8);
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

void storeCallerSavedRegisters()
{
    int numReg = sizeof(temporaryRegisters) / sizeof(temporaryRegisters[0]);
    for (int i = 0; i < numReg; i++) {
        int reg = temporaryRegisters[i];
        savedRegOffset[reg] = allocFrame(8);
        fprintf(fout, "\tsd x%d, -%lld(fp)\n", reg, savedRegOffset[reg]);
    }
    numReg = sizeof(floatTemporaryRegisters) / sizeof(floatTemporaryRegisters[0]);
    for (int i = 0; i < numReg; i++) {
        int reg = floatTemporaryRegisters[i];
        floatSavedRegOffset[reg] = allocFrame(8);
        fprintf(fout, "\tfsd f%d, -%lld(fp)\n", reg, floatSavedRegOffset[reg]);
    }
}

void restoreCallerSavedRegisters()
{
    int numReg = sizeof(temporaryRegisters) / sizeof(temporaryRegisters[0]);
    for (int i = 0; i < numReg; i++) {
        int reg = temporaryRegisters[i];
        fprintf(fout, "\tld x%d, -%lld(fp)\n", reg, temporaryRegOffset[reg]);
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

void initFrameSize()
{
    curFrameSize = 0;
}

long long allocFrame(long long size)
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

/******************************
 * Type Conversion
 ******************************/
void typeConversion(AST_NODE* node, DATA_TYPE targetType)
{
    if (node->dataType == targetType)
        return;
    
    int intReg = getReg('i');
    int floatReg = getReg('f');
    switch (node->dataType) {
        case INT_TYPE:
            loadNode(node, intReg);
            fprintf(fout, "\tfcvt.s.w f%d, x%d\n", floatReg, intReg);
            node->dataType = FLOAT_TYPE;
            storeNode(node, floatReg);
            break;
        case FLOAT_TYPE:
            loadNode(node, floatReg);
            fprintf(fout, "\tfcvt.w.s x%d, f%d\n", intReg, floatReg);
            node->dataType = INT_TYPE;
            storeNode(node, intReg);
            break;
        default:
            fprintf(stderr, "typeConversion: only support int-float conversion\n");
            exit(1);
    }
    freeReg(intReg, 'i');
    freeReg(floatReg, 'f');
}

void storeNode(AST_NODE* node, int reg)
{   
    /* the data type of reg and node should match */
    /* node->offset should be the symbol offset if node is a local variable reference */
    int addrReg = getReg('i');

    // load the (base) address of this node
    if (isGlobalId(node)) {
        fprintf(fout, "\tla x%d, %s\n", addrReg, getSymtabEntry(node)->globalLabel);
    } else if (node->nodeType == IDENTIFIER_NODE) {  // local variable
        fprintf(fout, "\taddi x%d, fp, -%lld\n", addrReg, getSymtabEntry(node)->offset);
    } else {
        fprintf(fout, "\taddi x%d, fp, -%lld\n", addrReg, node->offset);
    }

    // if this node is an array reference, calculate the address of the referenced element
    if (isArrayId(node)) {
        int arrayOffsetReg = getReg('i');
        loadNode(node->child, arrayOffsetReg);
        fprintf(fout, "\tadd x%d, x%d, x%d\n", addrReg, addrReg, arrayOffsetReg);
        freeReg(arrayOffsetReg, 'i');
    }

    switch (node->dataType) {
        case INT_TYPE:
            fprintf(fout, "\tsw x%d, 0(x%d)\n", reg, addrReg);
            break;
        case FLOAT_TYPE:
            fprintf(fout, "\tfsw f%d, 0(x%d)\n", reg, addrReg);
            break;
        case INT_PTR_TYPE:
        case FLOAT_PTR_TYPE:
            fprintf(fout, "\tsd, x%d, 0(x%d)\n", reg, addrReg);
            break;
        default:
            fprintf(stderr, "storeNode: Invalid data type\n");
            exit(1);
    }
}

void loadNode(AST_NODE* node, int reg)
{
    /* node->offset should be the symbol offset if node is a local variable reference */
    int addrReg = -1;
    if (node->dataType == FLOAT_TYPE) {
        addrReg = getReg('f');
    } else {
        addrReg = getReg('i');
    }
    
    // load the (base) address of this node
    if (isGlobalId(node)) {
        fprintf(fout, "\tla, x%d, %s\n", addrReg, getSymtabEntry(node)->globalLabel);
    } else if (node->nodeType == CONST_VALUE_NODE) {
        loadConstantNode(node, reg);
        return;
    } else {
        fprintf(fout, "\taddi x%d, fp, -%lld\n", addrReg, node->offset);
    }

    // if this node is an array reference, calculate the address of the referenced element
    if (isArrayId(node)) {
        int arrayOffsetReg = getReg('i');
        loadNode(node->child, arrayOffsetReg);
        fprintf(fout, "\tadd x%d, x%d, x%d\n", addrReg, addrReg, arrayOffsetReg);
        freeReg(arrayOffsetReg, 'i');
    }

    switch (node->dataType) {
        case INT_TYPE:
            fprintf(fout, "\tlw x%d, 0(x%d)\n", reg, addrReg);
            break;
        case FLOAT_TYPE:
            fprintf(fout, "\tflw f%d, 0(x%d)\n", reg, addrReg);
            break;
        case INT_PTR_TYPE:
        case FLOAT_PTR_TYPE:
            fprintf(fout, "\tld, x%d, 0(x%d)\n", reg, addrReg);
            break;
        default:
            fprintf(stderr, "loadNode: Invalid data type\n");
            exit(1);
    }
}

void loadConstantNode(AST_NODE* constNode, int reg)
{
    int tmpReg = -1;
    switch (constNode->dataType) {
        case INT_TYPE:
            fprintf(fout, "\tli x%d, %d\n", reg,
                    constNode->semantic_value.const1->const_u.intval);
            break;
        case FLOAT_TYPE:
            tmpReg = getReg('i');
            fprintf(fout, "\tla x%d, %s\n", tmpReg, constNode->globalLabel);
            fprintf(fout, "\tflw f%d, 0(x%d)\n", reg, tmpReg);
            freeReg(tmpReg, 'i');
            break;
        case CONST_STRING_TYPE:
            fprintf(fout, "\tla x%d, %s\n", reg, constNode->globalLabel);
            break;
    }
}

void AssignNode(AST_NODE* dst, AST_NODE* src)
{
    if (src->dataType != dst->dataType)
        typeConversion(src, dst->dataType);
    if (dst->dataType == FLOAT_TYPE) {
        int tmpReg = getReg('f');
        loadNode(src, tmpReg);
        storeNode(dst, tmpReg);
        freeReg(tmpReg, 'f');
    } else {
        int tmpReg = getReg('i');
        loadNode(src, tmpReg);
        storeNode(dst, tmpReg);
        freeReg(tmpReg, 'i');
    }
}
/******************
 * Utility
 ******************/
SymbolTableEntry* getSymtabEntry(AST_NODE* idNode)
{
    return idNode->semantic_value.identifierSemanticValue.symbolTableEntry;
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

bool isPtrType(AST_NODE* node)
{
    return node->dataType == INT_PTR_TYPE || node->dataType == FLOAT_PTR_TYPE;
}

bool isGlobalId(AST_NODE* node)
{
    return node->nodeType == IDENTIFIER_NODE &&
           getSymtabEntry(node)->nestingLevel == 0;
}

bool isArrayId(AST_NODE* node)
{
    return node->nodeType == IDENTIFIER_NODE &&
           node->semantic_value.identifierSemanticValue.kind == ARRAY_ID;
}

unsigned getFloatRepr(float f)
{
    union { unsigned uu; float ff; } tmp;
    tmp.ff = f;
    return tmp.uu;
}
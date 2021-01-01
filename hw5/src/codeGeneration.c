#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "header.h"
#include "symbolTable.h"
#include "codeGeneration.h"

#define SYS_ERR_EXIT(msg) { perror(msg); exit(127); }
DATA_TYPE getBiggerType(DATA_TYPE dataType1, DATA_TYPE dataType2);

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
    snprintf(label, 128, ".GlobalVar%d", getLabel());
    symtabEntry->globalLabel = strdup(label);
    fprintf(fout, ".data\n");
    fprintf(fout, "%s: .word ", label);
    if (isArrayId(idNode)) {
        int numElement = getSymbolSize(symtabEntry);
        for (int i = 0; i < numElement; i++)
            fprintf(fout, "0%c", ",\n"[i == numElement-1]);
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
                  "_start_%s:\n", funcName, funcName);
}

char* genPrologue(char* funcName)
{
    fprintf(fout, "\taddi sp, sp, -16\n"
                  "\tsd ra, 8(sp)\n"
                  "\tsd fp, 0(sp)\n"
                  "\taddi fp, sp, 8\n"
                  "\tla ra, _frameSize_%s\n"
                  "\tlw ra, 0(ra)\n"
                  "\tsub sp, sp, ra\n", funcName);
    storeCalleeSavedRegisters();
}

void genEpilogue(char* funcName)
{   
    fprintf(fout, "_end_%s:\n", funcName);
    restoreCalleeSavedRegisters();
    fprintf(fout, "\tld ra, 0(fp)\n"
                  "\taddi sp, fp, 8\n"
                  "\tld fp, -8(fp)\n"
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
    AST_NODE *testNode = whileNode->child;
    AST_NODE *whileBodyNode = testNode->rightSibling;

    int label = getLabel();
    fprintf(fout, ".whileTest%d:\n", label);

    genAssignOrExpr(testNode);
    int boolReg = getReg('i');
    if (testNode->dataType == FLOAT_TYPE) {   
        int testReg = getReg('f');
        int zeroReg = getReg('f');
        loadNode(testNode, testReg);
        fprintf(fout, "\tfmv.w.x f%d, zero\n", zeroReg);
        fprintf(fout, "\tfeq.s x%d, f%d, f%d\n", boolReg, zeroReg, testReg);
        freeReg(testReg, 'f');
        freeReg(zeroReg, 'f');
    } else {
        loadNode(testNode, boolReg);
    }
    fprintf(fout, "\tbeqz x%d, .whileExit%d\n", boolReg, label);
    freeReg(boolReg, 'i');
    genStmtNode(whileBodyNode, endLabel);
    fprintf(fout, "\tj .whileTest%d\n", label);
    fprintf(fout, ".whileExit%d:\n", label);
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
    if (elseBodyNode->nodeType != NUL_NODE) {
        genIfElseStmt(stmtNode, endLabel);
        return;
    }
    genAssignOrExpr(boolExprNode);

    int boolReg = getReg('i');
    if (boolExprNode->dataType == FLOAT_TYPE) {   
        int testReg = getReg('f');
        int zeroReg = getReg('f');
        loadNode(boolExprNode, testReg);
        fprintf(fout, "\tfmv.w.x f%d, zero\n", zeroReg);
        fprintf(fout, "\tfeq.s x%d, f%d, f%d\n", boolReg, zeroReg, testReg);
        freeReg(testReg, 'f');
        freeReg(zeroReg, 'f');
    } else {
        loadNode(boolExprNode, boolReg);
    }

    int label = getLabel();
    fprintf(fout, "\tbeqz x%d, .ifExit%d\n", boolReg, label);
    freeReg(boolReg, 'i');

    genStmtNode(ifBodyNode, endLabel);
    fprintf(fout, ".ifExit%d:\n", label);
}

void genIfElseStmt(AST_NODE* stmtNode, char* endLabel)
{
    AST_NODE* boolExprNode = stmtNode->child;
    AST_NODE* ifBodyNode = boolExprNode->rightSibling;
    AST_NODE* elseBodyNode = ifBodyNode->rightSibling;

    genAssignOrExpr(boolExprNode);

    int boolReg = getReg('i');
    if (boolExprNode->dataType == FLOAT_TYPE) {   
        int testReg = getReg('f');
        int zeroReg = getReg('f');
        loadNode(boolExprNode, testReg);
        fprintf(fout, "\tfmv.w.x f%d, zero\n", zeroReg);
        fprintf(fout, "\tfeq.s x%d, f%d, f%d\n", boolReg, zeroReg, testReg);
        freeReg(testReg, 'f');
        freeReg(zeroReg, 'f');
    } else {
        loadNode(boolExprNode, boolReg);
    }

    int label = getLabel();
    fprintf(fout, "\tbeqz x%d, .ifElse%d\n", boolReg, label);
    freeReg(boolReg, 'i');
    
    genStmtNode(ifBodyNode, endLabel);
    fprintf(fout, "\tj .ifExit%d\n", label);

    fprintf(fout, ".ifElse%d:\n", label);
    genStmtNode(elseBodyNode, endLabel);
    fprintf(fout, ".ifExit%d:\n", label);
}

void genAssignmentStmt(AST_NODE* stmtNode)
{
    AST_NODE* idNode = stmtNode->child;
    AST_NODE* exprNode = idNode->rightSibling;

    genVariable(idNode);
    genExprRelatedNode(exprNode);

    AssignNode(idNode, exprNode);
}

void genFunctionCall(AST_NODE* stmtNode)
{
    AST_NODE* funcNameNode = stmtNode->child;
    char* funcName = funcNameNode->semantic_value.identifierSemanticValue.identifierName;
    AST_NODE* paramListNode = funcNameNode->rightSibling;
    SymbolTableEntry* symtab_entry = retrieveSymbol(funcName);
    DATA_TYPE returnType = symtab_entry ? symtab_entry->attribute->attr.functionSignature->returnType : NONE_TYPE;
    bool hasReturnValue = returnType == INT_TYPE || returnType == FLOAT_TYPE;

    storeCallerSavedRegisters();

    if (strcmp(funcName, "write") == 0) {
        genWrite(paramListNode);
    } else if (strcmp(funcName, "read") == 0) {
        fprintf(fout, "\tcall _read_int\n");
    } else if (strcmp(funcName, "fread") == 0) {
        fprintf(fout, "\tcall _read_float\n");
    } else {
        fprintf(fout, "\tcall _start_%s\n", funcName);
    }
    if (hasReturnValue) {
        stmtNode->offset = allocFrame(4);
        int reg = 10;
        storeNode(stmtNode, reg);
    }

    restoreCallerSavedRegisters();
}

void genWrite(AST_NODE* paramListNode)
{
    AST_NODE* paramNode = paramListNode->child;
    genExprRelatedNode(paramNode);
    int reg = 10;
    loadNode(paramNode, reg);
    switch (paramNode->dataType) {
        case INT_TYPE:
            fprintf(fout, "\tcall _write_int\n");
            break;
        case FLOAT_TYPE:
            fprintf(fout, "\tcall _write_float\n");
            break;
        case CONST_STRING_TYPE:
            fprintf(fout, "\tcall _write_str\n");
            break;
        default:
            break;
    }
}

void genReturnStmt(AST_NODE* stmtNode, char* endLabel)
{
    int reg = 10;
    AST_NODE* returnExprNode = stmtNode->child;
    genExprRelatedNode(returnExprNode);
    loadNode(returnExprNode, reg);
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
    exprNode->offset = allocFrame(4);
    AST_NODE *leftOperand, *rightOperand;
    switch (exprNode->semantic_value.exprSemanticValue.kind) {
        case BINARY_OPERATION:
            leftOperand = exprNode->child;
            rightOperand = leftOperand->rightSibling;
            genExprRelatedNode(leftOperand);
            genExprRelatedNode(rightOperand);
            
            // special case: short-circuit && ||
            if (exprNode->semantic_value.exprSemanticValue.op.binaryOp == BINARY_OP_AND) {
                genLogicalAnd(exprNode, leftOperand, rightOperand);
                break;
            } else if (exprNode->semantic_value.exprSemanticValue.op.binaryOp == BINARY_OP_OR) {
                genLogicalOr(exprNode, leftOperand, rightOperand);
                break;
            }

            // If op is comparison or logical, then expr->dataType may not == biggerType
            DATA_TYPE biggerType = getBiggerType(leftOperand->dataType, rightOperand->dataType);
            typeConversion(leftOperand, biggerType);
            typeConversion(rightOperand, biggerType);
            switch (biggerType) {
                case INT_TYPE:
                    genBinaryOpInt(exprNode, leftOperand, rightOperand);
                    break;
                case FLOAT_TYPE:
                    genBinaryOpFloat(exprNode, leftOperand, rightOperand);
                    break;
                default:
                    fprintf(stderr, "genExprNode: Invalid data type\n");
                    exit(1);
            }
            break;
        case UNARY_OPERATION:
            leftOperand = exprNode->child;
            genExprRelatedNode(leftOperand);
            // if op is logical not, then exprNode->dataType != leftOperand->dataType
            switch (leftOperand->dataType) {
                case INT_TYPE:
                    genUnaryOpInt(exprNode, leftOperand);
                    break;
                case FLOAT_TYPE:
                    genUnaryOpFloat(exprNode, leftOperand);
                    break;
                default:
                    fprintf(stderr, "genExprNode: Invalid data type\n");
                    exit(1);
            }
            break;
    }
}

void genBinaryOpInt(AST_NODE* exprNode, AST_NODE* leftOperand, AST_NODE* rightOperand)
{
    int lhsReg = getReg('i');
    int rhsReg = getReg('i');
    int resultReg = getReg('i');
    loadNode(leftOperand, lhsReg);
    loadNode(rightOperand, rhsReg);
    switch (exprNode->semantic_value.exprSemanticValue.op.binaryOp) {
        case BINARY_OP_ADD:
            fprintf(fout, "\tadd x%d, x%d, x%d\n", resultReg, lhsReg, rhsReg);
            break;
        case BINARY_OP_SUB:
            fprintf(fout, "\tsub x%d, x%d, x%d\n", resultReg, lhsReg, rhsReg);
            break;
        case BINARY_OP_MUL:
            fprintf(fout, "\tmul x%d, x%d, x%d\n", resultReg, lhsReg, rhsReg);
            break;
        case BINARY_OP_DIV:
            fprintf(fout, "\tdiv x%d, x%d, x%d\n", resultReg, lhsReg, rhsReg);
            break;
        case BINARY_OP_EQ:
            fprintf(fout, "\tsub x%d, x%d, x%d\n", resultReg, lhsReg, rhsReg);
            fprintf(fout, "\tseqz x%d, x%d\n", resultReg, resultReg);
            break;
        case BINARY_OP_GE:
            // lhs >= rhs => !(lhs < rhs)
            fprintf(fout, "\tsub x%d, x%d, x%d\n", resultReg, lhsReg, rhsReg);
            fprintf(fout, "\tsltz x%d, x%d\n", resultReg, resultReg);
            fprintf(fout, "\tnot x%d, x%d\n", resultReg, resultReg);
            break;
        case BINARY_OP_LE:
            // lhs <= rhs => !(lhs > rhs)
            fprintf(fout, "\tsub x%d, x%d, x%d\n", resultReg, lhsReg, rhsReg);
            fprintf(fout, "\tsgtz x%d, x%d\n", resultReg, resultReg);
            fprintf(fout, "\tnot x%d, x%d\n", resultReg, resultReg);
            break;
        case BINARY_OP_NE:
            fprintf(fout, "\tsub x%d, x%d, x%d\n", resultReg, lhsReg, rhsReg);
            fprintf(fout, "\tseqz x%d, x%d\n", resultReg, resultReg);
            fprintf(fout, "\tnot x%d, x%d\n", resultReg, resultReg);
            break;
        case BINARY_OP_GT:
            fprintf(fout, "\tsub x%d, x%d, x%d\n", resultReg, lhsReg, rhsReg);
            fprintf(fout, "\tsgtz x%d, x%d\n", resultReg, resultReg);
            break;
        case BINARY_OP_LT:
            fprintf(fout, "\tsub x%d, x%d, x%d\n", resultReg, lhsReg, rhsReg);
            fprintf(fout, "\tsltz x%d, x%d\n", resultReg, resultReg);
            break;
    }
    storeNode(exprNode, resultReg);
    freeReg(lhsReg, 'i');
    freeReg(rhsReg, 'i');
    freeReg(resultReg, 'i');
}

void genBinaryOpFloat(AST_NODE* exprNode, AST_NODE* leftOperand, AST_NODE* rightOperand)
{
    int lhsReg = getReg('f');
    int rhsReg = getReg('f');
    int resultReg = getReg('f');
    int intResultReg = getReg('i');  // for comparison op
    loadNode(leftOperand, lhsReg);
    loadNode(rightOperand, rhsReg);
    switch (exprNode->semantic_value.exprSemanticValue.op.binaryOp) {
        case BINARY_OP_ADD:
            fprintf(fout, "\tfadd.s f%d, f%d, f%d\n", resultReg, lhsReg, rhsReg);
            break;
        case BINARY_OP_SUB:
            fprintf(fout, "\tfsub.s f%d, f%d, f%d\n", resultReg, lhsReg, rhsReg);
            break;
        case BINARY_OP_MUL:
            fprintf(fout, "\tfmul.s f%d, f%d, f%d\n", resultReg, lhsReg, rhsReg);
            break;
        case BINARY_OP_DIV:
            fprintf(fout, "\tfdiv.s f%d, f%d, f%d\n", resultReg, lhsReg, rhsReg);
            break;
        case BINARY_OP_EQ:
            fprintf(fout, "\tfeq.s x%d, f%d, f%d\n", intResultReg, lhsReg, rhsReg);
            break;
        case BINARY_OP_GE:
            // lhs >= rhs => !(lhs < rhs)
            fprintf(fout, "\tflt.s x%d, f%d, f%d\n", intResultReg, lhsReg, rhsReg);
            fprintf(fout, "\tnot x%d, x%d\n", intResultReg, intResultReg);
            break;
        case BINARY_OP_LE:
            fprintf(fout, "\tfle.s x%d, f%d, f%d\n", intResultReg, lhsReg, rhsReg);
            break;
        case BINARY_OP_NE:
            fprintf(fout, "\tfeq.s x%d, f%d, f%d\n", intResultReg, lhsReg, rhsReg);
            fprintf(fout, "\tnot x%d, x%d\n", intResultReg, intResultReg);
            break;
        case BINARY_OP_GT:
            // lhs > rhs => !(lhs <= rhs)
            fprintf(fout, "\tfle.s x%d, f%d, f%d\n", intResultReg, lhsReg, rhsReg);
            fprintf(fout, "\tnot x%d, x%d\n", intResultReg, intResultReg);
            break;
        case BINARY_OP_LT:
            fprintf(fout, "\tflt.s x%d, f%d, f%d\n", intResultReg, lhsReg, rhsReg);
            break;
    }
    if (exprNode->dataType == INT_TYPE)
        storeNode(exprNode, intResultReg);
    else
        storeNode(exprNode, resultReg);
    freeReg(lhsReg, 'f');
    freeReg(rhsReg, 'f');
    freeReg(resultReg, 'f');
    freeReg(intResultReg, 'i');
}

void genUnaryOpInt(AST_NODE* exprNode, AST_NODE* operand)
{
    int operandReg = getReg('i');
    int resultReg = getReg('i');
    switch (exprNode->semantic_value.exprSemanticValue.op.unaryOp) {
        case UNARY_OP_NEGATIVE:
            fprintf(fout, "\tneg x%d, x%d\n", resultReg, operandReg);
            break;
        case UNARY_OP_LOGICAL_NEGATION:
            fprintf(fout, "\tseqz x%d, x%d\n", resultReg, operandReg);
            break;
    }
    storeNode(exprNode, resultReg);
    freeReg(operandReg, 'i');
    freeReg(resultReg, 'i');
}

void genUnaryOpFloat(AST_NODE* exprNode, AST_NODE* operand)
{
    int operandReg = getReg('f');
    loadNode(operand, operandReg);
    int resultReg = getReg('f');
    int intResultReg = getReg('i');
    switch (exprNode->semantic_value.exprSemanticValue.op.unaryOp) {
        case UNARY_OP_NEGATIVE:
            fprintf(fout, "\tfneg.s f%d, f%d\n", resultReg, operandReg);
            break;
        case UNARY_OP_LOGICAL_NEGATION:
            fprintf(fout, "\tfmv.w.x f%d, zero\n", resultReg);
            fprintf(fout, "\tfeq.s x%d, f%d, f%d\n", intResultReg, operandReg, resultReg);
            break;
    }
    if (exprNode->dataType == INT_TYPE)
        storeNode(exprNode, intResultReg);
    else
        storeNode(exprNode, resultReg);
    freeReg(operandReg, 'f');
    freeReg(resultReg, 'f');
    freeReg(intResultReg, 'i');
}

void genLogicalAnd(AST_NODE* exprNode, AST_NODE* leftOperand, AST_NODE* rightOperand)
{
    int leftBoolReg = getReg('i');
    int rightBoolReg = getReg('i');
    if (leftOperand->dataType == FLOAT_TYPE) {
        int zeroReg = getReg('f');
        int leftOperandReg = getReg('f');
        loadNode(leftOperand, leftOperandReg);
        fprintf(fout, "\tfmv.w.x f%d, zero\n", zeroReg);
        fprintf(fout, "\tfeq.s x%d, f%d, f%d\n", leftBoolReg, zeroReg, leftOperandReg);
        freeReg(zeroReg, 'f');
        freeReg(leftOperandReg, 'f');
    } else {
        loadNode(leftOperand, leftBoolReg);
    }

    char endLabel[128];
    snprintf(endLabel, 128, ".lAndFalse%d", getLabel());
    fprintf(fout, "\tbeqz x%d, %s\n", leftBoolReg, endLabel);
    
    if (rightOperand->dataType == FLOAT_TYPE) {
        int zeroReg = getReg('f');
        int rightOperandReg = getReg('f');
        loadNode(leftOperand, rightOperandReg);
        fprintf(fout, "\tfmv.w.x f%d, zero\n", zeroReg);
        fprintf(fout, "\tfeq.s x%d, f%d, f%d\n", rightBoolReg, zeroReg, rightOperandReg);
        freeReg(zeroReg, 'f');
        freeReg(rightOperandReg, 'f');
    } else {
        loadNode(rightOperand, rightBoolReg);
    }

    fprintf(fout, "\tbeqz x%d, %s\n", rightBoolReg, endLabel);

    // evaluated as true
    storeNode(exprNode, rightBoolReg);

    // evaluated as false
    fprintf(fout, "%s:\n", endLabel);
    storeNode(exprNode, leftBoolReg);

    freeReg(leftBoolReg, 'i');
    freeReg(rightBoolReg, 'i');
}

void genLogicalOr(AST_NODE* exprNode, AST_NODE* leftOperand, AST_NODE* rightOperand)
{
    int leftBoolReg = getReg('i');
    int rightBoolReg = getReg('i');
    if (leftOperand->dataType == FLOAT_TYPE) {
        int zeroReg = getReg('f');
        int leftOperandReg = getReg('f');
        loadNode(leftOperand, leftOperandReg);
        fprintf(fout, "\tfmv.w.x f%d, zero\n", zeroReg);
        fprintf(fout, "\tfeq.s x%d, f%d, f%d\n", leftBoolReg, zeroReg, leftOperandReg);
        freeReg(zeroReg, 'f');
        freeReg(leftOperandReg, 'f');
    } else {
        loadNode(leftOperand, leftBoolReg);
    }

    char endLabel[128];
    snprintf(endLabel, 128, ".lOrTrue%d", getLabel());
    fprintf(fout, "\tbnez x%d, %s\n", leftBoolReg, endLabel);
    
    if (rightOperand->dataType == FLOAT_TYPE) {
        int zeroReg = getReg('f');
        int rightOperandReg = getReg('f');
        loadNode(leftOperand, rightOperandReg);
        fprintf(fout, "\tfmv.w.x f%d, zero\n", zeroReg);
        fprintf(fout, "\tfeq.s x%d, f%d, f%d\n", rightBoolReg, zeroReg, rightOperandReg);
        freeReg(zeroReg, 'f');
        freeReg(rightOperandReg, 'f');
    } else {
        loadNode(rightOperand, rightBoolReg);
    }

    fprintf(fout, "\tbnez x%d, %s\n", rightBoolReg, endLabel);

    // evaluated as false
    storeNode(exprNode, rightBoolReg);

    // evaluated as true
    fprintf(fout, "%s:\n", endLabel);
    storeNode(exprNode, leftBoolReg);

    freeReg(leftBoolReg, 'i');
    freeReg(rightBoolReg, 'i');
}

void genConst(AST_NODE* constNode)
{
    char label[128];
    int reg = -1, addrReg = -1;
    switch (constNode->dataType) {
        case INT_TYPE:
            constNode->offset = allocFrame(4);
            reg = getReg('i');
            fprintf(fout, "\tli x%d, %d\n", reg,
                    constNode->semantic_value.const1->const_u.intval);
            storeNode(constNode, reg);
            freeReg(reg, 'i');
            break;
        case FLOAT_TYPE:
            constNode->offset = allocFrame(4);
            snprintf(label, 128, ".Const%d", getLabel());
            fprintf(fout, ".data\n");
            fprintf(fout, "%s: .word %u\n", label,
                    getFloatRepr(constNode->semantic_value.const1->const_u.fval));
            addrReg = getReg('i');
            fprintf(fout, ".text\n");
            fprintf(fout, "\tla x%d, %s\n", addrReg, label);
            reg = getReg('f');
            fprintf(fout, "\tflw f%d, 0(x%d)\n", reg, addrReg);
            storeNode(constNode, reg);
            freeReg(addrReg, 'i');
            freeReg(reg, 'f');
            break;
        case CONST_STRING_TYPE:
            snprintf(label, 128, ".Const%d", getLabel());
            constNode->globalLabel = strdup(label);
            fprintf(fout, ".data\n"
                          ".align 3\n");
            fprintf(fout, "%s: .string %s\n", label,
                    constNode->semantic_value.const1->const_u.sc);
            break;
    }
    fprintf(fout, ".text\n");
}

/******************************
 * Register Management
 ******************************/

int temporaryRegisters[] = {5, 6, 7, 28, 29, 30, 31};
int savedRegisters[] = {9, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27};
int allocatableRegisters[] = {9, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 5, 6, 7, 28, 29, 30, 31};
int argumentRegisters[] = {10, 11, 12, 13, 14, 15, 16, 17};

int floatTemporaryRegisters[] = {0, 1, 2, 3, 4, 5, 6, 7, 28, 29, 30, 31};
int floatSavedRegisters[] = {8, 9, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27};
int floatAllocatableRegisters[] = {8, 9, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
int floatArgumentRegisters[] = {10, 11, 12, 13, 14, 15, 16, 17};

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
    long long currentOffset = 16;
    int numReg = sizeof(savedRegisters) / sizeof(savedRegisters[0]);
    for (int i = 0; i < numReg; i++, currentOffset += 8) {
        int reg = savedRegisters[i];
        savedRegOffset[reg] = currentOffset;
        fprintf(fout, "\tsd x%d, -%lld(fp)\n", reg, savedRegOffset[reg]);
    }
    numReg = sizeof(floatSavedRegisters) / sizeof(floatSavedRegisters[0]);
    for (int i = 0; i < numReg; i++, currentOffset += 8) {
        int reg = floatSavedRegisters[i];
        floatSavedRegOffset[reg] = currentOffset;
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

long long temporaryRegOffset[32], floatTemporaryRegOffset[32];
long long argumentRegOffset[32], floatArgumentRegOffset[32];
void storeCallerSavedRegisters()
{
    long long currentOffset = 200;
    int numReg = sizeof(temporaryRegisters) / sizeof(temporaryRegisters[0]);
    for (int i = 0; i < numReg; i++, currentOffset += 8) {
        int reg = temporaryRegisters[i];
        temporaryRegOffset[reg] = currentOffset;
        fprintf(fout, "\tsd x%d, -%lld(fp)\n", reg, temporaryRegOffset[reg]);
    }
    numReg = sizeof(argumentRegisters) / sizeof(argumentRegisters[0]);
    for (int i = 0; i < numReg; i++, currentOffset += 8) {
        int reg = argumentRegisters[i];
        argumentRegOffset[reg] = currentOffset;
        fprintf(fout, "\tsd x%d, -%lld(fp)\n", reg, argumentRegOffset[reg]);
    }
    numReg = sizeof(floatTemporaryRegisters) / sizeof(floatTemporaryRegisters[0]);
    for (int i = 0; i < numReg; i++, currentOffset += 8) {
        int reg = floatTemporaryRegisters[i];
        floatTemporaryRegOffset[reg] = currentOffset;
        fprintf(fout, "\tfsd f%d, -%lld(fp)\n", reg, floatTemporaryRegOffset[reg]);
    }
    numReg = sizeof(floatArgumentRegisters) / sizeof(floatArgumentRegisters[0]);
    for (int i = 0; i < numReg; i++, currentOffset += 8) {
        int reg = floatArgumentRegisters[i];
        floatArgumentRegOffset[reg] = currentOffset;
        fprintf(fout, "\tfsd f%d, -%lld(fp)\n", reg, floatArgumentRegOffset[reg]);
    }
}

void restoreCallerSavedRegisters()
{
    int numReg = sizeof(temporaryRegisters) / sizeof(temporaryRegisters[0]);
    for (int i = 0; i < numReg; i++) {
        int reg = temporaryRegisters[i];
        fprintf(fout, "\tld x%d, -%lld(fp)\n", reg, temporaryRegOffset[reg]);
    }
    numReg = sizeof(argumentRegisters) / sizeof(argumentRegisters[0]);
    for (int i = 0; i < numReg; i++) {
        int reg = argumentRegisters[i];
        fprintf(fout, "\tld x%d, -%lld(fp)\n", reg, argumentRegOffset[reg]);
    }
    numReg = sizeof(floatTemporaryRegisters) / sizeof(floatTemporaryRegisters[0]);
    for (int i = 0; i < numReg; i++) {
        int reg = floatTemporaryRegisters[i];
        fprintf(fout, "\tfld f%d, -%lld(fp)\n", reg, floatTemporaryRegOffset[reg]);
    }
    numReg = sizeof(floatArgumentRegisters) / sizeof(floatArgumentRegisters[0]);
    for (int i = 0; i < numReg; i++) {
        int reg = floatArgumentRegisters[i];
        fprintf(fout, "\tfld f%d, -%lld(fp)\n", reg, floatArgumentRegOffset[reg]);
    }
}

/******************************
 * Stack Management
 ******************************/

long long curFrameSize;

void initFrameSize()
{
    curFrameSize = 472;
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

void setFrameSize(long long frameSize)
{
    curFrameSize = frameSize;
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
        addi(addrReg, getSymtabEntry(node)->offset);
    } else {
        addi(addrReg, node->offset);
    }

    // if this node is an array reference, calculate the address of the referenced element
    if (isArrayId(node)) {
        genExprRelatedNode(node->child);
        int arrayOffsetReg = getReg('i');
        loadNode(node->child, arrayOffsetReg);
        fprintf(fout, "\tslli x%d, x%d, 2\n", arrayOffsetReg, arrayOffsetReg);
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

    freeReg(addrReg, 'i');
}

void loadNode(AST_NODE* node, int reg)
{
    /* node->offset should be the symbol offset if node is a local variable reference */
    int addrReg = -1;
    addrReg = getReg('i');
    
    // load the (base) address of this node
    if (isGlobalId(node)) {
        fprintf(fout, "\tla x%d, %s\n", addrReg, getSymtabEntry(node)->globalLabel);
    } else if (node->nodeType == CONST_VALUE_NODE) {
        freeReg(addrReg, 'i');
        loadConstantNode(node, reg);
        return;
    } else {
        addi(addrReg, node->offset);
    }

    // if this node is an array reference, calculate the address of the referenced element
    if (isArrayId(node)) {
        genExprRelatedNode(node->child);
        int arrayOffsetReg = getReg('i');
        loadNode(node->child, arrayOffsetReg);
        fprintf(fout, "\tslli x%d, x%d, 2\n", arrayOffsetReg, arrayOffsetReg);
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

    freeReg(addrReg, 'i');
}

void loadConstantNode(AST_NODE* constNode, int reg)
{
    int tmpReg = -1;
    switch (constNode->dataType) {
        case INT_TYPE:
            tmpReg = getReg('i');
            addi(tmpReg, constNode->offset);
            fprintf(fout, "\tlw x%d, 0(x%d)\n", reg, tmpReg);
            freeReg(tmpReg, 'i');
            break;
        case FLOAT_TYPE:
            tmpReg = getReg('i');
            addi(tmpReg, constNode->offset);
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
           getSymtabEntry(node)->attribute->attr.typeDescriptor->kind == ARRAY_TYPE_DESCRIPTOR;
}

unsigned getFloatRepr(float f)
{
    union { unsigned uu; float ff; } tmp;
    tmp.ff = f;
    return tmp.uu;
}

void addi(int reg, long long offset) {
    int tmpReg = getReg('i');
    fprintf(fout, "\tli x%d, -%lld\n", tmpReg, offset);
    fprintf(fout, "\tadd x%d, fp, x%d\n", reg, tmpReg);
    freeReg(tmpReg, 'i');
}
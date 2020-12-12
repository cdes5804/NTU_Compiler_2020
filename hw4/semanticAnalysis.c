#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "header.h"
#include "symbolTable.h"
// This file is for reference only, you are not required to follow the implementation. //
// You only need to check for errors stated in the hw4 document. //
int g_anyErrorOccur = 0;

DATA_TYPE getBiggerType(DATA_TYPE dataType1, DATA_TYPE dataType2);
void processProgramNode(AST_NODE *programNode);
void processDeclarationNode(AST_NODE* declarationNode);
void declareIdList(AST_NODE* typeNode, SymbolAttributeKind isVariableOrTypeAttribute, int ignoreArrayFirstDimSize);
void declareFunction(AST_NODE* returnTypeNode);
void processDeclDimList(AST_NODE* variableDeclDimList, TypeDescriptor* typeDescriptor, int ignoreFirstDimSize);
void processTypeNode(AST_NODE* typeNode);
void processBlockNode(AST_NODE* blockNode);
void processStmtNode(AST_NODE* stmtNode);
void processGeneralNode(AST_NODE *node);
void checkAssignOrExpr(AST_NODE* assignOrExprRelatedNode);
void checkWhileStmt(AST_NODE* whileNode);
void checkForStmt(AST_NODE* forNode);
void checkAssignmentStmt(AST_NODE* assignmentNode);
void checkIfStmt(AST_NODE* ifNode);
void checkWriteFunction(AST_NODE* functionCallNode);
void checkFunctionCall(AST_NODE* functionCallNode);
void processExprRelatedNode(AST_NODE* exprRelatedNode);
void checkParameterPassing(Parameter* formalParameter, AST_NODE* actualParameter);
void checkReturnStmt(AST_NODE* returnNode);
void processExprNode(AST_NODE* exprNode);
void processVariableLValue(AST_NODE* idNode);
void processVariableRValue(AST_NODE* idNode);
void processConstValueNode(AST_NODE* constValueNode);
void getExprOrConstValue(AST_NODE* exprOrConstNode, int* iValue, float* fValue);
void evaluateExprValue(AST_NODE* exprNode);


typedef enum ErrorMsgKind
{
    SYMBOL_IS_NOT_TYPE,
    SYMBOL_REDECLARE,
    SYMBOL_UNDECLARED,
    NOT_FUNCTION_NAME,
    TRY_TO_INIT_ARRAY,
    EXCESSIVE_ARRAY_DIM_DECLARATION,
    RETURN_ARRAY,
    VOID_VARIABLE,
    TYPEDEF_VOID_ARRAY,
    PARAMETER_TYPE_UNMATCH,
    TOO_FEW_ARGUMENTS,
    TOO_MANY_ARGUMENTS,
    RETURN_TYPE_UNMATCH,
    INCOMPATIBLE_ARRAY_DIMENSION,
    NOT_ASSIGNABLE,
    NOT_ARRAY,
    IS_TYPE_NOT_VARIABLE,
    IS_FUNCTION_NOT_VARIABLE,
    STRING_OPERATION,
    ARRAY_SIZE_NOT_INT,
    ARRAY_SIZE_NEGATIVE,
    ARRAY_SUBSCRIPT_NOT_INT,
    PASS_ARRAY_TO_SCALAR,
    PASS_SCALAR_TO_ARRAY,
    NON_CONST_GLOBAL_INITIALIZATION,
    TYPE_REDECLARE,
} ErrorMsgKind;

void printErrorMsgSpecial(AST_NODE* node, char* name, ErrorMsgKind errorMsgKind)
{
    g_anyErrorOccur = 1;
    printf("Error found in line %d\n", node->linenumber);
    
    switch(errorMsgKind)
    {
        case PASS_ARRAY_TO_SCALAR:
            printf("invalid conversion from array \'%s\' to scalar \'%s\'\n",
                   node->semantic_value.identifierSemanticValue.identifierName, name);
            break;
        case PASS_SCALAR_TO_ARRAY:
            printf("invalid conversion from scalar \'%s\' to array \'%s\'\n",
                   node->semantic_value.identifierSemanticValue.identifierName, name);
            break;
        default:
            printf("Unhandled case in void printErrorMsgSpecial(AST_NODE* node, char* name, ERROR_MSG_KIND* errorMsgKind)\n");
            break;
    }
}


void printErrorMsg(AST_NODE* node, ErrorMsgKind errorMsgKind)
{
    g_anyErrorOccur = 1;
    printf("Error found in line %d\n", node->linenumber);

    switch (errorMsgKind)
    {
        case SYMBOL_IS_NOT_TYPE:
            printf("unknown type name \'%s\'\n",
                   node->semantic_value.identifierSemanticValue.identifierName);
            break;
        case SYMBOL_REDECLARE:
            printf("redeclaration of \'%s\'\n",
                   node->semantic_value.identifierSemanticValue.identifierName);
            break;
        case SYMBOL_UNDECLARED:
            printf("identifier \'%s\' was not declared in this scope\n",
                   node->semantic_value.identifierSemanticValue.identifierName);
            break;
        case NOT_FUNCTION_NAME:
            printf("called object \'%s\' is not a function\n",
                   node->semantic_value.identifierSemanticValue.identifierName);
            break;
        case TRY_TO_INIT_ARRAY:
            printf("array \'%s\' cannot be initialized. This feature is not supported in C--\n",
                   node->semantic_value.identifierSemanticValue.identifierName);
            break;
        case EXCESSIVE_ARRAY_DIM_DECLARATION:
            printf("dimension of array \'%s\' exceeds maximum array dimension \'%d\'\n",
                   node->semantic_value.identifierSemanticValue.identifierName,
                   MAX_ARRAY_DIMENSION);
            break;
        case RETURN_ARRAY:
            printf("function \'%s\' cannot return array\n",
                   node->semantic_value.identifierSemanticValue.identifierName);
            break;
        case VOID_VARIABLE:
            printf("variable \'%s\' declared void\n",
                   node->semantic_value.identifierSemanticValue.identifierName);
            break;
        case TYPEDEF_VOID_ARRAY:
            printf("declaration of \'%s\' as array of voids\n",
                   node->semantic_value.identifierSemanticValue.identifierName);
            break;
        case PARAMETER_TYPE_UNMATCH:
            /* Because of type coercion in C, this error might be unnecessary */
            printf("parameter type unmatched\n");
            break;
        case TOO_FEW_ARGUMENTS:
            printf("too few arguments to function \'%s\'\n",
                   node->semantic_value.identifierSemanticValue.identifierName);
            break;
        case TOO_MANY_ARGUMENTS:
            printf("too many arguments to function \'%s\'\n",
                   node->semantic_value.identifierSemanticValue.identifierName);
            break;
        case RETURN_TYPE_UNMATCH:
            /* Because of type coercion in c, this error only happens if it returns something in void function */
            printf("return type unmatched\n");
            break;
        case INCOMPATIBLE_ARRAY_DIMENSION:
            printf("incompatible array dimensions\n");
            break;
        case NOT_ASSIGNABLE:
            printf("\'%s\' is not assignable",
                   node->semantic_value.identifierSemanticValue.identifierName);
            break;
        case NOT_ARRAY:
            printf("subscripted value is neither array nor pointer nor vector\n");
            break;
        case IS_TYPE_NOT_VARIABLE:
            printf("identifier \'%s\' is a type, not a variable\n",
                   node->semantic_value.identifierSemanticValue.identifierName);
            break;
        case IS_FUNCTION_NOT_VARIABLE:
            printf("identifier \'%s\' is a function, not a variable\n",
                   node->semantic_value.identifierSemanticValue.identifierName);
            break;
        case STRING_OPERATION:
            printf("string operation is not supported in C--\n");
            break;
        case ARRAY_SIZE_NOT_INT:
            printf("size of array \'%s\' is not an integer\n",
                   node->semantic_value.identifierSemanticValue.identifierName);
            break;
        case ARRAY_SIZE_NEGATIVE:
            printf("size of array \'%s\' is negative\n",
                   node->semantic_value.identifierSemanticValue.identifierName);
            break;
        case ARRAY_SUBSCRIPT_NOT_INT:
            printf("array subscript is not an integer\n");
            break;
        case NON_CONST_GLOBAL_INITIALIZATION:
            printf("initializer element is not constant\n");
            break;
        case TYPE_REDECLARE:
            printf("conflicting types for \'%s\'\n",
                   node->semantic_value.identifierSemanticValue.identifierName);
            break;
        default:
            printf("Unhandled case in void printErrorMsg(AST_NODE* node, ERROR_MSG_KIND* errorMsgKind)\n");
            break;
    }
}


void semanticAnalysis(AST_NODE *root)
{
    processProgramNode(root);
}


DATA_TYPE getBiggerType(DATA_TYPE dataType1, DATA_TYPE dataType2)
{
    if(dataType1 == FLOAT_TYPE || dataType2 == FLOAT_TYPE) {
        return FLOAT_TYPE;
    } else {
        return INT_TYPE;
    }
}

char* getIdName(AST_NODE* node) {
    return node->semantic_value.identifierSemanticValue.identifierName;
}

void processProgramNode(AST_NODE* programNode)
{
    AST_NODE* globalDeclarationNode = programNode->child;
    while (globalDeclarationNode) {
        AST_TYPE nodeType = globalDeclarationNode->nodeType;
        if (nodeType == VARIABLE_DECL_LIST_NODE) { // variable declaration
            processGeneralNode(globalDeclarationNode);
        } else if (nodeType == DECLARATION_NODE) { // function declaration
            processDeclarationNode(globalDeclarationNode);
        } else {
            fprintf(stderr, "Invalid node type in program node\n");
        }

        globalDeclarationNode = globalDeclarationNode->rightSibling;
    }
}

void processDeclarationNode(AST_NODE* declarationNode)
{
    AST_NODE* typeNode = declarationNode->child;
    processTypeNode(typeNode);
    DECL_KIND declarationType = declarationNode->semantic_value.declSemanticValue.kind;
    if (declarationType == VARIABLE_DECL) {
        declareIdList(declarationNode, VARIABLE_ATTRIBUTE, 0);
    } else if (declarationType == TYPE_DECL) {
        declareIdList(declarationNode, TYPE_ATTRIBUTE, 0);
    } else if (declarationType == FUNCTION_DECL) {
        declareFunction(declarationNode);
    } else if (declarationType == FUNCTION_PARAMETER_DECL) {
        declareIdList(declarationNode, VARIABLE_ATTRIBUTE, 1);
    } else {
        fprintf(stderr, "Invalid declaration type in declaration node\n");
    }
}


void processTypeNode(AST_NODE* idNodeAsType)
{
}


void declareIdList(AST_NODE* declarationNode, SymbolAttributeKind isVariableOrTypeAttribute, int ignoreArrayFirstDimSize)
{
}

void checkAssignOrExpr(AST_NODE* assignOrExprRelatedNode)
{
}

void checkWhileStmt(AST_NODE* whileNode)
{
}


void checkForStmt(AST_NODE* forNode)
{
}


void checkAssignmentStmt(AST_NODE* assignmentNode)
{
}


void checkIfStmt(AST_NODE* ifNode)
{
}

void checkWriteFunction(AST_NODE* functionCallNode)
{
}

void checkFunctionCall(AST_NODE* functionCallNode)
{
}

void checkParameterPassing(Parameter* formalParameter, AST_NODE* actualParameter)
{
}


void processExprRelatedNode(AST_NODE* exprRelatedNode)
{
}

void getExprOrConstValue(AST_NODE* exprOrConstNode, int* iValue, float* fValue)
{
}

void evaluateExprValue(AST_NODE* exprNode)
{
}


void processExprNode(AST_NODE* exprNode)
{
}


void processVariableLValue(AST_NODE* idNode)
{
}

void processVariableRValue(AST_NODE* idNode)
{
}


void processConstValueNode(AST_NODE* constValueNode)
{
}


void checkReturnStmt(AST_NODE* returnNode)
{
}


void processBlockNode(AST_NODE* blockNode)
{
}


void processStmtNode(AST_NODE* stmtNode)
{
}


void processGeneralNode(AST_NODE *node)
{
    AST_TYPE nodeType = node->nodeType;
    AST_NODE* childNode = node->child;
    if (nodeType == VARIABLE_DECL_LIST_NODE) {
        while (childNode) {
            processDeclarationNode(childNode);
            childNode = childNode->rightSibling;
        }
    } else if (nodeType == STMT_LIST_NODE) {
        while (childNode) {
            processStmtNode(childNode);
            childNode = childNode->rightSibling;
        }
    } else if (nodeType == NONEMPTY_ASSIGN_EXPR_LIST_NODE) {
        while (childNode) {

        }
    } else if (nodeType == NONEMPTY_RELOP_EXPR_LIST_NODE) {
        while (childNode) {

        }
    } else {
        fprintf(stderr, "Invalid node type in general node\n");
    }
}

void processDeclDimList(AST_NODE* idNode, TypeDescriptor* typeDescriptor, int ignoreFirstDimSize)
{
}


void declareFunction(AST_NODE* declarationNode)
{
}

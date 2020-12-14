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
void processInitializer(AST_NODE* idNode);

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
            printf("initializer element is not constant in global scope\n");
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
    SymbolTableEntry *symtab_lookup = retrieveSymbol(idNodeAsType->semantic_value.identifierSemanticValue.identifierName);
    if (symtab_lookup == NULL || symtab_lookup->attribute->attributeKind != TYPE_ATTRIBUTE) {
        printErrorMsg(idNodeAsType, SYMBOL_IS_NOT_TYPE);
        idNodeAsType->dataType = ERROR_TYPE;
        return;
    }
    idNodeAsType->semantic_value.identifierSemanticValue.symbolTableEntry = symtab_lookup;
    switch (symtab_lookup->attribute->attr.typeDescriptor->kind) {
        case SCALAR_TYPE_DESCRIPTOR:
            idNodeAsType->dataType = symtab_lookup->attribute->attr.typeDescriptor->properties.dataType;
            break;
        case ARRAY_TYPE_DESCRIPTOR:
            idNodeAsType->dataType = symtab_lookup->attribute->attr.typeDescriptor->properties.arrayProperties.elementType;
            break;
        default:
            fprintf(stderr, "Internal Error: invalid typeDescriptor kind in processTypeNode\n");
            break;
    }
}

void declareIdList(AST_NODE* declarationNode, SymbolAttributeKind isVariableOrTypeAttribute, int ignoreArrayFirstDimSize)
{
    AST_NODE *typeNode = declarationNode->child;
    TypeDescriptor *typeDescriptor_typeNode = typeNode->semantic_value.identifierSemanticValue.symbolTableEntry->attribute->attr.typeDescriptor;
    AST_NODE *idNode = declarationNode->child->rightSibling;

    for (; idNode != NULL; idNode = idNode->rightSibling) {
        idNode->dataType = typeNode->dataType;
        IdentifierSemanticValue semanticValue = idNode->semantic_value.identifierSemanticValue;

        // detect redeclaration error
        if (declaredLocally(semanticValue.identifierName)) {
            if (isVariableOrTypeAttribute == VARIABLE_ATTRIBUTE) {
                printErrorMsg(idNode, SYMBOL_REDECLARE);
            } else if (isVariableOrTypeAttribute == TYPE_ATTRIBUTE) {
                printErrorMsg(idNode, TYPE_REDECLARE);
            }
            idNode->dataType = declarationNode->dataType = ERROR_TYPE;
            continue;
        }

        // detect void variable
        if (typeNode->dataType == VOID_TYPE && isVariableOrTypeAttribute == VARIABLE_ATTRIBUTE) {
            printErrorMsg(idNode, VOID_VARIABLE);
            idNode->dataType = declarationNode->dataType = ERROR_TYPE;
            continue;
        }

        SymbolAttribute *symbolAttribute = (SymbolAttribute*)malloc(sizeof(SymbolAttribute));
        symbolAttribute->attributeKind = isVariableOrTypeAttribute;
        switch (idNode->semantic_value.identifierSemanticValue.kind) {
            case NORMAL_ID:
                symbolAttribute->attr.typeDescriptor = typeDescriptor_typeNode;
                break;
            case ARRAY_ID:
                // detect "typedef void array"
                if (typeNode->dataType == VOID_TYPE && isVariableOrTypeAttribute == TYPE_ATTRIBUTE) {
                    printErrorMsg(idNode, TYPEDEF_VOID_ARRAY);
                    idNode->dataType = ERROR_TYPE;
                }

                TypeDescriptor *typeDescriptor_idNode = (TypeDescriptor*)malloc(sizeof(TypeDescriptor));
                symbolAttribute->attr.typeDescriptor = typeDescriptor_idNode;
                typeDescriptor_idNode->kind = ARRAY_TYPE_DESCRIPTOR;

                processDeclDimList(idNode, typeDescriptor_idNode, ignoreArrayFirstDimSize);

                // synthesize array dimension
                int typeArrayDimension = typeDescriptor_typeNode->properties.arrayProperties.dimension;
                int idArrayDimension = typeDescriptor_idNode->properties.arrayProperties.dimension;
                typeDescriptor_idNode->properties.arrayProperties.dimension += typeArrayDimension;
                if (symbolAttribute->attr.typeDescriptor->properties.arrayProperties.dimension > MAX_ARRAY_DIMENSION) {
                    printErrorMsg(idNode, EXCESSIVE_ARRAY_DIM_DECLARATION);
                    idNode->dataType = ERROR_TYPE;
                    break;
                }
                for (int i = 0; i < typeArrayDimension; i++) {
                    typeDescriptor_idNode->properties.arrayProperties.sizeInEachDimension[idArrayDimension + i] = 
                        typeDescriptor_typeNode->properties.arrayProperties.sizeInEachDimension[i];
                }

                // synthesize element type
                typeDescriptor_idNode->properties.arrayProperties.elementType = 
                    typeDescriptor_typeNode->properties.arrayProperties.elementType;
                break;
            case WITH_INIT_ID:
                if (typeDescriptor_typeNode->kind == ARRAY_TYPE_DESCRIPTOR) {
                    printErrorMsg(idNode, TRY_TO_INIT_ARRAY);
                    idNode->dataType = ERROR_TYPE;
                } else {
                    symbolAttribute->attr.typeDescriptor = typeDescriptor_typeNode;
                    processInitializer(idNode);
                }
                break;
            default:
                fprintf(stderr, "Internal Error: unrecognized identifier kind in declareIdList\n");
                break;
        }

        if (idNode->dataType == ERROR_TYPE) {
            free(symbolAttribute);
            declarationNode->dataType = ERROR_TYPE;
            continue;
        }

        SymbolTableEntry* symtab_entry = insertSymbol(semanticValue.identifierName, symbolAttribute);
        idNode->semantic_value.identifierSemanticValue.symbolTableEntry = symtab_entry;
    }
}

void checkAssignOrExpr(AST_NODE* assignOrExprRelatedNode)
{
    if (assignOrExprRelatedNode->nodeType == STMT_NODE) {
        switch (assignOrExprRelatedNode->semantic_value.stmtSemanticValue.kind) {
            case ASSIGN_STMT:
                checkAssignmentStmt(assignOrExprRelatedNode);
                break;
            case FUNCTION_CALL_STMT:
                checkFunctionCall(assignOrExprRelatedNode);
                break;
        }
    } else {
        processExprRelatedNode(assignOrExprRelatedNode);
    }
}

void checkWhileStmt(AST_NODE* whileNode)
{
    checkAssignOrExpr(whileNode->child);
    processStmtNode(whileNode->child->rightSibling);
}


void checkForStmt(AST_NODE* forNode)
{
    AST_NODE* initExpr = forNode->child;
    AST_NODE* conditionExpr = initExpr->rightSibling;
    AST_NODE* updateExpr = conditionExpr->rightSibling;
    AST_NODE* bodyNode = conditionExpr->rightSibling;

    processGeneralNode(initExpr);
    processGeneralNode(conditionExpr);
    processGeneralNode(updateExpr);
    processStmtNode(bodyNode);
}


void checkAssignmentStmt(AST_NODE* assignmentNode)
{
    AST_NODE* idNode = assignmentNode->child;
    AST_NODE* exprNode = idNode->rightSibling;

    processVariableLValue(idNode);
    processExprRelatedNode(exprNode);

    if (idNode->dataType == ERROR_TYPE || exprNode->dataType == ERROR_TYPE) {
        assignmentNode->dataType = ERROR_TYPE;
    } else if (exprNode->dataType == INT_PTR_TYPE || exprNode->dataType == FLOAT_PTR_TYPE) {
        printErrorMsg(exprNode, INCOMPATIBLE_ARRAY_DIMENSION);
        assignmentNode->dataType = ERROR_TYPE;
    } else if (exprNode->dataType = CONST_STRING_TYPE) {
        printErrorMsg(exprNode, STRING_OPERATION);
        assignmentNode->dataType = ERROR_TYPE;
    } else {
        assignmentNode->dataType = getBiggerType(idNode->dataType, exprNode->dataType);
    }
}


void checkIfStmt(AST_NODE* ifNode)
{
    AST_NODE* boolExprNode = ifNode->child;
    AST_NODE* ifBodyNode = boolExprNode->rightSibling;
    AST_NODE* elseBodyNode = ifBodyNode->rightSibling;

    checkAssignOrExpr(boolExprNode);
    processStmtNode(ifBodyNode);
    processStmtNode(elseBodyNode);
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
    switch (exprRelatedNode->nodeType) {
        case EXPR_NODE:
            processExprNode(exprRelatedNode);
            break;
        case STMT_NODE:
            checkFunctionCall(exprRelatedNode);
            break;
        case IDENTIFIER_NODE:
            processVariableRValue(exprRelatedNode);
            break;
        case CONST_VALUE_NODE:
            processConstValueNode(exprRelatedNode);
            break;
        default:
            fprintf(stderr, "Unrecognized nodeType in exprRelatedNode\n");
            exprRelatedNode->dataType = ERROR_TYPE;
            break;
    }
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
    switch (constValueNode->semantic_value.const1->const_type) {
        case INTEGERC:
            constValueNode->dataType = INT_TYPE;
            constValueNode->semantic_value.exprSemanticValue.constEvalValue.iValue =
                constValueNode->semantic_value.const1->const_u.intval;
            break;
        case FLOATC:
            constValueNode->dataType = FLOAT_TYPE;
            constValueNode->semantic_value.exprSemanticValue.constEvalValue.fValue = 
                constValueNode->semantic_value.const1->const_u.fval;
            break;
        case STRINGC:
            constValueNode->dataType = CONST_STRING_TYPE;
            break;
        default:
            fprintf(stderr, "Unrecognized const_type in constValueNode\n");
            constValueNode->dataType = ERROR_TYPE;
            break;
    }
}


void checkReturnStmt(AST_NODE* returnNode)
{
}


void processBlockNode(AST_NODE* blockNode)
{
    openNewScope();

    AST_NODE* node = blockNode->child;
    while (node) {
        processGeneralNode(node);
        node = node->rightSibling;
    }

    closeCurrentScope();
}


void processStmtNode(AST_NODE* stmtNode)
{
    if (stmtNode->nodeType == NUL_NODE) {
        return;
    }
    else if (stmtNode->nodeType == BLOCK_NODE) {
        processBlockNode(stmtNode);
    } else {
        switch (stmtNode->semantic_value.stmtSemanticValue.kind) {
            case WHILE_STMT:
                checkWhileStmt(stmtNode);
                break;
            case FOR_STMT:
                checkForStmt(stmtNode);
                break;
            case ASSIGN_STMT:
                checkAssignmentStmt(stmtNode);
                break;
            case IF_STMT:
                checkIfStmt(stmtNode);
                break;
            case FUNCTION_CALL_STMT:
                checkFunctionCall(stmtNode);
                break;
            case RETURN_STMT:
                checkReturnStmt(stmtNode);
                break;
            default:
                fprintf(stderr, "Internal Error: unrecognized SemanticValue kind in stmtNode\n");
                stmtNode->dataType = ERROR_TYPE;
                break;
        }
    }
}

void processGeneralNode(AST_NODE* node)
{
    AST_TYPE nodeType = node->nodeType;
    AST_NODE* childNode = node->child;
    if (nodeType == VARIABLE_DECL_LIST_NODE) {
        while (childNode) {
            processDeclarationNode(childNode);
            if (childNode->dataType == ERROR_TYPE) {
                node->dataType = ERROR_TYPE;
            }
            childNode = childNode->rightSibling;
        }
    } else if (nodeType == STMT_LIST_NODE) {
        while (childNode) {
            processStmtNode(childNode);
            if (childNode->dataType == ERROR_TYPE) {
                node->dataType = ERROR_TYPE;
            }
            childNode = childNode->rightSibling;
        }
    } else if (nodeType == NONEMPTY_ASSIGN_EXPR_LIST_NODE) {
        while (childNode) {
            checkAssignOrExpr(childNode);
            if (childNode->dataType == ERROR_TYPE) {
                node->dataType = ERROR_TYPE;
            }
            childNode = childNode->rightSibling;
        }
    } else if (nodeType == NONEMPTY_RELOP_EXPR_LIST_NODE) {
        while (childNode) {
            processExprRelatedNode(childNode);
            if (childNode->dataType == ERROR_TYPE) {
                node->dataType = ERROR_TYPE;
            }
            childNode = childNode->rightSibling;
        }
    } else {
        if (nodeType != NUL_NODE) {
            fprintf(stderr, "Invalid node type in general node\n");
            node->dataType = ERROR_TYPE;
        }
    }
}

void processDeclDimList(AST_NODE* idNode, TypeDescriptor* typeDescriptor, int ignoreFirstDimSize)
{
    AST_NODE *dimNode = idNode->child;
    int dimension = 0;
    int *dimSize = typeDescriptor->properties.arrayProperties.sizeInEachDimension;
    for (; dimNode != NULL; dimNode = dimNode->rightSibling, ++dimension) {
        if (dimension == 0 && ignoreFirstDimSize && dimNode->nodeType == NUL_NODE) {
            dimSize[0] = 0;
            continue;
        }
        
        processExprRelatedNode(dimNode);
        
        if (dimNode->dataType == ERROR_TYPE) {
            idNode->dataType = ERROR_TYPE;
            return;
        } else if (dimNode->dataType != INT_TYPE) {
            printErrorMsg(idNode, ARRAY_SIZE_NOT_INT);
            idNode->dataType = ERROR_TYPE;
            return;
        }

        switch (dimNode->nodeType) {
            case CONST_VALUE_NODE:
                dimSize[dimension] = dimNode->semantic_value.const1->const_u.intval;
                if (dimSize[dimension] < 0) {
                    printErrorMsg(idNode, ARRAY_SIZE_NEGATIVE);
                    idNode->dataType = ERROR_TYPE;
                }
                break;
            case EXPR_NODE:
                if (dimNode->semantic_value.exprSemanticValue.isConstEval) {
                    dimSize[dimension] = dimNode->semantic_value.exprSemanticValue.constEvalValue.iValue;
                } else {
                    /* TODO handle non-const array size declaration */
                    dimSize[dimension] = -1;
                }
                break;
            default:
                fprintf(stderr, "Internal Error: unexpected node type in dimension declaration\n");
                break;
        }
    }
}


void declareFunction(AST_NODE* declarationNode)
{
}

void processInitializer(AST_NODE* idNode)
{
    AST_NODE *initializerNode = idNode->child;
    processExprRelatedNode(initializerNode);
    if (initializerNode->nodeType == EXPR_NODE) {
        if (!initializerNode->semantic_value.exprSemanticValue.isConstEval && isCurrentScopeGlobal()) {
            printErrorMsg(idNode, NON_CONST_GLOBAL_INITIALIZATION);
            idNode->dataType = ERROR_TYPE;
        }
    }
    if (initializerNode->dataType == ERROR_TYPE)
        idNode->dataType = ERROR_TYPE;
}

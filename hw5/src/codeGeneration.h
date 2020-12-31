#ifndef __CODE_GENERATION_H__
#define __CODE_GENERATION_H__

#include "header.h"
#include <stdbool.h>

void codeGeneration(AST_NODE* program);
void genProgram(AST_NODE* programNode);
void genGeneralNode(AST_NODE* node, char* endLabel);
void genDeclarationNode(AST_NODE* declarationNode);
void genGlobalVar(AST_NODE* idNode, SymbolTableEntry* symtabEntry);
void genLocalVar(AST_NODE* idNode, SymbolTableEntry* symtabEntry);
void genVarDecl(AST_NODE* declarationNode);
void genFuncHead(char* funcName);
void genProloque(char* funcName);
void genEpilogue(char* funcName);
void genFuncDecl(AST_NODE* declarationNode);
void genBlockNode(AST_NODE* blockNode, char* endLabel);
void genStmtNode(AST_NODE* stmtNode, char* endLabel);
void genWhileStmt(AST_NODE* stmtNode, char* endLabel);
void genAssignOrExpr(AST_NODE* node);
void genForStmt(AST_NODE* stmtNode, char* endLabel);
void genIfStmt(AST_NODE* stmtNode, char* endLabel);
void genAssignmentStmt(AST_NODE* stmtNode);
void genFunctionCall(AST_NODE* stmtNode);
void genReturnStmt(AST_NODE* stmtNode, char* endLabel);
void genExprRelatedNode(AST_NODE* exprRelatedNode);
void genVariable(AST_NODE* idNode);
void genExprNode(AST_NODE* exprNode);
void genBinaryOpInt(AST_NODE* exprNode, AST_NODE* leftOperand, AST_NODE* rightOperand);
void genBinaryOpFloat(AST_NODE* exprNode, AST_NODE* leftOperand, AST_NODE* rightOperand);
void genUnaryOpInt(AST_NODE* exprNode, AST_NODE* operand);
void genUnaryOpFloat(AST_NODE* exprNode, AST_NODE* operand);
void genLogicalAnd(AST_NODE* exprNode, AST_NODE* leftOperand, AST_NODE* rightOperand);
void genLogicalOr(AST_NODE* exprNode, AST_NODE* leftOperand, AST_NODE* rightOperand);
void genConst(AST_NODE* constNode);

/* Register Management */
void initReg();
int getReg(char type);
void freeReg(int reg, char type);
void storeCalleeSavedRegisters();
void restoreCalleeSavedRegisters();

/* Stack Management */
void initFrameSize();
long long allocFrame(long long size);
long long getFrameSize();

/* Label Management */
int getLabel();

/* Type Conversion */
void typeConversion(AST_NODE* node, DATA_TYPE targetType);
void storeNode(AST_NODE* node, int reg);
void loadNode(AST_NODE* node, int reg);
void loadConstantNode(AST_NODE* constNode, int reg);
void AssignNode(AST_NODE* dst, AST_NODE* src);

/* Utility */
SymbolTableEntry* getSymtabEntry(AST_NODE* idNode);
int getSymbolSize(SymbolTableEntry* symtabEntry);
bool isPtrType(AST_NODE* node);
bool isGlobalId(AST_NODE* node);
bool isArrayId(AST_NODE* node);
unsigned getFloatRepr(float f);

#endif
#ifndef __CODE_GENERATION_H__
#define __CODE_GENERATION_H__

#include "header.h"

void codeGeneration(AST_NODE* program);
void genProgram(AST_NODE* programNode);
void genGeneralNode(AST_NODE* node, char* endLabel);
void genDeclarationNode(AST_NODE* declarationNode);
int getSymbolSize(SymbolTableEntry* symtabEntry);
void genGlobalVar(SymbolTableEntry* symtabEntry);
void genLocalVar(SymbolTableEntry* symtabEntry);
void genVarDecl(AST_NODE* declarationNode);
void genFuncHead(char* funcName);
void genProloque(char* funcName);
void genEpilogue(char* funcName);
void genFuncDecl(AST_NODE* declarationNode);
void genBlockNode(AST_NODE* blockNode, char* endLabel);
void genWhileStmt(AST_NODE* stmtNode, char* endLabel);
void genForStmt(AST_NODE* stmtNode, char* endLabel);
void genIfStmt(AST_NODE* stmtNode, char* endLabel);
void genAssignmentStmt(AST_NODE* stmtNode);
void genFunctionCall(AST_NODE* stmtNode);
void genReturnStmt(AST_NODE* stmtNode, char* endLabel);

void initReg();
int getReg(char type);
void freeReg(int reg, char type);
void storeCalleeSavedRegisters();
void restoreCalleeSavedRegisters();

void initStack();
long long pushStack(long long size);
long long getFrameSize();

int getLabel();

#endif
#ifndef __CODE_GENERATION_H__
#define __CODE_GENERATION_H__

#include "header.h"

void codeGeneration(AST_NODE* program);
void genProgram(AST_NODE* programNode);
void genGeneralNode(AST_NODE* node);
void genDeclarationNode(AST_NODE* declarationNode);
int getSymbolSize(SymbolTableEntry* symtabEntry);
void genGlobalVar(SymbolTableEntry* symtabEntry);
void genLocalVar(SymbolTableEntry* symtabEntry);
void genVarDecl(AST_NODE* declarationNode);
void genFuncHead(char* funcName);
void genProloque(char* funcName);
void genEpilogue(char* funcName);
void genFuncDecl(AST_NODE* declarationNode);
void genBlockNode(AST_NODE* blockNode);

void initReg();
int getReg();
void freeReg(int reg);
void storeCalleeSavedRegisters();
void restoreCalleeSavedRegisters();

void initStack();
long long pushStack(long long size);
long long getFrameSize();

#endif
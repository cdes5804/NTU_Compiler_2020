#ifndef __CODE_GENERATION_H__
#define __CODE_GENERATION_H__

#include "header.h"

void codeGeneration(AST_NODE* program);
long long pushStack32(int reg);
long long pushStack64(int reg);

#endif
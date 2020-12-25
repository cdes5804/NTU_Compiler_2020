#include <stdio.h>
#include <stdlib.h>

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
    fclose(fout);
}

long long stackOffset;

long long pushStack32(int reg)
{
    fprintf(fout, "addi sp, sp, -4\n");
    fprintf(fout, "sw, x%d, 0(sp)\n");
    stackOffset += 4;
    return stackOffset;
}

long long pushStack64(int reg)
{
    fprintf(fout, "addi sp, sp, -8\n");
    fprintf(fout, "sd, x%d, 0(sp)\n");
    stackOffset += 8;
    return stackOffset;
}
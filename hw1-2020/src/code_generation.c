#include <stdio.h>
#include <stdlib.h>

#include "header.h"

/***********************************************************************
  Code generation
 ************************************************************************/
void fprint_op(FILE *target, ValueType op)
{
    switch (op) {
        case MinusNode:
            fprintf(target,"-\n");
            break;
        case PlusNode:
            fprintf(target,"+\n");
            break;
        case MulNode:
            fprintf(target, "*\n");
            break;
        case DivNode:
            fprintf(target, "/\n");
            break;
        default:
            fprintf(target,"Error in fprintf_op ValueType = %d\n",op);
            break;
    }
}

void fprint_expr( FILE *target, Expression *expr)
{

    if (expr->leftOperand == NULL) {
        switch ((expr->v).type ) {
            case Identifier:
                fprintf(target,"l%c\n",(expr->v).val.id);
                break;
            case IntConst:
                fprintf(target,"%d\n",(expr->v).val.ivalue);
                break;
            case FloatConst:
                fprintf(target,"%f\n", (expr->v).val.fvalue);
                break;
            default:
                fprintf(target,"Error In fprint_left_expr. (expr->v).type=%d\n",(expr->v).type);
                break;
        }
    } else {
        fprint_expr(target, expr->leftOperand);
        if (expr->rightOperand == NULL) {
            fprintf(target,"5k\n");
        } else {
            //	fprint_right_expr(expr->rightOperand);
            fprint_expr(target, expr->rightOperand);
            fprint_op(target, (expr->v).type);
        }
    }
}

void gencode(Program prog, FILE * target)
{
    Statements *stmts = prog.statements;
    Statement stmt;

    while (stmts != NULL) {
        stmt = stmts->first;
        switch (stmt.type) {
            case Print:
                fprintf(target,"l%c\n",stmt.stmt.variable);
                fprintf(target,"p\n");
                break;
            case Assignment:
                fprint_expr(target, stmt.stmt.assign.expr);
                /*
                   if(stmt.stmt.assign.type == Int){
                   fprintf(target,"0 k\n");
                   }
                   else if(stmt.stmt.assign.type == Float){
                   fprintf(target,"5 k\n");
                   }*/
                fprintf(target,"s%c\n",stmt.stmt.assign.id);
                fprintf(target,"0 k\n");
                break;
        }
        stmts=stmts->rest;
    }

}

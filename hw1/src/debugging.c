#include <stdio.h>
#include <stdlib.h>

#include "header.h"

/***************************************
  For our debug,
  you can omit them.
 ****************************************/
void print_expr(Expression *expr)
{
    if (expr == NULL)
        return;
    else {
        print_expr(expr->leftOperand);
        switch ((expr->v).type) {
            case Identifier:
                printf("%c ", (expr->v).val.id);
                break;
            case IntConst:
                printf("%d ", (expr->v).val.ivalue);
                break;
            case FloatConst:
                printf("%f ", (expr->v).val.fvalue);
                break;
            case PlusNode:
                printf("+ ");
                break;
            case MinusNode:
                printf("- ");
                break;
            case MulNode:
                printf("* ");
                break;
            case DivNode:
                printf("/ ");
                break;
            case IntToFloatConvertNode:
                printf("(float) ");
                break;
            default:
                printf("error ");
                break;
        }
        print_expr(expr->rightOperand);
    }
}

void pretty_print_expr(Expression *expr)
{
    if (expr == NULL)
        return;
    switch((expr->v).type) {
        case Identifier:
            pretty_print_expr(expr->leftOperand);
            printf("%c ", (expr->v).val.id);
            pretty_print_expr(expr->rightOperand);
            break;
        case IntConst:
            pretty_print_expr(expr->leftOperand);
            printf("%d ", (expr->v).val.ivalue);
            pretty_print_expr(expr->rightOperand);
            break;
        case FloatConst:
            pretty_print_expr(expr->leftOperand);
            printf("%f ", (expr->v).val.fvalue);
            pretty_print_expr(expr->rightOperand);
            break;
        case PlusNode:
            printf("( ");
            pretty_print_expr(expr->leftOperand);
            printf("+ ");
            pretty_print_expr(expr->rightOperand);
            printf(") ");
            break;
        case MinusNode:
            printf("( ");
            pretty_print_expr(expr->leftOperand);
            printf("- ");
            pretty_print_expr(expr->rightOperand);
            printf(") ");
            break;
        case MulNode:
            printf("( ");
            pretty_print_expr(expr->leftOperand);
            printf("* ");
            pretty_print_expr(expr->rightOperand);
            printf(") ");
            break;
        case DivNode:
            printf("( ");
            pretty_print_expr(expr->leftOperand);
            printf("/ ");
            pretty_print_expr(expr->rightOperand);
            printf(") ");
            break;
        case IntToFloatConvertNode:
            printf("(float)");
            pretty_print_expr(expr->leftOperand);
            break;
        default:
            printf("error ");
            break;
    }
}

Program test_parser(FILE *source)
{
    Declarations *decls;
    Statements *stmts;
    Declaration decl;
    Statement stmt;
    Program program = parser(source);
    fclose(source);
    SymbolTable symtab = build(program);
    check(&program, &symtab);
    program.statements = optimize(program.statements);

    decls = program.declarations;

    while (decls != NULL) {
        decl = decls->first;
        if(decl.type == Int)
            printf("i ");
        if(decl.type == Float)
            printf("f ");
        printf("%c\n",decl.name);
        decls = decls->rest;
    }

    stmts = program.statements;

    while (stmts != NULL) {
        stmt = stmts->first;
        if (stmt.type == Print) {
            printf("p %c \n", stmt.stmt.variable);
        }

        if (stmt.type == Assignment) {
            printf("%c = ", stmt.stmt.assign.id);
            pretty_print_expr(stmt.stmt.assign.expr);
            putchar('\n');
        }
        stmts = stmts->rest;
    }

}

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "header.h"

/********************************************************
  Constant folding on AST
 *********************************************************/

bool isOperator(Expression *expr)
{
    return ((expr->v).type == PlusNode || (expr->v).type == MinusNode || (expr->v).type == MulNode || (expr->v).type == DivNode);
}

bool isConst(Expression *expr)
{
    return (expr->v).type == IntConst || (expr->v).type == FloatConst;
}

int arithmeticInt(int leftVal, int rightVal, Operation op)
{
    int result = 0;
    switch (op) {
        case Plus:
            result = leftVal + rightVal;
            break;
        case Minus:
            result = leftVal - rightVal;
            break;
        case Mul:
            result = leftVal * rightVal;
            break;
        case Div:
            result = leftVal / rightVal;
            break;
        default:
            fprintf(stderr, "Invalid operation in arithmeticInt.\n");
            exit(1);
    }
    return result;
}

float arithmeticFloat(float leftVal, float rightVal, Operation op)
{
    float result = 0;
    switch (op) {
        case Plus:
            result = leftVal + rightVal;
            break;
        case Minus:
            result = leftVal - rightVal;
            break;
        case Mul:
            result = leftVal * rightVal;
            break;
        case Div:
            result = leftVal / rightVal;
            break;
        default:
            fprintf(stderr, "Invalid operation in arithmeticInt.\n");
            exit(1);
    }
    return result;
}

Expression *foldConstant(Expression *expr)
{
    if (expr == NULL)
        return NULL;

    expr->leftOperand = foldConstant(expr->leftOperand);
    expr->rightOperand = foldConstant(expr->rightOperand);

    if ((expr->v).type == IntToFloatConvertNode && isConst(expr->leftOperand)) {
        // If the node is a convertor, and its child is a IntConst, fold it to a FloatConst directly.
        Expression *value = (Expression *)malloc(sizeof(Expression));
        value->leftOperand = value->rightOperand = NULL;
        (value->v).type = FloatConst;
        (value->v).val.fvalue = (float)((expr->leftOperand->v).val.ivalue);
        return value;
    } else if (isOperator(expr) && isConst(expr->leftOperand) && isConst(expr->rightOperand)) {
        // If the node is arithmetic, and both children are constants, fold them to a constant.
        Expression *value = (Expression *)malloc(sizeof(Expression));
        value->leftOperand = value->rightOperand = NULL;
        if (expr->type == Int) {
            int leftVal = (expr->leftOperand->v).val.ivalue;
            int rightVal = (expr->rightOperand->v).val.ivalue;
            Operation op = (expr->v).val.op;
            (value->v).type = IntConst;
            (value->v).val.ivalue = arithmeticInt(leftVal, rightVal, op);
        } else if (expr->type == Float) {
            float leftVal = (expr->leftOperand->v).val.fvalue;
            float rightVal = (expr->rightOperand->v).val.fvalue;
            Operation op = (expr->v).val.op;
            (value->v).type = FloatConst;
            (value->v).val.fvalue = arithmeticFloat(leftVal, rightVal, op);
        } else {
            fprintf(stderr, "Folding error: type is neither Int nor Float.\n");
            exit(1);
        }
        return value;
    }

    return expr;
}

void fold(Statement *stmt)
{
    if (stmt->type == Assignment) {
        AssignmentStatement assign = stmt->stmt.assign;
        fprintf(stderr, "assignment : %c \n", assign.id);
        assign.expr = foldConstant(assign.expr);
        stmt->stmt.assign = assign;
        return;
    } else if (stmt->type == Print) {
        fprintf(stderr, "No constant folding for Print statement.\n");
        return;
    } else {
        fprintf(stderr, "Error: statement error\n");//error
        exit(1);
    }
}

Statements *optimize(Statements *stmts)
{
    Statements *current = stmts;
    while (current != NULL) {
        fold(&current->first);
        current = current->rest; 
    }
    return stmts;
}

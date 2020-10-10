#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "header.h"

/********************************************************
  Parsing
 *********************************************************/
Declaration parseDeclaration(FILE *source, Token token)
{
    Token token2;
    switch (token.type) {
        case FloatDeclaration:
        case IntegerDeclaration:
            token2 = scanner(source);
            if (strcmp(token2.tok, "f") == 0 ||
                    strcmp(token2.tok, "i") == 0 ||
                    strcmp(token2.tok, "p") == 0) {
                printf("Syntax Error: %s cannot be used as id\n", token2.tok);
                exit(1);
            }
            return makeDeclarationNode(token, token2);
        default:
            printf("Syntax Error: Expect Declaration %s\n", token.tok);
            exit(1);
    }
}

Declarations *parseDeclarations(FILE *source)
{
    Token token = scanner(source);
    Declaration decl;
    Declarations *decls;
    switch (token.type) {
        case FloatDeclaration:
        case IntegerDeclaration:
            decl = parseDeclaration(source, token);
            decls = parseDeclarations(source);
            return makeDeclarationTree(decl, decls);
        case PrintOp:
        case Alphabet:
            ungetc(token.tok[0], source);
            return NULL;
        case EOFsymbol:
            return NULL;
        default:
            printf("Syntax Error: Expect declarations %s\n", token.tok);
            exit(1);
    }
}

Expression *parseValue(FILE *source)
{
    Token token = scanner(source);
    Expression *value = (Expression *)malloc(sizeof(Expression));
    value->leftOperand = value->rightOperand = NULL;

    switch (token.type) {
        case Alphabet:
            (value->v).type = Identifier;
            (value->v).val.id = token.tok[0];
            break;
        case IntValue:
            (value->v).type = IntConst;
            (value->v).val.ivalue = atoi(token.tok);
            break;
        case FloatValue:
            (value->v).type = FloatConst;
            (value->v).val.fvalue = atof(token.tok);
            break;
        default:
            printf("Syntax Error: Expect Identifier or a Number %s\n", token.tok);
            exit(1);
    }

    return value;
}

Expression *parseExpressionTail(FILE *source, Expression *lvalue)
{
    Token token = scanner(source);
    Expression *expr;

    switch (token.type) {
        case PlusOp:
            expr = (Expression *)malloc(sizeof(Expression));
            (expr->v).type = PlusNode;
            (expr->v).val.op = Plus;
            expr->leftOperand = lvalue;
            expr->rightOperand = parseValue(source);
            return parseExpressionTail(source, expr);
        case MinusOp:
            expr = (Expression *)malloc(sizeof(Expression));
            (expr->v).type = MinusNode;
            (expr->v).val.op = Minus;
            expr->leftOperand = lvalue;
            expr->rightOperand = parseValue(source);
            return parseExpressionTail(source, expr);
        case Alphabet:
        case PrintOp:
            ungetc(token.tok[0], source);
            return lvalue;
        case EOFsymbol:
            return lvalue;
        default:
            printf("Syntax Error: Expect a numeric value or an identifier %s\n", token.tok);
            exit(1);
    }
}

Expression *parseExpression(FILE *source, Expression *lvalue)
{
    Token token = scanner(source);
    Expression *expr;

    switch (token.type) {
        case PlusOp:
            expr = (Expression *)malloc(sizeof(Expression));
            (expr->v).type = PlusNode;
            (expr->v).val.op = Plus;
            expr->leftOperand = lvalue;
            expr->rightOperand = parseValue(source);
            return parseExpressionTail(source, expr);
        case MinusOp:
            expr = (Expression *)malloc(sizeof(Expression));
            (expr->v).type = MinusNode;
            (expr->v).val.op = Minus;
            expr->leftOperand = lvalue;
            expr->rightOperand = parseValue(source);
            return parseExpressionTail(source, expr);
        case Alphabet:
        case PrintOp:
            ungetc(token.tok[0], source);
            return NULL;
        case EOFsymbol:
            return NULL;
        default:
            printf("Syntax Error: Expect a numeric value or an identifier %s\n", token.tok);
            exit(1);
    }
}

Statement parseStatement(FILE *source, Token token)
{
    Token next_token;
    Expression *value, *expr;

    switch(token.type){
        case Alphabet:
            next_token = scanner(source);
            if(next_token.type == AssignmentOp){
                value = parseValue(source);
                expr = parseExpression(source, value);
                return makeAssignmentNode(token.tok[0], value, expr);
            }
            else{
                printf("Syntax Error: Expect an assignment op %s\n", next_token.tok);
                exit(1);
            }
        case PrintOp:
            next_token = scanner(source);
            if(next_token.type == Alphabet)
                return makePrintNode(next_token.tok[0]);
            else{
                printf("Syntax Error: Expect an identifier %s\n", next_token.tok);
                exit(1);
            }
            break;
        default:
            printf("Syntax Error: Expect a statement %s\n", token.tok);
            exit(1);
    }
}

Statements *parseStatements(FILE * source)
{

    Token token = scanner(source);
    Statement stmt;
    Statements *stmts;

    switch (token.type) {
        case Alphabet:
        case PrintOp:
            stmt = parseStatement(source, token);
            stmts = parseStatements(source);
            return makeStatementTree(stmt , stmts);
        case EOFsymbol:
            return NULL;
        default:
            printf("Syntax Error: Expect statements %s\n", token.tok);
            exit(1);
    }
}

/*********************************************************************
  Build AST
 **********************************************************************/
Declaration makeDeclarationNode(Token declare_type, Token identifier)
{
    Declaration tree_node;

    switch (declare_type.type) {
        case FloatDeclaration:
            tree_node.type = Float;
            break;
        case IntegerDeclaration:
            tree_node.type = Int;
            break;
        default:
            break;
    }

    tree_node.name = identifier.tok[0];

    return tree_node;
}

Declarations *makeDeclarationTree(Declaration decl, Declarations *decls)
{
    Declarations *new_tree = (Declarations *)malloc(sizeof(Declarations));
    new_tree->first = decl;
    new_tree->rest = decls;

    return new_tree;
}


Statement makeAssignmentNode(char id, Expression *v, Expression *expr_tail)
{
    Statement stmt;
    AssignmentStatement assign;

    stmt.type = Assignment;
    assign.id = id;
    if(expr_tail == NULL)
        assign.expr = v;
    else
        assign.expr = expr_tail;
    stmt.stmt.assign = assign;

    return stmt;
}

Statement makePrintNode(char id) 
{
    Statement stmt;
    stmt.type = Print;
    stmt.stmt.variable = id;

    return stmt;
}

Statements *makeStatementTree(Statement stmt, Statements *stmts)
{
    Statements *new_tree = (Statements *)malloc(sizeof(Statements));
    new_tree->first = stmt;
    new_tree->rest = stmts;

    return new_tree;
}

/* parser */
Program parser(FILE *source)
{
    Program program;

    program.declarations = parseDeclarations(source);
    program.statements = parseStatements(source);

    return program;
}

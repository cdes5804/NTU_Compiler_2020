%option noyywrap 
%{
#include <stdio.h>
#include <stdlib.h>
#include "header.h" 

int line_number;
symtab* lookup();
symtab* ptr;
void insertID();
void printSymTab();
void printIdentifier();

/* You need to define for all tokens in C--, here are some examples */

/* reserved words */
#define RW_RETURN       10
#define RW_TYPEDEF      11
#define RW_IF           12
#define RW_ELSE         13
#define RW_INT	        14
#define RW_FLOAT        15
#define RW_FOR          16
#define RW_VOID         17
#define RW_WHILE        18

/* constants */
#define CONST_INT       20
#define CONST_FLOAT     21
#define CONST_STR       22

/* identifiers */
#define ID              30

/* symbols and operators */
#define OP_ADD          40
#define OP_SUB          41
#define OP_DIV          42
#define OP_MUL          43
#define OP_LT           44
#define OP_GT           45
#define OP_GEQ          46
#define OP_LEQ          47
#define OP_NEQ          48
#define OP_EQ           49
#define OP_OR           50
#define OP_AND          51
#define OP_NEG          52
#define OP_ASSIGN       53

/* separators */
#define SEP_LBRACE      60
#define SEP_RBRACE      61
#define SEP_LBRACKET    62
#define SEP_RBRACKET    63
#define SEP_LPAREN      64
#define SEP_RPAREN      65
#define SEP_SEMICOL     66
#define SEP_COMMA       67
#define SEP_DOT         68

#define ERROR           100
%}

letter              [A-Za-z]
digit               [0-9]
ID                  {letter}({letter}|{digit}|"_")*
WS                  [ \t]+
Int_constant        {digit}+

/* Explanation of floating-point literal regex
({digit}+)([eE][+-]?{digit}+)                : 12e5 or 12e+5 or 12e-5
{digit}+(\.{digit}+)([eE][+-]?{digit}+)?     : 12.5 or 12.5e5 or 12.5e+5 or 12.5e-5
({digit}+)?(\.{digit}+)([eE][+-]?{digit}+)?  : .5 or .5e5 or .5e+5 or .5e-5
({digit}+)\.([eE][+-]?{digit}+)?             : 1. or 1.e5 or 1.e+5 or 1.e-5
*/
Float_constant      (({digit}+)([eE][+-]?{digit}+))|({digit}+(\.{digit}+)([eE][+-]?{digit}+)?)|(({digit}+)?(\.{digit}+)([eE][+-]?{digit}+)?)|(({digit}+)\.([eE][+-]?{digit}+)?)

/* reference: https://stackoverflow.com/questions/2039795/regular-expression-for-a-string-literal-in-flex-lex */
String_constant     \"(\\.|[^"\\])*\"

comment             \/\*([^*]|(\*+([^*/])))*\*+\/

OP_add              "+"
OP_sub              "-"
OP_div              "/"
OP_mul              "*"
OP_lt               "<"
OP_gt               ">"
OP_geq              ">="
OP_leq              "<="
OP_neq              "!="
OP_eq               "=="
OP_or               "||"
OP_and              "&&"
OP_neg              "!"
OP_assign           "="

newline             "\n"

SEP_lparen          "("
SEP_rparen          ")"
SEP_lbracket        "["
SEP_rbracket        "]"
SEP_lbrace          "{"
SEP_rbrace          "}"
SEP_comma           ","
SEP_semicol         ";"
SEP_dot             "."

/* Other separators appear here */

error                .

%%

{WS}            {}
{ID}            { 
                    ptr = lookup(yytext);
                    if (ptr == NULL)
                        insertID(yytext);
                    else
                        ptr->counter++;
                }

{OP_assign}     {}  
{OP_or}         {} 
{SEP_lparen}    {} 
{SEP_rparen}    {} 
{SEP_lbrace}    {} 
{SEP_rbrace}    {} 
{SEP_comma}     {}
{SEP_semicol}   {}
{comment}       {printf("%s\n", yytext);}
{newline}       {line_number += 1;}
{error}         {
                    printf("Unrecognized token: %s\n", yytext);
                }


%%

int main(int argc, char **argv)
{
    argc--; ++argv;
    if (argc > 0)
        yyin = fopen(argv[0], "r");
    else
        yyin = stdin;
    yylex();
    printIdentifier();
    return 0;
}

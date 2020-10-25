%option noyywrap 
%{
#include <stdio.h>
#include "header.h" 

int linenumber;
symtab* lookup();
symtab* ptr;
void insertID();
void printSymTab();

/* You need to define for all tokens in C--, here are some examples */

/* reserved words */
#define RW_RETURN       10
#define RW_TYPEDEF      11
#define RW_IF           12
#define RW_ELSE         13
#define RW_INT	         14
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

/* separators */
#define SP_LBRACE       60
#define SP_RBRACE       61
#define SP_LBRACKET     62
#define SP_RBRACKET     63
#define SP_LPAREN       64
#define SP_RPAREN       65
#define SP_SEMICOLON    66
#define SP_COMMA        67
#define SP_DOT          68

#define ERROR           100
%}

letter   [A-Za-z]
digit    [0-9]
ID	 {letter}({letter}|{digit}|"_")*
WS	 [ \t]+
Int_constant {digit}+

/* You need to define the following RE's 
Float_constant
String_constant
comment
*/

OP_assign        "="
OP_or   "||"

/* Other operators appear here */ 


newline  "\n"

DL_lparen "("
DL_rparen ")"
DL_lbrace "{"
DL_rbrace "}"
DL_comma  ","
DL_semicol ";"
DL_dot  "."

/* Other separators appear here */

error    .

%%

{WS}            {}
{RWInt}         {/* return INT; */ }
{RWFloat}       {/* return FLOAT; */}
{ID}            { 
			ptr = lookup(yytext);
			if (ptr == NULL)
			     insertID(yytext);	
			else 
			     ptr->counter++;
		}

{OP_assign}     /* return OP_ASSIGN; */ 
{OP_or}         /* return OP_OR; */
{DL_lparen}     /* return MK_LPAREN; */
{DL_rparen}     /* return MK_RPAREN; */
{DL_lbrace}     /* return MK_LBRACE; */
{DL_rbrace}     /* return MK_RBRACE; */
{DL_comma}      {}
{DL_semicol}    {}
{newline}       linenumber += 1;
{error}         printf("ERR \n");/* return ERROR; */


%%

main(int argc, char **argv)
{
 argc--; ++argv;
   if (argc > 0)
      yyin = fopen(argv[0], "r");
   else
      yyin = stdin;
   yylex();
   printSymTab();
}


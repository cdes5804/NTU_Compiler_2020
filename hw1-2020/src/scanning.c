#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "header.h"

/********************************************* 
  Scanning 
 *********************************************/

Token getNumericToken(FILE *source, char c)
{
    Token token;
    int i = 0;

    while (isdigit(c)) {
        token.tok[i++] = c;
        c = fgetc(source);
    }

    if (c != '.') {
        ungetc(c, source);
        token.tok[i] = '\0';
        token.type = IntValue;
        return token;
    }

    token.tok[i++] = '.';

    c = fgetc(source);
    if (!isdigit(c)) {
        ungetc(c, source);
        printf("Expect a digit : %c\n", c);
        exit(1);
    }

    while (isdigit(c)) {
        token.tok[i++] = c;
        c = fgetc(source);
    }

    ungetc(c, source);
    token.tok[i] = '\0';
    token.type = FloatValue;
    return token;
}

int in_alphabet_set(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || (c == '_');
}

void getString(FILE *source, char c, char tok[])
{
    int len = 0;
    while (in_alphabet_set(c)) {
        tok[len++] = c;
        c = fgetc(source);
    }
    ungetc(c, source);
    tok[len] = '\0';
}

void ungetString(FILE *source, char buf[])
{
    for (int i = strlen(buf)-1; i >= 0; i--)
        ungetc(buf[i], source);
}

Token scanner(FILE *source)
{
    char c;
    Token token;

    while (!feof(source)){
        c = fgetc(source);

        while (isspace(c)) c = fgetc(source);

        if(isdigit(c))
            return getNumericToken(source, c);

        switch (c) {
            case '=':
                token.type = AssignmentOp;
                return token;
            case '+':
                token.type = PlusOp;
                return token;
            case '-':
                token.type = MinusOp;
                return token;
            case '*':
                token.type = MulOp;
                return token;
            case '/':
                token.type = DivOp;
                return token;
            case EOF:
                token.type = EOFsymbol;
                token.tok[0] = '\0';
                return token;
        }

        getString(source, c, token.tok);
        if (token.tok[0] != '\0') {
            if (!strcmp(token.tok, "f"))
                token.type = FloatDeclaration;
            else if (!strcmp(token.tok, "i"))
                token.type = IntegerDeclaration;
            else if (!strcmp(token.tok, "p"))
                token.type = PrintOp;
            else
                token.type = Alphabet;
            return token;
        }
        
        printf("Invalid character : %c\n", c);
        exit(1);
    }

    token.tok[0] = '\0';
    token.type = EOFsymbol;
    return token;
}

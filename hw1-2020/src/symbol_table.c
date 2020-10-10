#include <stdio.h>
#include <stdlib.h>

#include "header.h"

/********************************************************
  Build symbol table
 *********************************************************/
void InitializeTable(SymbolTable *table)
{
    for (int i = 0 ; i < 26; i++)
        table->table[i] = Notype;
}

void add_table(SymbolTable *table, char c, DataType t)
{
    int index = (int)(c - 'a');

    if (table->table[index] != Notype)
        printf("Error : id %c has been declared\n", c);//error
    table->table[index] = t;
}

SymbolTable build(Program program)
{
    SymbolTable table;
    Declarations *decls = program.declarations;
    Declaration current;

    InitializeTable(&table);

    while (decls != NULL) {
        current = decls->first;
        add_table(&table, current.name, current.type);
        decls = decls->rest;
    }

    return table;
}

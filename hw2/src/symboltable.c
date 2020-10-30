#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "header.h"

#define TABLE_SIZE 512
#define RESERVED_SIZE 9

symtab* hash_table[TABLE_SIZE];
extern int line_number;
int identifier_number;

char reserved_words[RESERVED_SIZE][32] = {"return", "typedef", "if", "else", "int", "float", "for", "void", "while"};

int HASH(char* str)
{
    int idx = 0;
    while (*str) {
        idx = idx << 1;
        idx += *str;
        str++;
    }   
    return (idx & (TABLE_SIZE - 1));
}

/*returns the symbol table entry if found else NULL*/

symtab* lookup(char *name)
{
    if (!name)
        return NULL;
    int hash_key = HASH(name);
    symtab* symptr = hash_table[hash_key];

    while (symptr) {
        if (!(strcmp(name, symptr->lexeme)))
            return symptr;
        symptr = symptr->front;
    }
    return NULL;
}

int is_reserved_word(char* name)
{
    for (int i = 0; i < RESERVED_SIZE; ++i) {
        if (!strcmp(name, reserved_words[i]))
            return 1;
    }
    return 0;
}


void insertID(char* name)
{
    int hash_key = HASH(name);
    symtab* ptr = hash_table[hash_key];
    symtab* symptr = (symtab*)malloc(sizeof(symtab));   
    
    if (ptr == NULL) {
        /*first entry for this hash_key*/
        hash_table[hash_key] = symptr;
        symptr->front = NULL;
        symptr->back = symptr;
    } else {
        symptr->front = ptr;
        ptr->back = symptr;
        symptr->back = symptr;
        hash_table[hash_key] = symptr;  
    }
    
    strcpy(symptr->lexeme, name);
    symptr->line = line_number;
    symptr->counter = 1;
    identifier_number += 1;
}

void printSym(symtab* ptr) 
{
    printf(" Name = %s \n", ptr->lexeme);
    printf(" References = %d \n", ptr->counter);
}

void printSymTab()
{
    printf("----- Symbol Table ---------\n");
    for (int i = 0; i < TABLE_SIZE; ++i) {
        symtab* symptr = hash_table[i];
        while (symptr != NULL) {
            printf("====>  index = %d \n", i);
            printSym(symptr);
            symptr = symptr->front;
        }
    }
}

int symtab_cmp(const void* param1, const void* param2)
{
    symtab* left = *(symtab**)param1;
    symtab* right = *(symtab**)param2;
    return strcmp(left->lexeme, right->lexeme);
}

void printIdentifier()
{
    symtab** arr = (symtab**)malloc(sizeof(symtab*) * identifier_number);
    int index = 0;
    if (!arr) {
        fprintf(stderr, "No enough memory to contain the identifiers!\n");
        exit(1);
    }
    for (int i = 0; i < TABLE_SIZE; ++i) {
        symtab* symptr = hash_table[i];
        while (symptr != NULL) {
            arr[index++] = symptr;
            symptr = symptr->front;
        }
    }
    qsort(arr, identifier_number, sizeof(symtab*), symtab_cmp);
    printf("Frequency of identifiers:\n");
    for (int i = 0; i < identifier_number; ++i)
        printf("%-33s%d\n", arr[i]->lexeme, arr[i]->counter);
}

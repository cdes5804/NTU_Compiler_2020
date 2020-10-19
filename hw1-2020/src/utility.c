#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char key[128];
    char val;
} HashTableElement;

typedef struct {
    HashTableElement table[1<<16];
} HashTable;

int __hashString(char *str)
{
    int hv = 0;
    for (char *s = str; *s; s++)
        hv = (hv * 256 + *s) % (1 << 16);
    return hv;
}

char htInsert(HashTable *ht, char *str, char id)
{
    int index = __hashString(str);
    while (ht->table[index].val != '\0') {
        if (!strcmp(str, ht->table[index].key))
            return ht->table[index].val;
        ++index;
    }
    strncpy(ht->table[index].key, str, 128);
    ht->table[index].val = id;
    return '\0';
}

char getId(char *str)
{
    static char id = 'a';
    static HashTable ht;
    char r = htInsert(&ht, str, id);
    if (r != '\0') {
        return r;
    } else if (id > 'z') {
        fprintf(stderr, "The number of different variables exceeds the limit(23)\n");
        exit(1);
    } else {
        return id++;
    }
}

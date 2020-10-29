struct symtab {
	char lexeme[32];
	struct symtab* front;
	struct symtab* back;
	int line;
	int counter;
};

typedef struct symtab symtab;
int is_reserved_word(char* name);
symtab* lookup(char *name);
void insertID(char* name);

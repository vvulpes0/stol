#ifndef STOL_SYMTABLE_H
#define STOL_SYMTABLE_H
struct stnode {
	struct stnode * next;
	char name[16];
	size_t length;
	unsigned int value;
	char type;
};
struct stsymlist {
	struct stnode *head;
	struct stnode *tail;
	size_t size;
};
struct stsymtab {
	struct stsymlist entries[256];
};

void stinit(struct stsymtab *);
void strelease(struct stsymtab *); /* does not free param */
struct stnode * stlookup(struct stsymtab const *, char const *);
void stassign(struct stsymtab *, char const *, unsigned int, char);
#endif

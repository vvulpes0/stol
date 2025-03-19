#ifndef STINSTR_H
#define STINSTR_H
struct stsymtab;
struct stoperand {
	unsigned char reg;
	union {
		int immediate;
		char *name;
	};
	int mode;
	int isName;
};

int stencode(char const *name, int condition,
             struct stoperand *op1, struct stoperand *op2,
             unsigned int *mem, struct stsymtab *tab, unsigned int *lc);

void stencsym(char const *name, unsigned int *mem,
              struct stsymtab *tab, unsigned int *lc);
#endif

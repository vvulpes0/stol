#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include "instrs.h"
#include "symtable.h"

void yyerror(char *);
static int isimm(int);
static void apply(struct stsymtab *, int, char const * const,
                  unsigned int *, unsigned int *);
static int istrcmp(char const *, char const *);

struct insmeta {
	char const * const name;
	unsigned int base;
	unsigned int numops; /* bit 2^n is set if n is a valid number */
	unsigned int dmodes;
	int encodeDMode;
	unsigned int smodes;
	int encodeSMode;
	int oneIsDest;
	int canCondition;
};
static struct insmeta const mnemonics[] = {
	{"add",   0x6000U, 0x4, 0xe, 1, 0xf, 1, 0, 0 },
	{"addc",  0x7000U, 0x4, 0xe, 1, 0xf, 1, 0, 0 },
	{"and",   0xa000U, 0x4, 0xe, 1, 0xf, 1, 0, 0 },
	{"andsr", 0x2800U, 0x4, 0x2, 0, 0xf, 1, 0, 0 },
	{"asl",   0x3400U, 0x4, 0x2, 0, 0x3, 2, 0, 0 },
	{"asr",   0x3600U, 0x4, 0x2, 0, 0x3, 2, 0, 0 },
	{"ba",    0x0400U, 0x2, 0x0, 0, 0xf, 1, 0, 1 },
	{"br",    0x0000U, 0x2, 0x0, 0, 0xf, 1, 0, 1 },
	{"call",  0x0800U, 0x2, 0x0, 0, 0xf, 1, 0, 1 },
	{"cmp",   0x8000U, 0x4, 0xe, 1, 0xf, 1, 0, 0 },
	{"lsr",   0x3200U, 0x4, 0x2, 0, 0x3, 2, 0, 0 },
	{"mov",   0xc000U, 0x4, 0xe, 1, 0xf, 1, 0, 0 },
	{"movsr", 0x2000U, 0x4, 0x2, 0, 0xf, 1, 0, 0 },
	{"neg",   0x3000U, 0x6, 0x2, 0, 0x2, 1, 2, 0 },
	{"nop",   0x0001U, 0x1, 0x0, 0, 0x0, 0, 0, 0 },
	{"or",    0x9000U, 0x4, 0xe, 1, 0xf, 1, 0, 0 },
	{"orsr",  0x2400U, 0x4, 0x2, 0, 0xf, 1, 0, 0 },
	{"pop",   0x1500U, 0x2, 0x2, 0, 0x0, 0, 1, 0 },
	{"pspr",  0x1a00U, 0x2, 0x2, 0, 0x0, 0, 1, 0 },
	{"psprw", 0x1b00U, 0x4, 0x2, 0, 0x2, 0, 0, 0 },
	{"pspw",  0x1900U, 0x2, 0x0, 0, 0x2, 0, 0, 0 },
	{"push",  0x1100U, 0x2, 0x0, 0, 0x2, 0, 0, 0 },
	{"res",   0x0000U, 0x2, 0x0, 0, 0x1, 0, 0, 0 },
	{"ret",   0x0d00U, 0x1, 0x0, 0, 0xf, 1, 0, 1 },
	{"rlc",   0x3c00U, 0x4, 0x2, 0, 0x3, 2, 0, 0 },
	{"rol",   0x3800U, 0x4, 0x2, 0, 0x3, 2, 0, 0 },
	{"ror",   0x3a00U, 0x4, 0x2, 0, 0x3, 2, 0, 0 },
	{"rrc",   0x3e00U, 0x4, 0x2, 0, 0x3, 2, 0, 0 },
	{"rtp",   0x0d0fU, 0x1, 0x0, 0, 0xf, 1, 0, 1 },
	{"sub",   0x4000U, 0x4, 0xe, 1, 0xf, 1, 0, 0 },
	{"subb",  0x5000U, 0x4, 0xe, 1, 0xf, 1, 0, 0 },
	{"trap",  0xd500U, 0x2, 0x0, 0, 0x1, 3, 0, 1 },
	{"xor",   0xb000U, 0x4, 0xe, 1, 0xf, 1, 0, 0 },
	{"xorsr", 0x2c00U, 0x4, 0x2, 0, 0xf, 1, 0, 0 },
};

int
stencode(char const *name, int condition,
         struct stoperand *op1, struct stoperand *op2,
         unsigned int *mem, struct stsymtab *tab, unsigned int *lc)
{
	if (!name || !mem || !tab || !lc) {
		yyerror("internal error: null required args");
		return 0;
	}
	if (istrcmp(name,"halt")==0 && !op1 && !op2) {
		struct stoperand x;
		x.reg = 0;
		x.immediate = *lc;
		x.mode = 0;
		x.isName = 0;
		return stencode("br", condition, &x, op2, mem, tab, lc);
	} else if (istrcmp(name,"halt")==0) {
		yyerror("invalid number of operands");
		return 0;
	}
	if (istrcmp(name,"not")==0 && op1 && !op2) {
		struct stoperand x;
		x.reg = 0;
		x.immediate = 0xffffU;
		x.mode = 0;
		x.isName = 0;
		return stencode("xor",condition, op1, &x, mem, tab, lc);
	} else if (istrcmp(name,"not")==0) {
		yyerror("invalid number of operands");
		return 0;
	}
	int start = 0;
	int end = sizeof(mnemonics)/sizeof(*mnemonics);
	struct insmeta const *meta = mnemonics;
	while (start <= end) {
		int mid = (start + end)/2;
		meta = mnemonics + mid;
		int cmp = istrcmp(name, meta->name);
		if (cmp == 0) {
			break;
		} else if (cmp < 0) {
			end = mid - 1;
		} else if (cmp > 0) {
			start = mid + 1;
		}
	}
	if (start > end) {
		yyerror("invalid instruction mnemonic");
		return 0;
	}
	if (condition >= 0 && !meta->canCondition) {
		yyerror("invalid target for conditioning");
		return 0;
	}
	int numops = (op1?1:0)+(op2?1:0);
	if (!(meta->numops & (1<<numops))) {
		yyerror("invalid number of operands");
		return 0;
	}
	int encoding = meta->base;

	struct stoperand const *dest = NULL;
	struct stoperand *source = NULL;
	if (op1 && op2) {
		dest = op1;
		source = op2;
	} else if (op1 && meta->oneIsDest) {
		dest = op1;
		if (meta->oneIsDest == 2) source = op1;
	} else {
		source = op1;
	}
	/* check modes */
	if (dest && !(meta->dmodes & (1<<dest->mode))) {
		yyerror("bad destination mode");
		return 0;
	}
	if (source && !(meta->smodes & (1<<source->mode))) {
		yyerror("bad source mode");
		return 0;
	}

	/* if `res n', just do that and leave */
	if (istrcmp(name, "res") == 0) {
		for (int i = 0; i < source->immediate; i++) {
			mem[(*lc)++] = 0;
		}
		return 1;
	}

	/* encode modes */
	if (source && meta->encodeSMode) {
		if (meta->encodeSMode == 2) {
			encoding |= ((source->mode)&1)<<8;
		} else {
			encoding |= (source->mode)<<8;
		}
	}
	if (dest && meta->encodeDMode) {
		encoding |= (dest->mode)<<10;
	}

	/* encode condition */
	if (condition >= 0) encoding |= (condition<<4);

	/* encode registers, if present */
	if (source && source->mode != 0) {
		encoding |= source->reg;
	}
	if (dest && dest->mode != 0) {
		encoding |= (dest->reg<<4);
	}
	if (source && meta->encodeSMode==2 && source->mode==0) {
		if (source->isName) {
			yyerror("shift amount must be constant or register");
			return 0;
		}
		int amount = source->immediate;
		if ((amount&0xf)!=amount) {
			yyerror("warning: excess shift truncated");
		}
		encoding |= amount&0xf;
	}

	if (source && meta->encodeSMode==3 && source->mode==0) {
		if (source->isName) {
			yyerror("trap vector must be constant");
			return 0;
		}
		encoding |= ((source->immediate)&(0x7))<<1;
	}

	int brel = !istrcmp(name, "br");
	if (brel && source && source->mode==0 && !source->isName) {
		source->immediate = source->immediate - *lc;
	}

	/* short nonzero immediates are directly embedded */
	int wrotesrc = 0;
	if (source && meta->encodeSMode<2 && source->mode==0
	    && !source->isName
	    && source->immediate
	    && source->immediate == (source->immediate & 0x0f)) {
		encoding |= source->immediate;
		wrotesrc = 1;
	}

	/* write instruction word */
	mem[(*lc)++] = encoding;

	/* if source has extension word immediate, write that */
	if (source && meta->encodeSMode<2 && isimm(source->mode)
	    && !wrotesrc) {
		if (source->isName) {
			apply(tab, brel, source->name, mem, lc);
		} else {
			mem[(*lc)++] = (source->immediate)&0xffff;;
		}
	}

	/* if dest has immediate, write that */
	if (dest && isimm(dest->mode)) {
		if (dest->isName) {
			apply(tab, brel, dest->name, mem, lc);
		} else {
			mem[(*lc)++] = (dest->immediate)&0xffff;
		}
	}
	return 1;
}

static int
isimm(int mode) {
	return mode==0 || mode==3;
}

static void
apply(struct stsymtab * tab, int brel,
      char const * const name, unsigned int *mem, unsigned int *lc)
{
	if (!tab || !name || !mem || !lc) return;
	struct stnode *p = stlookup(tab, name);
	if (p && p->type == 'V') {
		mem[*lc] = brel ? (p->value - (*lc) + 1)&0xffff : p->value;
		(*lc)++;
		return;
	}
	if (p && p->type == 'U') {
		mem[*lc] = p->value;
		p->value = (*lc)++;
		if (brel) p->value = ~(p->value);
		return;
	}
	stassign(tab, name, brel ? ~(*lc) : *lc, 'U');
	mem[(*lc)++] = 0;
}

void
stencsym(char const *name, unsigned int *mem,
         struct stsymtab *tab, unsigned int *lc) {
	apply(tab, 0, name, mem, lc);
}

static int
istrcmp(char const *a, char const *b) {
	if (!a && !b) return 0;
	if (!a) return -1;
	if (!b) return 1;
	while (*a && *b) {
		int x = tolower(*(a++)) - tolower(*(b++));
		if (x) return x;
	}
	if (*a) return 1;
	if (*b) return -1;
	return 0;
}

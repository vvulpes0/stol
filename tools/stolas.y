%{
	#include <stdio.h>
	#include "instrs.h"
	#include "symtable.h"
	int yylex(void);
	void yyerror(char *);
	int mksym(char *, unsigned int);
	extern int startOfLine;
	extern int yyline;
	struct stsymtab tab;
	unsigned int rombase;
	unsigned int rambase;
	unsigned int lc;
	unsigned int maxlc;
	unsigned int obj[65536];
%}
%union {
	int iValue;
	unsigned char reg;
	char *name;
	struct { char *contents; unsigned int length; } str;
	struct stoperand operand;
}
%token <iValue> CONDITION
%token <iValue> INTEGER
%token <reg> REGISTER
%token <name> NAME
%token <str> STRING
%token DIRECTWORD
%token DEFINE
%token SECBSS
%token INVALID
%type <operand> operand
%type <iValue> condition
%type <iValue> integer
%%
program:
	program statement '\n'
	|
	;
statement:
	section
	| define
	| label instruction
	| label
	| instruction
	|
	;
section:
	SECBSS {
		if (lc >= rombase) lc = rambase;
	}
	;
define:
	DEFINE NAME integer {
			if (!mksym($2, $3)) YYERROR;
			free($2);
			$2 = NULL;
		}
	;
label:
	NAME ':' {
			if (!mksym($1, lc)) YYERROR;
			free($1);
			$1 = NULL;
		}
	;
instruction:
	DIRECTWORD intlist
	| NAME condition operand ',' operand {
			if (!stencode($1,$2,&$3,&$5,obj,&tab,&lc)) {
				YYERROR;
			}
			if (lc > maxlc) maxlc = lc;
			free($1);
			$1 = NULL;
			if ($3.isName) { free($3.name); $3.name = NULL; }
			if ($5.isName) { free($5.name); $5.name = NULL; }
		}
	| NAME condition operand {
			if (!stencode($1,$2,&$3,NULL,obj,&tab,&lc)) {
				YYERROR;
			}
			if (lc > maxlc) maxlc = lc;
			free($1);
			$1 = NULL;
			if ($3.isName) { free($3.name); $3.name = NULL; }
		}
	| NAME condition {
			if (!stencode($1,$2,NULL,NULL,obj,&tab,&lc)) {
				YYERROR;
			}
			if (lc > maxlc) maxlc = lc;
			free($1);
			$1 = NULL;
		}
	;
intlist:
	intlist ',' ilint
	| ilint
	;
ilint:
	integer {
			obj[lc++] = $1&0xffff;
			if (lc > maxlc) maxlc = lc;
		}
	| NAME {
			stencsym($1,obj,&tab,&lc);
			if (lc > maxlc) maxlc = lc;
			free($1);
			$1 = NULL;
		}
	| STRING {
			for (unsigned int i = 0; i < $1.length; i++) {
				obj[lc++] = 0xffff & $1.contents[i];
				if (lc > maxlc) maxlc = lc;
			}
			free($1.contents);
			$1.contents = NULL;
			$1.length = 0;
	}
	;
condition:
	CONDITION
	| { $$ = -1; }
	;
operand:
	integer {
			$$.immediate = $1;
			$$.isName = 0;
			$$.mode = 0;
		}
	| NAME {
			$$.name = $1;
			$$.isName = 1;
			$$.mode = 0;
		}
	| REGISTER { $$.reg = $1; $$.mode = 1; $$.isName = 0; }
	| '(' REGISTER ')' {
			$$.reg = $2;
			$$.mode = 2;
			$$.isName = 0;
		}
	| '(' REGISTER '+' INTEGER ')'  {
			$$.reg = $2;
			$$.immediate = $4;
			$$.isName = 0;
			$$.mode = 3;
			$$.isName = 0;
		}
	| '(' REGISTER '+' NAME ')'  {
			$$.reg = $2;
			$$.name = $4;
			$$.isName = 1;
			$$.mode = 3;
			$$.isName = 0;
		}
	| '(' REGISTER '-' INTEGER ')'  {
			$$.reg = $2;
			$$.immediate = -$4;
			$$.isName = 0;
			$$.mode = 3;
			$$.isName = 0;
		}
	;
integer:
	'@' { $$ = lc; }
	| '@' '+' INTEGER { $$ = lc + $3; }
	| '@' '-' INTEGER { $$ = lc - $3; }
	| INTEGER '+' '@' { $$ = $1 + lc; }
	| INTEGER '-' '@' { $$ = $1 - lc; }
	| INTEGER
	| '-' INTEGER { $$ = -$2; }
%%
void yyerror(char *s) {
	fprintf(stderr, "%d: %s\n", yyline - startOfLine, s);
}
int mksym(char *name, unsigned int value) {
	struct stnode *p = stlookup(&tab, name);
	if (p && p->type == 'V') {
		char outbuf[256];
		snprintf(outbuf, sizeof(outbuf),
		         "%d: "
		         "multiple definitions of symbol"
		         " '%s'", yyline, name);
		yyerror(outbuf);
		return 0;
	}
	if (p) {
		unsigned int x = p->value;
		unsigned int loc = (x&0x10000)?~x:x;
		while (x) {
			unsigned int t = obj[loc];
			if (x&0x10000) {
				obj[loc] = value - loc + 1;
			} else {
				obj[loc] = value;
			}
			x = t;
			loc = (x&0x10000)?~x:x;
		}
	}
	stassign(&tab, name, value, 'V');
	return 1;
}
int main(void) {
	startOfLine = 1;
	yyline = 1;
	rambase = 0x0200U;
	rombase = lc = maxlc = 0x8000U;
	stinit(&tab);
	if (yyparse() != 0) {
		strelease(&tab);
		return 1;
	}
	int ok = 1;
	for (int i = 0;
	     i < sizeof(tab.entries)/sizeof(tab.entries[0]);
	     i++) {
		struct stnode *p = tab.entries[i].head;
		while (p) {
			if (p->type == 'U') {
				fprintf(stderr, "%s: `%s'\n",
				        "undefined symbol",
				        p->name);
				ok = 0;
			}
			p = p->next;
		}
	}
	if (!ok) {
		strelease(&tab);
		return 1;
	}
	printf("v3.0 hex words addressed");
	for (unsigned int i = rombase; i < maxlc; i++) {
		if (0 == (i-rombase)%8) {
			printf("\n%04x:", i-rombase);
		}
		printf(" %04x", obj[i]);
	}
	printf("\n");
}

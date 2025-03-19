%{
#include <stdlib.h>
void yyerror(char *);
#include "instrs.h"
#include "y.tab.h"
int regnum(char *s);
int yyline;
int startOfLine;
char buf[1024];
char *s;
%}
%x QSTRING
%%
\"	{ BEGIN(QSTRING); startOfLine = 0; s = buf; }
<QSTRING>\\a	*(s++) = '\a';
<QSTRING>\\b	*(s++) = '\b';
<QSTRING>\\f	*(s++) = '\f';
<QSTRING>\\n	*(s++) = '\n';
<QSTRING>\\r	*(s++) = '\r';
<QSTRING>\\t	*(s++) = '\t';
<QSTRING>\\v	*(s++) = '\v';
<QSTRING>\\\\	*(s++) = '\\';
<QSTRING>\\\'	*(s++) = '\'';
<QSTRING>\\\"	*(s++) = '\"';
<QSTRING>\"	{
		*s = 0;
		BEGIN 0;
		yylval.str.length = s - buf;
		yylval.str.contents = malloc(yylval.str.length + 1);
		memcpy(yylval.str.contents, buf, yylval.str.length + 1);
		return STRING;
	}
<QSTRING>\\([0-7]{1,3}) {
		startOfLine = 0;
		*s = 0;
		for (int i = 1; i < yyleng; i++) {
			*s *= 8;
			*s += yytext[i] - '0';
		}
		++s;
	 }
<QSTRING>\n	{
		++yyline;
		startOfLine = 1;
		yyerror("expected \"");
		return INVALID;
	}
<QSTRING>.	*(s++) = *yytext;
\n	{ ++yyline; startOfLine = 1; return *yytext; }
[/][dD][eE][fF][iI][nN][eE] { startOfLine = 0; return DEFINE; }
[/][bB][sS][sS] { startOfLine = 0; return SECBSS; }
dw { startOfLine = 0; return DIRECTWORD; }
\'\\a\' { startOfLine = 0; yylval.iValue = '\a'; return INTEGER; }
\'\\b\' { startOfLine = 0; yylval.iValue = '\b'; return INTEGER; }
\'\\f\' { startOfLine = 0; yylval.iValue = '\f'; return INTEGER; }
\'\\n\' { startOfLine = 0; yylval.iValue = '\n'; return INTEGER; }
\'\\r\' { startOfLine = 0; yylval.iValue = '\r'; return INTEGER; }
\'\\t\' { startOfLine = 0; yylval.iValue = '\t'; return INTEGER; }
\'\\v\' { startOfLine = 0; yylval.iValue = '\v'; return INTEGER; }
\'\\\\\' { startOfLine = 0; yylval.iValue = '\\'; return INTEGER; }
\'\\\'\' { startOfLine = 0; yylval.iValue = '\''; return INTEGER; }
\'\\\"\' { startOfLine = 0; yylval.iValue = '\"'; return INTEGER; }
\'\\([0-7]{1,3})\' {
		startOfLine = 0;
		yylval.iValue = 0;
		for (int i = 2; i < yyleng; i++) {
			yylval.iValue *= 8;
			yylval.iValue += yytext[i] - '0';
		}
		return INTEGER;
	 }
\'[^\'\\]\' { startOfLine = 0; yylval.iValue = yytext[1]; return INTEGER; }
[-(),:+@] { startOfLine = 0; return *yytext; }
[.][uU]<= { startOfLine = 0; yylval.iValue = 5; return CONDITION; }
[.]([cC][cC]|[uU]>=) { startOfLine = 0; yylval.iValue = 9; return CONDITION; }
[.]([cC]|[uU]<) { startOfLine = 0; yylval.iValue = 1; return CONDITION; }
[.]([nN][vV]) { startOfLine = 0; yylval.iValue = 10; return CONDITION; }
[.]([nN][zZ]|!=) { startOfLine = 0; yylval.iValue = 12; return CONDITION; }
[.][fF] { startOfLine = 0; yylval.iValue = 8; return CONDITION; }
[.][nN] { startOfLine = 0; yylval.iValue = 3; return CONDITION; }
[.][vV] { startOfLine = 0; yylval.iValue = 2; return CONDITION; }
[.]([zZ]|=) { startOfLine = 0; yylval.iValue = 4; return CONDITION; }
[.][sS]>= { startOfLine = 0; yylval.iValue = 14; return CONDITION; }
[.][sS]<= { startOfLine = 0; yylval.iValue = 7; return CONDITION; }
[.][sS]> { startOfLine = 0; yylval.iValue = 15; return CONDITION; }
[.][sS]< { startOfLine = 0; yylval.iValue = 6; return CONDITION; }
[.][pP] { startOfLine = 0; yylval.iValue = 11; return CONDITION; }
[.][uU]> { startOfLine = 0; yylval.iValue = 13; return CONDITION; }
[.] { startOfLine = 0; yylval.iValue = 0; return CONDITION; }
[[:alpha:]_][[:alnum:]_]* {
		int i;
		startOfLine = 0;
		if ((i = regnum(yytext)) < 0) {
			yylval.name = strdup(yytext);
			return NAME;
		}
		yylval.reg = i;
		return REGISTER;
	}
0x[0-9A-Fa-f]+ {
		startOfLine = 0;
		yylval.iValue = 0;
		int overlarge = 0;
		for (int i = 2; i < yyleng; ++i) {
			yylval.iValue *= 16;
			if (yytext[i] >= '0' && yytext[i] < '9') {
				yylval.iValue += yytext[i] - '0';
			} else if (yytext[i] >= 'A' && yytext[i] < 'F') {
				yylval.iValue += yytext[i] - 'A' + 10;
			} else {
				yylval.iValue += yytext[i] - 'a' + 10;
			}
			if ((unsigned int)(yylval.iValue) > 65535U) {
				overlarge = 1;
			}
		}
		if (overlarge) {
			fprintf(stderr,
			        "%d: warning: overlarge constant "
			        "\"%s\" truncated\n",
			        yyline, yytext);
		}
		return INTEGER;
	}
0[0-7]* {
		startOfLine = 0;
		yylval.iValue = 0;
		int overlarge = 0;
		for (int i = 1; i < yyleng; ++i) {
			yylval.iValue *= 8;
			yylval.iValue += yytext[i] - '0';
			if ((unsigned int)(yylval.iValue) > 65535U) {
				overlarge = 1;
			}
		}
		if (overlarge) {
			fprintf(stderr,
			        "%d: warning: overlarge constant "
			        "\"%s\" truncated\n",
			        yyline, yytext);
		}
		return INTEGER;
	}
[1-9][0-9]* {
		startOfLine = 0;
		yylval.iValue = 0;
		int overlarge = 0;
		for (int i = 0; i < yyleng; ++i) {
			yylval.iValue *= 10;
			yylval.iValue += yytext[i] - '0';
			if ((unsigned int)(yylval.iValue) > 65535U) {
				overlarge = 1;
			}
		}
		if (overlarge) {
			fprintf(stderr,
			        "%d: warning: overlarge constant "
			        "\"%s\" truncated\n",
			        yyline, yytext);
		}
		return INTEGER;
	}
[[:space:]]	; /* skip whitespace */
[;].*$	; /* semicolon is comment char */
.	{ startOfLine = 0; yyerror("tokenization error"); return INVALID; }
%%
int yywrap(void) {
	return 1;
}
int regnum(char *s) {
	if (s[0] == 'r' || s[0] == 'R') {
		int i = 1;
		for (; s[i]; i++) {
			if (s[i] < '0' || s[i] > '9') return -1;
		}
		i = atoi(s+1);
		if (i < 16) return i;
		return -1;
	}
	if (s[0] == 'f' || s[0] == 'F') {
		if (s[1] == 'p' || s[1] == 'P') return 14;
	}
	if (s[0] == 's' || s[0] == 'S') {
		if (s[1] == 'p' || s[1] == 'P') return 15;
	}
	return -1;
}

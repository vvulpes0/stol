#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "symtable.h"

static int hash(char const *);
static void releaselist(struct stsymlist *);

void
stinit(struct stsymtab *tab) {
	int i;
	struct stsymlist *p;
	if (!tab) return;
	for (i = 0, p = tab->entries;
	     i < sizeof(tab->entries)/sizeof(tab->entries[0]);
	     i++, p++) {
		p->head = p->tail = NULL;
		p->size = 0;
	}
}

void
strelease(struct stsymtab *tab) {
	int i;
	struct stsymlist *p;
	if (!tab) return;
	for (i = 0, p = tab->entries;
	     i < sizeof(tab->entries)/sizeof(tab->entries[0]);
	     i++, p++) {
		releaselist(p);
	}
}

static void
releaselist(struct stsymlist * syms) {
	struct stnode *p;
	struct stnode *t;
	if (!syms) return;
	p = syms->head;
	while (p) {
		t = p->next;
		free(p);
		p = t;
	}
	syms->head = syms->tail = NULL;
}

struct stnode *
stlookup(struct stsymtab const *tab, char const *s) {
	struct stsymlist syms;
	struct stnode *p;
	int len = strlen(s);
	if (!tab) return NULL;
	syms = tab->entries[hash(s)];
	for (p = syms.head; p; p = p->next) {
		if (len != p->length) continue;
		if (strncmp(s, p->name, sizeof(p->name)) != 0) continue;
		return p;
	}
	return NULL;
}

void
stassign(struct stsymtab *tab, char const *s,
         unsigned int value, char type) {
	struct stsymlist *syms;
	struct stnode *p = stlookup(tab, s);
	if (!tab) return;
	int len = strlen(s);
	if (!p) {
		p = malloc(sizeof(*p));
		if (!p) return; /* error?? */
		p->next = NULL;
		syms = &(tab->entries[hash(s)]);
		if (!syms->head) {
			syms->head = syms->tail = p;
		} else {
			syms->tail->next = p;
			syms->tail = p;
		}
	}
	strncpy(p->name, s, sizeof(p->name));
	p->length = len > sizeof(p->name) ? sizeof(p->name) : len;
	p->value = value;
	p->type = type;
}

static int
hash(char const *s) {
	unsigned int sum = 0;
	int i = 0;
	while (s[i]) {
		sum *= 13;
		sum += s[i]*i;
		sum %= 256;
		i++;
	}
	return (int)sum;
}

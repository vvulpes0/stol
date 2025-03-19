.SUFFIXES :
.SUFFIXES : .c .o
.c.o :
	$(CC) $(CFLAGS) -c -o $@ $<
stolas : lex.yy.o y.tab.o symtable.o instrs.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(LDLIBS) -o $@ $^
y.tab.h y.tab.c : stolas.y
	yacc -d $^
lex.yy.c : stolas.lex
	lex $^

instrs.o : instrs.c instrs.h symtable.h
symtable.o : symtable.c symtable.h
y.tab.o : y.tab.c instrs.h symtable.h
lex.yy.o : lex.yy.c y.tab.h instrs.h

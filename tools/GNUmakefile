.SUFFIXES :
.SUFFIXES : .c .o
.c.o :
	$(CC) $(CFLAGS) -c -o $@ $<
.o :
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

.MAIN : all
.PHONY : all
all : stolas stole

stolas : lex.yy.o y.tab.o symtable.o instrs.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)
y.tab.h y.tab.c : stolas.y
	yacc -d $^
lex.yy.c : stolas.lex
	lex $^

instrs.o : instrs.c instrs.h symtable.h
symtable.o : symtable.c symtable.h
y.tab.o : y.tab.c instrs.h symtable.h
lex.yy.o : lex.yy.c y.tab.h instrs.h

#
# Makefile for TINY
# Gnu C Version
# K. Louden 2/3/98
#

CC = gcc

CFLAGS =

OBJS = main.o util.o scan.o symtab.o analyze.o #parse.o code.o cgen.o

tiny: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o tiny

main.o: main.c globals.h util.h scan.h parse.h analyze.h cgen.h
	$(CC) $(CFLAGS) -c main.c

util.o: util.c util.h globals.h
	$(CC) $(CFLAGS) -c util.c

scan.o: scan.c scan.h util.h globals.h
	$(CC) $(CFLAGS) -c scan.c

#parse.o: parse.c parse.h scan.h globals.h util.h
#	$(CC) $(CFLAGS) -c parse.c

symtab.o: symtab.c symtab.h
	$(CC) $(CFLAGS) -c symtab.c

analyze.o: analyze.c globals.h symtab.h analyze.h
	$(CC) $(CFLAGS) -c analyze.c

#code.o: code.c code.h globals.h
#	$(CC) $(CFLAGS) -c code.c

#cgen.o: cgen.c globals.h symtab.h code.h cgen.h
#	$(CC) $(CFLAGS) -c cgen.c

tm: tm.c
	$(CC) $(CFLAGS) tm.c -o tm


#by flex
OBJS_FLEX = main.o util.o lex.yy.o

cminus_flex: $(OBJS_FLEX)
	$(CC) $(CFLAGS) $(OBJS_FLEX) -o cminus_flex -lfl

lex.yy.o: cminus.l scan.h util.h globals.h
	flex cminus.l
	$(CC) $(CFLAGS) -c lex.yy.c -lfl


#by yacc, flex
OBJS_YACC = y.tab.o main.o util.o lex.yy.o symtab.o analyze.o

cminus_yacc: $(OBJS_YACC)
	$(CC) $(CFLAGS) $(OBJS_YACC) -o cminus_yacc -lfl

y.tab.o: cminus.y scan.h util.h globals.h parse.h symtab.h analyze.h
	yacc -d --debug cminus.y
	$(CC) $(CFLAGS) -c y.tab.c -lfl


all: tiny tm cminus_flex cminus_yacc


clean:
	-rm tiny
	-rm tm
	-rm $(OBJS)
	-rm lex.yy.*
	-rm y.tab.*
	-rm cminus_flex
	-rm cminus_yacc

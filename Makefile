LEX=lex
YACC=yacc -vd

CC=gcc

amircc: y.tab.o lex.yy.o symbol_table.o ast.o
	$(CC) -o amircc y.tab.o lex.yy.o symbol_table.o ast_nodes.o ast.o -lfl


symbol_table.o: symbol_table.c
	$(CC) -c -o $@ $<

ast_nodes.o: ast_nodes.c
	$(CC) -c -o $@ $<

ast.o: ast.c
	$(CC) -c -o $@ $<
	
lex.yy.o: lex.yy.c y.tab.h
lex.yy.o y.tab.o: amircc.h


y.tab.c y.tab.h: parser.y symbol_table.o ast_nodes.o ast.o
	$(YACC) parser.y

lex.yy.c: tokenizer.lex
	$(LEX) tokenizer.lex

clean:
	rm *.o lex.yy.c *.tab.* amircc *.output

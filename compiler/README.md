yacc -d kim.y
lex kim.l
cc -w -g y.tab.c lex.yy.c syntax.c print.c semantic.c print_sem.c gen.c

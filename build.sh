#!/bin/bash

bison -dvt -Wall -o koala_yacc.c yacc/koala.y
flex -o koala_lex.c yacc/koala.l
gcc -g -std=gnu99 -Wall -o koalac koala_yacc.c koala_lex.c koalac.c ast.c \
  symtable.c compile.c -I. -L. -lkoala

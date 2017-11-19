#!/bin/bash

rm *.a

gcc -c -g -std=c99 -Wall \
hash_table.c hash.c vector.c \
object.c nameobject.c tupleobject.c tableobject.c methodobject.c \
structobject.c \
kstate.c

ar -r libkoala.a \
hash_table.o hash.o vector.o \
object.o nameobject.o tupleobject.o tableobject.o methodobject.o \
structobject.o \
kstate.o

rm *.o

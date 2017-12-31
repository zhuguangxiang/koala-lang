#!/bin/bash

rm *.a

gcc -c -g -std=c99 -W -Wall \
debug.c hash_table.c hash.c vector.c

ar -r libkoala.a \
debug.o hash_table.o hash.o vector.o

rm *.o

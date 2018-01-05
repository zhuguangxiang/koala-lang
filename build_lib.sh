#!/bin/bash

rm *.a

gcc -c -g -std=c99 -W -Wall \
debug.c hashtable.c hash.c vector.c

ar -r libkoala.a \
debug.o hashtable.o hash.o vector.o

rm *.o

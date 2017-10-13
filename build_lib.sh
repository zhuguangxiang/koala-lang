#!/bin/bash

rm *.a

gcc -c -g -std=c99 -Wall hash_table.c hash.c vector.c namei.c koala.c \
object.c gstate.c \
module_object.c method_object.c tuple_object.c \
integer_object.c string_object.c io_module.c sys_module.c

ar -r libkoala.a hash_table.o hash.o vector.o namei.o koala.o object.o \
gstate.o module_object.o method_object.o tuple_object.o integer_object.o \
string_object.o io_module.o sys_module.o

rm *.o

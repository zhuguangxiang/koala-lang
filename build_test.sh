#!/bin/bash

# valgrind ./a.out

gcc -g -Wall src/mm.c src/hashmap.c test/test_hashmap.c -I./include
./a.out

gcc -g -Wall src/mm.c src/vector.c test/test_vector.c -I./include
./a.out

gcc -g -Wall test/test_list.c -I./include
./a.out

gcc -g -Wall src/mm.c src/gc.c test/test_gc.c src/vector.c src/hashmap.c -I./include
./a.out

gcc -g -Wall test/test_typeobject.c src/typeobject.c src/vector.c src/mm.c src/hashmap.c -I./include
./a.out

gcc -rdynamic -g -Wall -fvisibility=hidden test/test_stringobject.c src/stringobject.c src/typeobject.c src/vector.c src/mm.c src/hashmap.c src/gc.c src/methodobject.c  -I./include -ldl
./a.out

gcc -rdynamic -g -Wall -fvisibility=hidden  test/test_state.c src/state.c src/typeobject.c src/vector.c src/mm.c src/gc.c src/hashmap.c src/stringobject.c src/methodobject.c -I./include -ldl./a.out

gcc -rdynamic -g -Wall -fvisibility=hidden test/test_mixin.c src/stringobject.c src/typeobject.c src/vector.c src/mm.c src/hashmap.c src/gc.c src/methodobject.c  -I./include -ldl

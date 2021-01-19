/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangXiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "object.h"

/*
 * cc -g test/test_typeobject.c src/typeobject.c src/vector.c src/mm.c
 * src/hashmap.c -I./include
 */
void test_typeobject(void)
{
    init_core_types();
}

int main(int argc, char *argv[])
{
    test_typeobject();
    return 0;
}

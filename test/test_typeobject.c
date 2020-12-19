/*===-- test_typeobject.c -----------------------------------------*- C -*-===*\
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) 2020 James, https://github.com/zhuguangxiang                 *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* Test typeobject in `object.h` and `typeobject.c`                           *|
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

/*===-- test_stringobject.c ---------------------------------------*- C -*-===*\
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) 2020 James, https://github.com/zhuguangxiang                 *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* Test stringobject in `stringobject.h` and `stringobject.c`                 *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "methodobject.h"
#include "stringobject.h"
#include <dlfcn.h>

void test_stringobject(void)
{
    init_core_types();
    init_string_type();
    init_method_type();
    Object *sobj = string_new("hello, world");
    // Object *meth = load_function(sobj, "length");
    int (*f)(void *) = dlsym(NULL, "kl_string_length");
    printf("len:%d\n", f(sobj));
}

int main(int argc, char *argv[])
{
    gc_init();
    test_stringobject();
    gc();
    gc_fini();
    return 0;
}

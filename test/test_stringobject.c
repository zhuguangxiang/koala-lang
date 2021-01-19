/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangXiang https://github.com/zhuguangxiang               *|
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

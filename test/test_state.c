/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangXiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "koala.h"
#include <assert.h>
#include <stdio.h>

void test_state(void)
{
    KoalaState *ks = koala_newstate();
    koala_pushint(ks, 100);
    koala_pushint(ks, 200);
    int stksize = koala_gettop(ks);
    assert(stksize == 2);
    int64_t ival1, ival2;
    ival1 = koala_toint(ks, -1);
    assert(ival1 == 200);
    ival2 = koala_toint(ks, -2);
    assert(ival2 == 100);
    koala_pushint(ks, ival1 + ival2);
    stksize = koala_gettop(ks);
    assert(stksize == 3);
    koala_pushstr(ks, "Jack&Rose");
    stksize = koala_gettop(ks);
    assert(stksize == 4);
}

int main(int argc, char *argv[])
{
    gc_init();
    test_state();
    gc();
    gc_fini();
    return 0;
}

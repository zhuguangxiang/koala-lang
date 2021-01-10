/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangxiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "object.h"

void test_tvalue_layout(void)
{
    TValueRef val = {};
    val.tag = 1;
    val.kind = 'i';
    assert(sizeof(val) == 2 * sizeof(void *));

    TValueRef obj = {};
    obj.vtbl = &obj;
    assert(obj.tag == 0 && obj.kind != 0);
}

int main(int argc, char *argv[])
{
    test_tvalue_layout();
    return 0;
}

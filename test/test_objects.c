/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "core/core.h"
#include "gc/gc.h"

uintptr any_tostr(uintptr self);

int main(int argc, char *argv[])
{
    gc_init(512);
    init_core_pkg();

    uintptr s = string_new("hello");
    string_show(s);
    int slot = type_get_func_slot(__GET_TYPEINFO(s), "__str__");
    assert(slot == 3);
    FuncNode *fn = object_get_func(s, slot);
    assert(fn->ptr != any_tostr);
    uintptr s2 = ((uintptr(*)(uintptr))fn->ptr)(s);
    string_show(s2);

    gc_fini();
    return 0;
}

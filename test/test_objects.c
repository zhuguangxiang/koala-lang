/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "core/core.h"
#include "gc/gc.h"

#ifdef __cplusplus
extern "C" {
#endif

int main(int argc, char *argv[])
{
    gc_init(512);
    init_core();

    uintptr s = 0, s2 = 0, clazz = 0;
    GC_STACK(3);
    gc_push(&s, 0);
    gc_push(&s2, 1);
    gc_push(&clazz, 2);

    s = string_new("hello");
    string_show(s);

    int slot = type_get_func_slot(__GET_TYPE(s), "__str__");
    assert(slot == 3);
    FuncNode *fn = object_get_func(s, slot);
    s2 = ((uintptr(*)(uintptr))fn->ptr)(s);
    string_show(s2);

    slot = type_get_func_slot(__GET_TYPE(s), "__class__");
    fn = object_get_func(s, slot);
    clazz = ((uintptr(*)(uintptr))fn->ptr)(s);

    slot = type_get_func_slot(__GET_TYPE(clazz), "__str__");
    fn = object_get_func(clazz, slot);
    s2 = ((uintptr(*)(uintptr))fn->ptr)(clazz);
    string_show(s2);

    gc();

    slot = type_get_func_slot(__GET_TYPE(clazz), "__name__");
    fn = object_get_func(clazz, slot);
    s2 = ((uintptr(*)(uintptr))fn->ptr)(clazz);
    string_show(s2);

    gc();

    gc_fini();
    return 0;
}

#ifdef __cplusplus
}
#endif

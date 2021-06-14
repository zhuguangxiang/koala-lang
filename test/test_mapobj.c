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
    gc_init(1024);

    init_core();

    GC_STACK(1);
    objref map = map_new(0);
    gc_push(&map, 0);

    map_put_absent(map, 100, 1000);
    map_put_absent(map, 101, 1001);
    map_put_absent(map, 102, 1002);
    map_put_absent(map, 103, 1003);
    map_put_absent(map, 104, 1004);
    map_put_absent(map, 105, 1005);
    map_put_absent(map, 106, 1006);
    map_put_absent(map, 107, 1007);
    map_put_absent(map, 108, 1008);
    map_put_absent(map, 109, 1009);
    map_put_absent(map, 110, 1010);
    map_put_absent(map, 111, 1011);
    map_put_absent(map, 112, 1012);

    anyref val = 0;
    map_put(map, 100, 100, &val);
    assert(val == 1000);

    gc();

    map_put_absent(map, 200, 2000);
    bool ret = map_put_absent(map, 200, 2001);
    assert(ret == 0);

    gc();

    map_get(map, 100, &val);
    assert(val == 100);

    map_get(map, 200, &val);
    assert(val == 2000);

    map_remove(map, 200, nil);
    ret = map_get(map, 200, &val);
    assert(ret == 0);

    map_remove(map, 100, nil);
    map_remove(map, 101, nil);
    map_remove(map, 102, nil);
    map_remove(map, 103, nil);
    map_remove(map, 104, nil);
    map_remove(map, 105, nil);
    map_remove(map, 106, nil);
    map_remove(map, 107, nil);
    map_remove(map, 108, nil);
    map_remove(map, 109, nil);
    map_remove(map, 200, nil);
    map_remove(map, 300, nil);

    gc();

    gc_pop();

    gc();

    gc_fini();
    return 0;
}

#ifdef __cplusplus
}
#endif

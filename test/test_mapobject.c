/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 */

#include "gc/gc.h"
#include "vm/mapobject.h"

#ifdef __cplusplus
extern "C" {
#endif

int main(int argc, char *argv[])
{
    gc_init(1000);

    GC_STACK(1);
    ObjectRef map = map_new(0, 0);
    gc_push1(&map);

    gc();

    map_put_absent(map, 100, 101);
    map_put_absent(map, 101, 101);
    map_put_absent(map, 102, 101);
    map_put_absent(map, 103, 101);
    map_put_absent(map, 104, 101);
    map_put_absent(map, 105, 101);
    map_put_absent(map, 106, 101);
    map_put_absent(map, 107, 101);
    map_put_absent(map, 108, 101);
    map_put_absent(map, 109, 101);
    map_put_absent(map, 110, 101);
    map_put_absent(map, 111, 101);
    map_put(map, 100, 100, NULL);

    gc();

    map_put_absent(map, 200, 201);
    int32 ret = map_put_absent(map, 200, 200);
    assert(ret == -1);

    gc();

    uintptr_t val = 0;
    map_get(map, 100, &val);
    assert(val == 100);
    map_get(map, 200, &val);
    assert(val == 201);

    map_remove(map, 200, NULL);
    ret = map_get(map, 200, &val);
    assert(ret == -1);
    gc();

    map_get(map, 100, &val);
    assert(val == 100);

    gc_pop();
    gc();
    gc_fini();
    return 0;
}

#ifdef __cplusplus
}
#endif

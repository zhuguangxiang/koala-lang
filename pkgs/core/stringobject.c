/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 */

#include "core.h"
#include "gc/gc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _StringObject StringObject;

struct _StringObject {
    OBJECT_HEAD
    char *gcstr;
};

static TypeInfo string_type = {
    .name = "string",
    .flags = TF_CLASS | TF_FINAL,
};

static int __string_objmap[] = {
    1,
    offsetof(StringObject, gcstr),
};

void init_string_type(void)
{
    type_ready(&string_type);
    pkg_add_type("/", &string_type);
}

Object *string_new(char *s)
{
    int len = strlen(s);
    StringObject *sobj = gc_alloc(sizeof(*sobj), __string_objmap);
    GC_STACK(1);
    gc_push(&sobj, 0);

    void *gcstr = gc_alloc_string(len + 1);
    memcpy(gcstr, s, len);
    sobj->type = &string_type;
    sobj->gcstr = gcstr;
    gc_pop();
    return (Object *)sobj;
}

#ifdef __cplusplus
}
#endif

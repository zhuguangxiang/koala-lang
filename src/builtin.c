/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "eval.h"

#ifdef __cplusplus
extern "C" {
#endif

static void object_print(BytesObject *buf, Value *val) {}

typedef struct _MethodCache {
    TypeObject *type;
    const char *name;
    Object *method;
} MethodCache;

typedef struct _CallSiteCache {
    int avail;
    MethodCache caches[8];
} CallSiteCache;

static Object *lookup_method_from_cache(CallSiteCache *cache, Object *self,
                                        const char *name)
{
    if (!cache->avail) {
    }
}

/*
pub func print(objs ..., sep = ' ', end = '\n', file io.Writer = none)
*/
static Value builtin_print(Value *module, Value *args, int nargs, Object *kwargs)
{
    char *sep = " ";
    Value r = kl_dict_get(kwargs, "sep");
    if (!IS_NONE(r)) {
        Object *s = r.obj;
        ASSERT(string_check(s));
        sep = STRING_DATA(s);
    }

    for (int i = 0; i < nargs; i++) {
        if (i != 0) {
            object_print(buf, sep);
        }
        object_format(buf, "", args + i);
    }

    object_print(buf, "\n");

    Object *writer = sys_stdout;
    r = kl_dict_get(kwargs, "file");
    if (!IS_NONE(r)) {
        writer = r.obj;
    }

    static CallSiteCache cache;
    call_method_by_cache(&cache, writer, "write", buf);

    return NoneValue;
}

static MethodDef builtin_methods[] = {
    { "print", builtin_print, "...|sep:s,end:s,file:Lio.Writer;", "" },
    { NULL },
};

#ifdef __cplusplus
}
#endif

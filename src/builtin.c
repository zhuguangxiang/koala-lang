/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "moduleobject.h"
#include "object.h"
#include "stringobject.h"
#include "tracestack.h"
#include "tupleobject.h"

#ifdef __cplusplus
extern "C" {
#endif

#if 0
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

// kl_call_lookup
// kl_call_cache
// kl_call_index
static Object *lookup_method_from_cache(CallSiteCache *cache, Object *self,
                                        const char *name)
{
    if (!cache->avail) {
    }
}

Value object_call_index_method_arg(Object *self, int index, Object *arg)
{
    Value args[] = { self, arg };
    object_call(meth, args, 2);
}

int object_format(Object *buf, const char *fmt, Object *obj)
{
    Object *fmt = kl_new_format(buf, fmt);
    INIT_TRACE_STACK(1);
    TRACE_STACK_PUSH(fmt);
    Value r = object_call_index_method(obj, 2, fmt);
    FINI_TRACE_STACK();
    return 0;
}

#endif

/*
public func print(objs ..., sep = ' ', end = '\n', file io.Writer = none)
*/
static Value builtin_print(Value *module, Value *args, int nargs, Object *names)
{
    char *sep = " ";
    char *end = "\n";
    Object *file = NULL;

    kl_parse_names(args, nargs, names, { "sep", "end", "file" }, "ssO", &sep, &end,
                   &file);

    Value r = kl_dict_get(kwargs, "sep");
    if (!IS_NONE(r)) {
        Object *s = VALUE_AS_OBJECT(r);
        ASSERT(string_check(s));
        sep = STRING_DATA(s);
    }

    Object *buf = kl_new_buf(0);

    for (int i = 0; i < nargs; i++) {
        if (i != 0) {
            object_print(buf, sep);
        }
        object_format(buf, "%s", args + i);
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
    { "print", builtin_print, METH_VAR_NAMES, "...|sep:s,end:s,file:Lio.Writer;", "" },
    { NULL },
};

static ModuleDef builtin_module = {
    .name = "builtin",
    .size = 0,
    .methods = builtin_methods,
    .init = builtin_module_init,
    .fini = NULL,
};

#ifdef __cplusplus
}
#endif

/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.call
 */

#include "exception.h"
#include "stringobject.h"
#include "tupleobject.h"

#ifdef __cplusplus
extern "C" {
#endif

static TypeObject *mapping[] = {
    &void_type, &none_type, &int_type, &float_type, NULL, &exc_type,
};

TypeObject *object_type(Value *val)
{
    TypeObject *tp = mapping[val->tag];
    if (tp) return tp;
    if (IS_OBJECT(val)) return OB_TYPE(val->obj);
    return NULL;
}

Value object_call(Value *self, Value *args, int nargs, Object *names)
{
    TypeObject *tp = object_type(self);
    ASSERT(tp);
    CallFunc call = tp->call;
    if (!call) {
        /* raise an error */
        raise_exc_fmt("'%s' is not callable", tp->name);
        return error_value;
    }

    return call(self, args, nargs, names);
}

Object *kl_lookup_method(Value *obj, const char *fname)
{
    TypeObject *tp = object_type(obj);
    Object *fn = hashmap_get(tp->vtbl, NULL);
    return fn;
}

Value methodsite_call(MethodSite *ms, Value *args, int nargs, Object *names)
{
    TypeObject *tp = object_type(args);
    ASSERT(tp);
    if (tp != ms->type) {
        Object *meth = kl_lookup_method(args, ms->fname);
        ASSERT(meth);
        ms->type = tp;
        ms->method = meth;
    }
    Value v = object_value(ms->method);
    return object_call(&v, args, nargs, names);
}

static int get_name_index(Object *names, const char *name)
{
    Value *items = TUPLE_ITEMS(names);
    int len = TUPLE_LEN(names);

    for (int i = 0; i < len; i++) {
        Object *sobj = value_as_object(items + i);
        ASSERT(IS_STR(sobj));
        const char *s = STR_BUF(sobj);
        if (!strcmp(s, name)) {
            return i;
        }
    }

    return -1;
}

static void save_value(Value *arg, va_list va_args)
{
    Object **obj_p;
    switch (arg->tag) {
        case VAL_TAG_NONE: {
            Object **obj_p = va_arg(va_args, Object **);
            *obj_p = NULL;
            break;
        }
        case VAL_TAG_INT: {
            int64_t *r = va_arg(va_args, int64_t *);
            *r = arg->ival;
            break;
        }
        case VAL_TAG_FLOAT: {
            double *r = va_arg(va_args, double *);
            *r = arg->fval;
            break;
        }
        case VAL_TAG_OBJECT: {
            Object *obj = arg->obj;
            if (IS_STR(obj)) {
                const char **r = va_arg(va_args, const char **);
                *r = STR_BUF(obj);
            } else {
                Object **obj_p = va_arg(va_args, Object **);
                *obj_p = obj;
            }
            break;
        }
        default: {
            UNREACHABLE();
            break;
        }
    }
}

/**
 * Parse optional keyword arguments of this function.
 *
 * @return 0 successful, -1 error
 *
 * @param args The base pointer of arguments passed to this function.
 * @param nargs The number of positional arguments passed to this function.
 * @param names The tuple of keyword arguments' names passed to this function.
 * @param npos The number of positional arguments defined by this function.
 * @param kws The string array of acceptable keyword arguments' names defined by this
 * function.
 *
 * @note
 * The value of `nargs - npos` is the number of keyword arguments which are passed by
 * positional arguments.
 *
 * It will be checked by compiler if there are arguments both have positional values and
 * keyword values.
 */
int kl_parse_kwargs(Value *args, int nargs, Object *names, int npos, const char **kws,
                    ...)
{
    ASSERT(nargs >= npos);
    Value *v;
    va_list va_args;
    va_start(va_args, kws);

    // Parse keyword arguments which are passed by position.
    for (int i = npos; i < nargs; i++) {
        v = va_arg(va_args, Value *);
        *v = *(args + i);
        ++kws;
    }

    // Parse keyword arguments which are passed by keyword.
    if (names) {
        ASSERT(IS_TUPLE(names));
        const char *kw;
        while ((kw = *kws)) {
            int i = get_name_index(names, kw);
            v = va_arg(va_args, Value *);
            if (i >= 0) {
                *v = *(args + nargs + i);
            }
            ++kws;
        }
    }

    va_end(va_args);
}

Value object_str(Value *self)
{
    TypeObject *tp = object_type(self);
    ASSERT(tp->str);
    return tp->str(self);
}

static Value base_hash(Value *self)
{
    unsigned int v = mem_hash(self, sizeof(Value));
    return int_value(v);
}

static Value base_compare(Value *self, Value *rhs)
{
    int v = memcmp(self, rhs, sizeof(Value));
    int r = _compare_result(v);
    return int_value(r);
}

static Value base_str(Value *self)
{
    TypeObject *tp = object_type(self);
    Object *res = kl_new_fmt_str("<%s object>", tp->name);
    return object_value(res);
}

static MethodDef base_methods[] = {
    { "__hash__", base_hash, METH_NO_ARGS, "", "i" },
    { "__cmp__", base_compare, METH_ONE_ARG, "O", "b" },
    { "__str__", base_str, METH_NO_ARGS, "", "s" },
    { NULL },
};

TypeObject base_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "object",
    .flags = TP_FLAGS_CLASS | TP_FLAGS_PUBLIC,
    .hash = base_hash,
    .cmp = base_compare,
    .str = base_str,
    .methods = base_methods,
};

static Value none_str(Value *self)
{
    Object *s = kl_new_str("none");
    return object_value(s);
}

TypeObject none_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "none",
    .flags = TP_FLAGS_CLASS | TP_FLAGS_PUBLIC | TP_FLAGS_FINAL,
    .str = none_str,
};

static Value void_str(Value *self)
{
    Object *s = kl_new_str("void");
    return object_value(s);
}

TypeObject void_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "void",
    .flags = TP_FLAGS_CLASS | TP_FLAGS_PUBLIC | TP_FLAGS_FINAL,
    .str = void_str,
};

#ifdef __cplusplus
}
#endif

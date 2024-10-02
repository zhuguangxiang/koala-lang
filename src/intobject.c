/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "exception.h"
#include "stringobject.h"

#ifdef __cplusplus
extern "C" {
#endif

static Value int_hash(Value *self)
{
    unsigned int v = mem_hash(&self->ival, sizeof(int64_t));
    return int_value(v);
}

static Value int_compare(Value *self, Value *rhs)
{
    if (!IS_INT(rhs)) {
        raise_exc_str("Unsupported");
        return error_value;
    }

    int64_t v1 = self->ival;
    int64_t v2 = rhs->ival;
    int64_t v = v1 - v2;
    int r = _compare_result(v);
    return int_value(r);
}

static Value int_str(Value *self)
{
    char buf[24];
    snprintf(buf, 23, "%ld", self->ival);
    buf[23] = '\0';
    Object *sobj = kl_new_str(buf);
    return object_value(sobj);
}

static MethodDef int_methods[] = {
    { "__hash__", int_hash, METH_NO_ARGS, "", "i" },
    { "__cmp__", int_compare, METH_ONE_ARG, "i", "b" },
    { "__str__", int_str, METH_NO_ARGS, "", "s" },
    { NULL },
};

/*
int()
int(12.3)
int(100)
int("100")
int("0xface", 16)
func int(x object = 0, base = 10) int;
*/
static int int_init(Value *self, Value *args, int nargs, Object *names)
{
    Value _x = void_value;
    Value _base = void_value;
    const char *_kws[] = { "x", "base", NULL };
    kl_parse_kwargs(args, nargs, names, 0, _kws, &_x, &_base);

    const char *s = NULL;
    int base = 10;

    if (!IS_VOID(&_base)) {
        // If `base` has value, check `x` MUST be string.
        ASSERT(IS_INT(&_base));
        if (!IS_STR_OBJ(&_x)) {
            TypeObject *tp = object_type(&_x);
            ASSERT(tp);
            raise_exc_fmt("expect 'str', but got '%s'", tp->name);
            return -1;
        }

        s = STR_BUF(value_object(&_x));
        base = value_int(&_base);
        errno = 0;
        long long v = strtoll(s, NULL, base);
        if (errno) {
            raise_exc_fmt("%s", strerror(errno));
            return -1;
        }

        *self = int_value(v);
        return 0;
    }

    // `base` is not set, and `x` MUST be int or float
    if (IS_INT(&_x)) {
        *self = int_value(value_int(&_x));
    } else if (IS_FLOAT(&_x)) {
        *self = int_value(value_float(&_x));
    } else {
        TypeObject *tp = object_type(&_x);
        raise_exc_fmt("expect 'int' or 'float', but got '%s'", tp->name);
        return -1;
    }

    return 0;
}

TypeObject int_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "int",
    .flags = TP_FLAGS_CLASS | TP_FLAGS_PUBLIC | TP_FLAGS_FINAL,
    .desc = "|Oi",
    .hash = int_hash,
    .cmp = int_compare,
    .str = int_str,
    .methods = int_methods,
    .init = int_init,
};

#ifdef __cplusplus
}
#endif

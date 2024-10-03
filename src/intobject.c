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
    unsigned int v = mem_hash(&to_int(self), sizeof(int64_t));
    return int_value(v);
}

static Value int_compare(Value *self, Value *rhs)
{
    if (!IS_INT(rhs)) {
        raise_exc_str("Unsupported");
        return error_value;
    }

    int64_t v1 = to_int(self);
    int64_t v2 = to_int(rhs);
    int64_t v = v1 - v2;
    int r = _compare_result(v);
    return int_value(r);
}

static Value int_str(Value *self)
{
    char buf[24];
    snprintf(buf, 23, "%ld", to_int(self));
    buf[23] = '\0';
    Object *sobj = kl_new_str(buf);
    return obj_value(sobj);
}

static MethodDef int_methods[] = {
    { "__hash__", int_hash, METH_NO_ARGS, "", "i" },
    { "__cmp__", int_compare, METH_ONE_ARG, "i", "b" },
    { "__str__", int_str, METH_NO_ARGS, "", "s" },
    { NULL },
};

static int str_to_int(const char *s, int base, Value *ret)
{
    errno = 0;
    long long v = strtoll(s, NULL, base);
    if (errno) {
        raise_exc_fmt("%s", strerror(errno));
        return -1;
    }

    *ret = int_value(v);
    return 0;
}

static int int_init_impl(Value *self, Value *_x, Value *_base)
{
    if (IS_UNDEF(_base)) {
        // `base` is not set, and `x` MUST be int, float or string
        if (IS_INT(_x)) {
            *self = *_x;
            return 0;
        } else if (IS_FLOAT(_x)) {
            *self = int_value(to_float(_x));
            return 0;
        } else if (IS_OBJ(_x)) {
            Object *obj = to_obj(_x);
            if (!IS_STR(obj)) {
                TypeObject *tp = OB_TYPE(obj);
                ASSERT(tp);
                raise_exc_fmt("expect 'str', but got '%s'", tp->name);
                return -1;
            }
            const char *s = STR_BUF(obj);
            return str_to_int(s, 10, self);
        } else {
            TypeObject *tp = object_type(_x);
            raise_exc_fmt("expect 'int', 'float' or 'str', but got '%s'", tp->name);
            return -1;
        }
    }

    ASSERT(IS_INT(_base));
    int base = to_int(_base);

    // If `base` has value, check `x` MUST be string.
    if (!IS_OBJ(_x) || !IS_STR(to_obj(_x))) {
        TypeObject *tp = object_type(_x);
        ASSERT(tp);
        raise_exc_fmt("expect 'str', but got '%s'", tp->name);
        return -1;
    }

    const char *s = STR_BUF(to_obj(_x));
    return str_to_int(s, base, self);
}

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
    Value _x = undef_value;
    Value _base = undef_value;
    const char *_kws[] = { "x", "base", NULL };
    kl_parse_kwargs(args, nargs, names, 0, _kws, &_x, &_base);

    return int_init_impl(self, &_x, &_base);
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

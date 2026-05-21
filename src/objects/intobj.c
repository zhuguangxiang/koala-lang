/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

// static Value int_hash(Value *self)
// {
//     uint64_t v = mem_hash(&to_int(self), sizeof(int64_t));
//     return int_value(v);
// }

// static Value int_compare(Value *self, Value *rhs)
// {
//     if (!IS_INT(rhs)) {
//         raise_exc_str("Unsupported");
//         return error_value;
//     }

//     int64_t v1 = to_int(self);
//     int64_t v2 = to_int(rhs);
//     int64_t v = v1 - v2;
//     int r = _compare_result(v);
//     return int_value(r);
// }

// static Value int_str(Value *self)
// {
//     char buf[24];
//     snprintf(buf, 23, "%ld", to_int64(self));
//     buf[23] = '\0';
//     Object *sobj = kl_new_str(buf);
//     return obj_value(sobj);
// }

// // static MethodDef int_methods[] = {
// //     { "__hash__", int_hash, METH_NO_ARGS, "", "i" },
// //     { "__cmp__", int_compare, METH_ONE_ARG, "i", "b" },
// //     { "__str__", int_str, METH_NO_ARGS, "", "s" },
// //     { NULL },
// // };

// static int str_to_int(const char *s, int base, Value *ret)
// {
//     errno = 0;
//     long long v = strtoll(s, NULL, base);
//     if (errno) {
//         raise_exc_fmt("%s", strerror(errno));
//         return -1;
//     }

//     *ret = int64_value(v);
//     return 0;
// }

// static int int_init_impl(Value *self, Value *_x, Value *_base)
// {
//     if (is_none(_base)) {
//         // `base` is not set, and `x` MUST be int, float or string
//         if (is_int(_x)) {
//             *self = *_x;
//             return 0;
//         } else if (is_float(_x)) {
//             *self = int64_value(to_float64(_x));
//             return 0;
//         } else if (is_obj(_x)) {
//             Object *obj = to_obj(_x);
//             // if (!IS_STR(obj)) {
//             //     TypeObject *tp = OB_TYPE(obj);
//             //     ASSERT(tp);
//             //     raise_exc_fmt("expect 'str', but got '%s'", tp->name);
//             //     return -1;
//             // }
//             const char *s = STR_BUF(obj);
//             return str_to_int(s, 10, self);
//         } else {
//             TypeObject *tp = object_typeof(_x);
//             raise_exc_fmt("expect 'int', 'float' or 'str', but got '%s'", tp->name);
//             return -1;
//         }
//     }

//     ASSERT(is_int(_base));
//     int base = to_int64(_base);

//     // If `base` has value, check `x` MUST be string.
//     // if (!is_obj(_x) || !IS_STR(to_obj(_x))) {
//     //     TypeObject *tp = value_typeof(_x);
//     //     ASSERT(tp);
//     //     raise_exc_fmt("expect 'str', but got '%s'", tp->name);
//     //     return -1;
//     // }

//     const char *s = STR_BUF(to_obj(_x));
//     return str_to_int(s, base, self);
// }

// /*
// int()
// int(12.3)
// int(100)
// int("100")
// int("0xface", 16)
// func int(x object = 0, base = 10) int;
// */
// static int int_init(Value *self, Value *args, int nargs, Object *names)
// {
//     Value _x = none_value;
//     Value _base = none_value;
//     // const char *_kws[] = { "x", "base", NULL };
//     // kl_parse_kwargs(args, nargs, names, 0, _kws, &_x, &_base);

//     return int_init_impl(self, &_x, &_base);
// }

static TValue int_str(TValue *self, TValue *args, int nargs)
{
    char buf[24];
    snprintf(buf, 23, "%" PRId64, self->ival);
    buf[23] = '\0';
    Object *sobj = kl_new_nstr(buf, strlen(buf));
    return obj_value(sobj);
}

static MethodDef int_methods[] = {
    { "__str__", int_str },
    { NULL },
};

static TypeObject *int_bases[] = {
    &Number_type,
    NULL,
};

TypeObject int_type = {
    ._type = &type_type,
    .name = "int",
    .flags = TP_FLAGS_CLASS | TP_FLAGS_PUBLIC,
    .bases = int_bases,
    .methdefs = int_methods,
};

#ifdef __cplusplus
}
#endif

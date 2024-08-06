/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "intobject.h"
#include "exception.h"
#include "strobject.h"

#ifdef __cplusplus
extern "C" {
#endif

static Value int_hash(Value *self)
{
    unsigned int v = mem_hash(&self->ival, sizeof(int64_t));
    return IntValue(v);
}

static Value int_compare(Value *lhs, Value *rhs)
{
    if (!IS_INT(rhs)) {
        raise_exc("Unsupported");
        return ErrorValue;
    }

    int64_t v1 = lhs->ival;
    int64_t v2 = rhs->ival;
    int64_t v = v1 - v2;
    int r = _compare_result(v);
    return IntValue(r);
}

static Value int_str(Value *self)
{
    char buf[24];
    snprintf(buf, 23, "%ld", self->ival);
    buf[23] = '\0';
    Object *sobj = kl_new_str(buf);
    return ObjValue(sobj);
}

/*
func __init__(v ToInt = 0, base = 10) {
    if v is Int {
        self = v
    } else if v is Float {
        self = v.to_int()
    } else if v is String {
        self = v.to_int(base)
    } else {
        self = v.to_int(base)
    }
}

int()
int(12.3)
int(100)
int("100")
int("0xface", 16)
*/
static int int_init(Value *self, Value *args, int nargs)
{
    // ASSERT(nargs == 2);
    self->tag = VAL_TAG_INT;
    if (IS_INT(args)) {
        self->ival = args->ival;
    } else if (IS_FLOAT(args)) {
        self->ival = (int64_t)args->fval;
    } else if (IS_OBJ(args)) {
        if (IS_STR(args->obj)) {
            // char *s = STR_BUF(args->obj);
            // errno = 0;
            // int base = 10;
            // long long v = strtoll(s, NULL, base);
            // if (errno) {
            // }
            // self->ival = v;
        } else {
            // Value r = object_call_method_noarg(args->obj, "to_int");
            // if (IS_ERROR(&r)) return -1;
            // self->ival = r.ival;
        }
    }
    return 0;
}

TypeObject int_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "Int",
    .flags = TP_FLAGS_CLASS | TP_FLAGS_PUBLIC | TP_FLAGS_FINAL,
    .size = 0,
    .hash = int_hash,
    .cmp = int_compare,
    .str = int_str,
    .init = int_init,
};

#ifdef __cplusplus
}
#endif

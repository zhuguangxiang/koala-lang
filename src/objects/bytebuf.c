/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "bytebuf.h"

#ifdef __cplusplus
extern "C" {
#endif

static TValue bytebuf_append(TValue *self, TValue *args, int nargs)
{
    ByteBufObject *bb = SELF_AS(bytebuf_type);
    ASSERT(nargs == 1);
    uint8_t value = kl_arg_uint8(args, nargs, 0);
    buf_write_byte(&bb->buf, value);
    return none_value;
}

static TValue bytebuf_len(TValue *self, TValue *args, int nargs)
{
    ByteBufObject *bb = SELF_AS(bytebuf_type);
    ASSERT(nargs == 0);
    return int64_value(BUF_LEN(bb->buf));
}

static TValue bytebuf_str(TValue *self, TValue *args, int nargs)
{
    ByteBufObject *bb = SELF_AS(bytebuf_type);
    ASSERT(nargs == 0);
    Object *so = kl_new_nstr(BUF_STR(bb->buf), BUF_LEN(bb->buf));
    return obj_value(so);
}

static TValue bytebuf_init(TValue *self, TValue *args, int nargs)
{
    ByteBufObject *bb = SELF_AS(bytebuf_type);
    ASSERT(nargs == 1);
    int64_t size = kl_arg_int64(0);
    if (size < 0) {
        panic("bytebuf size must be >= 0");
    }
    ASSERT(BUF_LEN(bb->buf) == 0);
    buf_reserve(&bb->buf, (size_t)size);
    return none_value;
}

static TValue bytebuf_to_bytes(TValue *self, TValue *args, int nargs)
{
    ByteBufObject *bb = SELF_AS(bytebuf_type);
    ASSERT(nargs == 0);
    Object *so = kl_new_nstr(BUF_STR(bb->buf), BUF_LEN(bb->buf));
    return obj_value(so);
}

static TValue bytebuf_clear(TValue *self, TValue *args, int nargs) { return none_value; }

static MethodDef bytebuf_methods[] = {
    { "append", bytebuf_append },
    { "__len__", bytebuf_len },
    { "__str__", bytebuf_str },
    { "__init__", bytebuf_init },
    { "to_bytes", bytebuf_to_bytes },
    { "clear", bytebuf_clear },
    { NULL },
};

static size_t bytebuf_seq_len(TValue *self)
{
    ByteBufObject *bb = SELF_AS(bytebuf_type);
    return BUF_LEN(bb->buf);
}

static TValue bytebuf_seq_get(TValue *self, size_t index)
{
    ByteBufObject *bb = SELF_AS(bytebuf_type);
    if (index >= BUF_LEN(bb->buf)) {
        panic("bytebuf index out of range");
        return none_value;
    }
    uint8_t value = (uint8_t)BUF_STR(bb->buf)[index];
    return uint8_value(value);
}

static void bytebuf_seq_set(TValue *self, size_t index, TValue *value)
{
    ByteBufObject *bb = SELF_AS(bytebuf_type);
    if (index >= BUF_LEN(bb->buf)) {
        panic("bytebuf index out of range");
    }
    uint8_t v = to_uint8(value);
    BUF_STR(bb->buf)[index] = v;
}

static SeqMethods bytebuf_seq_methods = {
    .len = bytebuf_seq_len,
    .get = bytebuf_seq_get,
    .set = bytebuf_seq_set,
};

static Object *bytebuf_alloc(TypeObject *tp)
{
    ByteBufObject *bb = mm_alloc_obj(bb);
    INIT_OBJECT_HEAD(bb, tp);
    return (Object *)bb;
}

TypeObject bytebuf_type = {
    ._type = &type_type,
    .name = "ByteBuf",
    .flags = TP_FLAGS_CLASS | TP_FLAGS_PUBLIC,
    .methdefs = bytebuf_methods,
    .seq = &bytebuf_seq_methods,
    .alloc = bytebuf_alloc,
};

Object *kl_new_bytebuf(size_t size)
{
    ByteBufObject *bb = mm_alloc_obj(bb);
    INIT_OBJECT_HEAD(bb, &bytebuf_type);
    buf_reserve(&bb->buf, size);
    return (Object *)bb;
}

#ifdef __cplusplus
}
#endif

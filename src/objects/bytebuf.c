/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "bytebuf.h"

#ifdef __cplusplus
extern "C" {
#endif

static TValue _bytebuf_append(TValue *self, TValue *args, int nargs)
{
    ByteBufObject *bb = SELF_AS(ByteBuf_type);
    ASSERT(nargs == 1);
    uint8_t value = kl_arg_uint8(0);
    buf_write_byte(&bb->buf, value);
    return none_value;
}

static TValue _bytebuf_len(TValue *self, TValue *args, int nargs)
{
    ByteBufObject *bb = SELF_AS(ByteBuf_type);
    ASSERT(nargs == 0);
    return int64_value(BUF_LEN(bb->buf));
}

static TValue _bytebuf_str(TValue *self, TValue *args, int nargs)
{
    ByteBufObject *bb = SELF_AS(ByteBuf_type);
    ASSERT(nargs == 0);
    Object *so = kl_new_nstr(BUF_STR(bb->buf), BUF_LEN(bb->buf));
    return obj_value(so);
}

static TValue _bytebuf_init(TValue *self, TValue *args, int nargs)
{
    ByteBufObject *bb = SELF_AS(ByteBuf_type);
    ASSERT(nargs == 1);
    int64_t size = kl_arg_int64(0);
    if (size < 0) {
        panic("bytebuf size must be >= 0");
    }
    ASSERT(BUF_LEN(bb->buf) == 0);
    buf_reserve(&bb->buf, (size_t)size);
    return none_value;
}

static TValue _bytebuf_to_bytes(TValue *self, TValue *args, int nargs)
{
    ByteBufObject *bb = SELF_AS(ByteBuf_type);
    ASSERT(nargs == 0);
    Object *so = kl_new_nstr(BUF_STR(bb->buf), BUF_LEN(bb->buf));
    return obj_value(so);
}

static TValue _bytebuf_getitem(TValue *self, TValue *args, int nargs)
{
    ByteBufObject *bb = SELF_AS(ByteBuf_type);
    ASSERT(nargs == 1);
    int64_t index = to_int64(args);
    if (index < 0 || index >= BUF_LEN(bb->buf)) {
        panic("bytebuf index out of range");
        return none_value;
    }
    uint8_t value = (uint8_t)BUF_STR(bb->buf)[index];
    return uint8_value(value);
}

static TValue _bytebuf_setitem(TValue *self, TValue *args, int nargs)
{
    ByteBufObject *bb = SELF_AS(ByteBuf_type);
    ASSERT(nargs == 2);
    int64_t index = to_int64(args);
    if (index < 0 || index >= BUF_LEN(bb->buf)) {
        panic("bytebuf index out of range");
        return none_value;
    }
    uint8_t value = to_uint8(&args[1]);
    BUF_STR(bb->buf)[index] = value;
    return none_value;
}

static TValue _bytebuf_clear(TValue *self, TValue *args, int nargs) { return none_value; }

static MethodDef _bytebuf_methods[] = {
    { "append", _bytebuf_append },
    { "__len__", _bytebuf_len },
    { "__setitem__", _bytebuf_setitem },
    { "__getitem__", _bytebuf_getitem },
    { "__str__", _bytebuf_str },
    { "__init__", _bytebuf_init },
    { "to_bytes", _bytebuf_to_bytes },
    { "clear", _bytebuf_clear },
    { NULL },
};

/* pub class ByteBuf : MutableSequence[uint8] { ... } */
DEFINE_TYPE(ByteBuf, TP_FLAGS_CLASS, sizeof(Buffer), _bytebuf_methods, NULL);

Object *kl_new_bytebuf(size_t size)
{
    ByteBufObject *bb = mm_alloc_obj(bb);
    INIT_OBJECT_HEAD(bb, &ByteBuf_type, 0);
    buf_reserve(&bb->buf, size);
    return (Object *)bb;
}

#ifdef __cplusplus
}
#endif

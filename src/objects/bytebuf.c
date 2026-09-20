/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "bytebuf.h"
#include "bytesobj.h"
#include "excobj.h"

#ifdef __cplusplus
extern "C" {
#endif

static TValue _bytebuf_write_byte(TValue *self, TValue *args, int nargs)
{
    ByteBufObject *bb = SELF_AS(bytebuf_type);
    ASSERT(nargs == 1);
    uint8_t value = kl_arg_uint8(0);
    buf_write_byte(&bb->buf, value);
    return nil_value;
}

static TValue _bytebuf_write_str(TValue *self, TValue *args, int nargs)
{
    ByteBufObject *bb = SELF_AS(bytebuf_type);
    ASSERT(nargs == 1);
    Object *sobj = kl_arg_strobj(0);
    const char *data = STR_BUF(sobj);
    size_t len = STR_LEN(sobj);
    buf_write_nstr(&bb->buf, data, len);
    return nil_value;
}

static TValue _bytebuf_write_bytes(TValue *self, TValue *args, int nargs)
{
    ByteBufObject *bb = SELF_AS(bytebuf_type);
    ASSERT(nargs == 1);
    Object *obj = to_obj(&args[0]);
    ASSERT(IS_BYTES(obj));
    BytesObject *bs = (BytesObject *)obj;
    buf_write_nstr(&bb->buf, (char *)bs->data + bs->offset, bs->size);
    return nil_value;
}

static TValue _bytebuf_len(TValue *self, TValue *args, int nargs)
{
    ByteBufObject *bb = SELF_AS(bytebuf_type);
    ASSERT(nargs == 0);
    return int64_value(BUF_LEN(bb->buf));
}

static TValue _bytebuf_to_str(TValue *self, TValue *args, int nargs)
{
    ByteBufObject *bb = SELF_AS(bytebuf_type);
    ASSERT(nargs == 0);
    Object *so = kl_new_nstr(BUF_STR(bb->buf), BUF_LEN(bb->buf));
    return obj_value(so);
}

static TValue _bytebuf_init(TValue *self, TValue *args, int nargs)
{
    ByteBufObject *bb = SELF_AS(bytebuf_type);
    ASSERT(nargs == 1);
    int64_t size = kl_arg_int64(0);
    if (size < 0) {
        raise_exc_str("bytebuf size must be >= 0");
        return error_value;
    }
    ASSERT(BUF_LEN(bb->buf) == 0);
    buf_reserve(&bb->buf, (size_t)size);
    return nil_value;
}

static TValue _bytebuf_to_bytes(TValue *self, TValue *args, int nargs)
{
    ByteBufObject *bb = SELF_AS(bytebuf_type);
    ASSERT(nargs == 0);
    Object *so = kl_new_nstr(BUF_STR(bb->buf), BUF_LEN(bb->buf));
    return obj_value(so);
}

static TValue _bytebuf_getitem(TValue *self, TValue *args, int nargs)
{
    ByteBufObject *bb = SELF_AS(bytebuf_type);
    ASSERT(nargs == 1);
    int64_t index = to_int64(&args[0]);
    if (index < 0 || index >= BUF_LEN(bb->buf)) {
        raise_exc_str("Buffer index out of range");
        return error_value;
    }
    uint8_t value = (uint8_t)BUF_STR(bb->buf)[index];
    return uint8_value(value);
}

static TValue _bytebuf_setitem(TValue *self, TValue *args, int nargs)
{
    ByteBufObject *bb = SELF_AS(bytebuf_type);
    ASSERT(nargs == 2);
    int64_t index = to_int64(&args[0]);
    if (index < 0 || index >= BUF_LEN(bb->buf)) {
        raise_exc_str("Buffer index out of range");
        return error_value;
    }
    uint8_t value = to_uint8(&args[1]);
    BUF_STR(bb->buf)[index] = value;
    return nil_value;
}

static TValue _bytebuf_clear(TValue *self, TValue *args, int nargs)
{
    ByteBufObject *bb = SELF_AS(bytebuf_type);
    ASSERT(nargs == 0);
    RESET_BUF(bb->buf);
    return nil_value;
}

static MethodDef _bytebuf_methods[] = {
    { "__init__", _bytebuf_init },
    { "write_byte", _bytebuf_write_byte },
    { "write_str", _bytebuf_write_str },
    { "write", _bytebuf_write_bytes },
    { "__len__", _bytebuf_len },
    { "__setitem__", _bytebuf_setitem },
    { "__getitem__", _bytebuf_getitem },
    { "to_str", _bytebuf_to_str },
    { "to_bytes", _bytebuf_to_bytes },
    { "clear", _bytebuf_clear },
    { NULL },
};

/* pub class Buffer { ... } */

TypeObject bytebuf_type = {
    ._type = &type_type,
    .name = "Buffer",
    .flags = ((1 << 1)),
    .priv_size = sizeof(ByteBufObject),
    .methdefs = _bytebuf_methods,
};

Object *kl_new_bytebuf(size_t size)
{
    ByteBufObject *bb = mm_alloc_obj(bb);
    INIT_OBJECT_HEAD(bb, &bytebuf_type, 0);
    buf_reserve(&bb->buf, size);
    return (Object *)bb;
}

#ifdef __cplusplus
}
#endif

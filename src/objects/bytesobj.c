/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "bytesobj.h"
#include "buffer.h"
#include "excobj.h"

#ifdef __cplusplus
extern "C" {
#endif

static TValue _bytes_len(TValue *self, TValue *args, int nargs)
{
    BytesObject *bytes = (BytesObject *)to_obj(self);
    return int64_value(bytes->size);
}

static TValue _bytes_str(TValue *self, TValue *args, int nargs)
{
    BytesObject *bytes = (BytesObject *)to_obj(self);
    int64_t size = bytes->size;

    BUF(buf);
    for (int i = 0; i < size; ++i) {
        buf_write_uint8_hex(&buf, bytes->data[bytes->offset + i]);
    }
    Object *sobj = kl_new_nstr(BUF_STR(buf), BUF_LEN(buf));
    FINI_BUF(buf);

    return obj_value(sobj);
}

static TValue _bytes_init(TValue *self, TValue *args, int nargs)
{
    Object *obj = to_obj(self);
    ASSERT(obj && IS_BYTES(obj));

    BytesObject *bytes = (BytesObject *)obj;

    ASSERT(nargs == 1);
    int64_t size = kl_arg_int64(0);
    if (size < 0) {
        raise_exc_str("bytes size must be non-negative");
        return error_value;
    }

    void *data = mm_alloc(size);
    bytes->data = (uint8_t *)data;
    bytes->size = size;
    return nil_value;
}

static TValue _bytes_index(TValue *self, TValue *args, int nargs)
{
    Object *obj = to_obj(self);
    ASSERT(obj && IS_BYTES(obj));

    BytesObject *bytes = (BytesObject *)obj;

    ASSERT(nargs == 3);
    uint8_t value = kl_arg_uint8(0);

    void *ptr = memchr(bytes->data + bytes->offset, value, bytes->size);
    if (ptr) {
        return int64_value((uint8_t *)ptr - (bytes->data + bytes->offset));
    } else {
        return int64_value(-1);
    }
}

static TValue _bytes_count(TValue *self, TValue *args, int nargs)
{
    Object *obj = to_obj(self);
    ASSERT(obj && IS_BYTES(obj));

    BytesObject *bytes = (BytesObject *)obj;

    ASSERT(nargs == 3);
    uint8_t value = kl_arg_uint8(0);

    uint8_t *ptr = bytes->data + bytes->offset;
    uint8_t *end = ptr + bytes->size;
    int count = 0;
    while (ptr < end) {
        if (*ptr == value) {
            count++;
        }
        ptr++;
    }
    return int64_value(count);
}

static TValue _bytes_copy(TValue *self, TValue *args, int nargs)
{
    Object *obj = to_obj(self);
    ASSERT(obj && IS_BYTES(obj));

    BytesObject *bytes = (BytesObject *)obj;

    ASSERT(nargs == 3);
    Object *src = to_obj(&args[0]);
    ASSERT(src && IS_BYTES(src));
    BytesObject *src_bytes = (BytesObject *)src;

    int src_start = kl_arg_int64(1);
    int src_end = kl_arg_int64(2);
    if (src_end < 0) src_end = src_bytes->size;

    int len = src_end - src_start;
    ASSERT(len <= bytes->size);

    memmove(bytes->data + bytes->offset, src_bytes->data + src_bytes->offset + src_start, len);
    return int64_value(len);
}

static TValue _bytes_fill(TValue *self, TValue *args, int nargs)
{
    Object *obj = to_obj(self);
    ASSERT(obj && IS_BYTES(obj));

    BytesObject *bytes = (BytesObject *)obj;

    ASSERT(nargs == 3);
    uint8_t value = kl_arg_uint8(0);
    memset(bytes->data + bytes->offset, value, bytes->size);
    return nil_value;
}

static TValue _bytes_zero(TValue *self, TValue *args, int nargs)
{
    Object *obj = to_obj(self);
    ASSERT(obj && IS_BYTES(obj));

    BytesObject *bytes = (BytesObject *)obj;

    ASSERT(nargs == 2);
    memset(bytes->data + bytes->offset, 0, bytes->size);
    return nil_value;
}

static TValue _bytes_view(TValue *self, TValue *args, int nargs)
{
    Object *obj = to_obj(self);
    ASSERT(obj && IS_BYTES(obj));

    BytesObject *bytes = (BytesObject *)obj;

    ASSERT(nargs == 2);
    int64_t start = kl_arg_int64(0);
    int64_t end = kl_arg_int64(1);
    if (end < 0) end = bytes->size;
    ASSERT(start >= 0 && end >= 0 && start <= end && end <= bytes->size);

    BytesObject *view_bytes = mm_alloc_obj(view_bytes);
    INIT_OBJECT_HEAD(view_bytes, &bytes_type, 0);
    view_bytes->offset += start;
    view_bytes->size = end - start;
    view_bytes->data = bytes->data;
    return obj_value((Object *)view_bytes);
}

static TValue _bytes_tostr(TValue *self, TValue *args, int nargs)
{
    Object *obj = to_obj(self);
    ASSERT(obj && IS_BYTES(obj));

    BytesObject *bytes = (BytesObject *)obj;
    ASSERT(nargs == 0);

    Object *s = kl_new_nstr((char *)bytes->data + bytes->offset, bytes->size);
    return obj_value(s);
}

static TValue _bytes_getitem(TValue *self, TValue *args, int nargs)
{
    BytesObject *bytes = SELF_AS(bytes_type);

    ASSERT(nargs == 1);
    int64_t index = kl_arg_int64(0);
    if (!(index >= 0 && index < bytes->size)) {
        raise_exc_str("bytes index out of range");
        return error_value;
    }
    return uint8_value(bytes->data[bytes->offset + index]);
}

static TValue _bytes_setitem(TValue *self, TValue *args, int nargs)
{
    BytesObject *bytes = SELF_AS(bytes_type);

    ASSERT(nargs == 2);
    int64_t index = kl_arg_int64(0);
    if (!(index >= 0 && index < bytes->size)) {
        raise_exc_str("bytes index out of range");
        return error_value;
    }

    uint8_t value = kl_arg_uint8(1);
    bytes->data[bytes->offset + index] = value;
    return nil_value;
}

static MethodDef bytes_methods[] = {
    { "__len__", _bytes_len },
    { "__str__", _bytes_str },
    { "__init__", _bytes_init },
    { "__getitem__", _bytes_getitem },
    { "__setitem__", _bytes_setitem },
    { "index", _bytes_index },
    { "count", _bytes_count },
    { "copy", _bytes_copy },
    { "fill", _bytes_fill },
    { "zero", _bytes_zero },
    { "view", _bytes_view },
    { "to_str", _bytes_tostr },
    { NULL, NULL },
};

/* pub class bytes : Sequence[uint8] { ... } */
DEFINE_TYPE(bytes, TP_FLAGS_CLASS, 2 * sizeof(uint32_t) + sizeof(uint8_t *), bytes_methods, NULL);

Object *kl_new_bytes(uint32_t size)
{
    BytesObject *bytes = mm_alloc_obj(bytes);
    INIT_OBJECT_HEAD(bytes, &bytes_type, 0);
    bytes->offset = 0;
    bytes->size = size;
    bytes->data = mm_alloc(size);
    return (Object *)bytes;
}

#ifdef __cplusplus
}
#endif

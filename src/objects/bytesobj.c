/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "bytesobj.h"
#include "buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

static TValue kl_bytes_len(TValue *self, TValue *args, int nargs)
{
    BytesObject *bytes = (BytesObject *)to_obj(self);
    return int64_value(bytes->size);
}

static TValue kl_bytes_str(TValue *self, TValue *args, int nargs)
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

static TValue kl_bytes_init(TValue *self, TValue *args, int nargs)
{
    Object *obj = to_obj(self);
    ASSERT(obj && IS_BYTES(obj));

    BytesObject *bytes = (BytesObject *)obj;

    ASSERT(nargs == 1);
    TValue *v = &args[0];
    int64_t size = to_int64(v);
    if (size < 0) {
        panic("bytes size must be non-negative");
        return none_value;
    }

    void *data = mm_alloc(size);
    bytes->data = (uint8_t *)data;
    bytes->size = size;
    return none_value;
}

static TValue kl_bytes_index(TValue *self, TValue *args, int nargs)
{
    Object *obj = to_obj(self);
    ASSERT(obj && IS_BYTES(obj));

    BytesObject *bytes = (BytesObject *)obj;

    ASSERT(nargs == 1);
    TValue *v = &args[0];
    ASSERT(is_uint8(v));
    uint8_t value = (uint8_t)v->ival;

    void *ptr = memchr(bytes->data + bytes->offset, value, bytes->size);
    if (ptr) {
        return int64_value((uint8_t *)ptr - (bytes->data + bytes->offset));
    } else {
        return int64_value(-1);
    }
}

static TValue kl_bytes_count(TValue *self, TValue *args, int nargs)
{
    Object *obj = to_obj(self);
    ASSERT(obj && IS_BYTES(obj));

    BytesObject *bytes = (BytesObject *)obj;

    ASSERT(nargs == 1);
    TValue *v = &args[0];
    ASSERT(is_uint8(v));
    uint8_t value = (uint8_t)v->ival;

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

static TValue kl_bytes_copy(TValue *self, TValue *args, int nargs)
{
    Object *obj = to_obj(self);
    ASSERT(obj && IS_BYTES(obj));

    BytesObject *bytes = (BytesObject *)obj;

    ASSERT(nargs == 3);
    Object *src = to_obj(&args[0]);
    ASSERT(src && IS_BYTES(src));
    BytesObject *src_bytes = (BytesObject *)src;

    int src_start = to_int64(&args[1]);
    int src_end = to_int64(&args[2]);
    if (src_end < 0) src_end = src_bytes->size;

    int len = src_end - src_start;
    ASSERT(len <= bytes->size);

    memmove(bytes->data + bytes->offset, src_bytes->data + src_bytes->offset + src_start, len);
    return int64_value(len);
}

static TValue kl_bytes_fill(TValue *self, TValue *args, int nargs)
{
    Object *obj = to_obj(self);
    ASSERT(obj && IS_BYTES(obj));

    BytesObject *bytes = (BytesObject *)obj;

    ASSERT(nargs == 3);
    TValue *v = &args[0];
    ASSERT(is_uint8(v));
    uint8_t value = (uint8_t)v->ival;
    memset(bytes->data + bytes->offset, value, bytes->size);
    return none_value;
}

static TValue kl_bytes_zero(TValue *self, TValue *args, int nargs)
{
    Object *obj = to_obj(self);
    ASSERT(obj && IS_BYTES(obj));

    BytesObject *bytes = (BytesObject *)obj;

    ASSERT(nargs == 2);
    memset(bytes->data + bytes->offset, 0, bytes->size);
    return none_value;
}

static TValue kl_bytes_view(TValue *self, TValue *args, int nargs)
{
    Object *obj = to_obj(self);
    ASSERT(obj && IS_BYTES(obj));

    BytesObject *bytes = (BytesObject *)obj;

    ASSERT(nargs == 2);
    ASSERT(is_int64(&args[0]) && is_int64(&args[1]));
    int64_t start = to_int64(&args[0]);
    int64_t end = to_int64(&args[1]);
    if (end < 0) end = bytes->size;
    ASSERT(start >= 0 && end >= 0 && start <= end && end <= bytes->size);

    BytesObject *view_bytes = mm_alloc_obj(view_bytes);
    INIT_OBJECT_HEAD(view_bytes, &bytes_type, 0);
    view_bytes->offset += start;
    view_bytes->size = end - start;
    view_bytes->data = bytes->data;
    return obj_value((Object *)view_bytes);
}

static TValue kl_bytes_tostr(TValue *self, TValue *args, int nargs)
{
    Object *obj = to_obj(self);
    ASSERT(obj && IS_BYTES(obj));

    BytesObject *bytes = (BytesObject *)obj;
    ASSERT(nargs == 0);

    Object *s = kl_new_nstr((char *)bytes->data + bytes->offset, bytes->size);
    return obj_value(s);
}

static MethodDef bytes_methods[] = {
    { "__len__", kl_bytes_len },
    { "__str__", kl_bytes_str },
    { "__init__", kl_bytes_init },
    { "index", kl_bytes_index },
    { "count", kl_bytes_count },
    { "copy", kl_bytes_copy },
    { "fill", kl_bytes_fill },
    { "zero", kl_bytes_zero },
    { "view", kl_bytes_view },
    { "to_str", kl_bytes_tostr },
    { NULL, NULL },
};

static size_t kl_bytes_seq_len(TValue *self)
{
    BytesObject *bytes = (BytesObject *)to_obj(self);
    return bytes->size;
}

static TValue kl_bytes_seq_get(TValue *self, size_t index)
{
    BytesObject *bytes = (BytesObject *)to_obj(self);
    if (index >= bytes->size) {
        panic("bytes index out of range");
        return none_value;
    }
    uint8_t value = bytes->data[bytes->offset + index];
    return uint8_value(value);
}

static void kl_bytes_seq_set(TValue *self, size_t index, TValue *value)
{
    BytesObject *bytes = (BytesObject *)to_obj(self);
    if (index >= bytes->size) {
        panic("bytes index out of range");
    }
    ASSERT(is_uint8(value));
    bytes->data[bytes->offset + index] = (uint8_t)value->ival;
}

static SeqMethods bytes_seq_methods = {
    .len = kl_bytes_seq_len,
    // .contains = kl_bytes_contains,
    .get = kl_bytes_seq_get,
    .set = kl_bytes_seq_set,
};

/* pub class bytes : MutableSequence[uint8] { ... } */
TypeObject bytes_type = {
    ._type = &type_type,
    .name = "bytes",
    .priv_size = 2 * sizeof(uint32_t) + sizeof(uint8_t *),
    .flags = TP_FLAGS_CLASS,
    .methdefs = bytes_methods,
    .seq = &bytes_seq_methods,
};

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

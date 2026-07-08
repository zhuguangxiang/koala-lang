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
        buf_write_uint8_hex(&buf, bytes->data[i]);
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

static MethodDef bytes_methods[] = {
    { "__len__", kl_bytes_len },
    { "__str__", kl_bytes_str },
    { "__init__", kl_bytes_init },
    { NULL },
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
    uint8_t value = bytes->data[index];
    return uint8_value(value);
}

static void kl_bytes_seq_set(TValue *self, size_t index, TValue *value)
{
    BytesObject *bytes = (BytesObject *)to_obj(self);
    if (index >= bytes->size) {
        panic("bytes index out of range");
    }
    ASSERT(is_uint8(value));
    bytes->data[index] = (uint8_t)value->ival;
}

static SeqMethods bytes_seq_methods = {
    .len = kl_bytes_seq_len,
    // .contains = kl_bytes_contains,
    .get = kl_bytes_seq_get,
    .set = kl_bytes_seq_set,
};

static Object *bytes_alloc(TypeObject *tp)
{
    BytesObject *bytes = mm_alloc_obj(bytes);
    INIT_OBJECT_HEAD(bytes, tp);
    bytes->size = 0;
    bytes->data = NULL;
    return (Object *)bytes;
}

/* pub class bytes : MutableSequence[uint8] { ... } */
TypeObject bytes_type = {
    ._type = &type_type,
    .name = "bytes",
    .flags = TP_FLAGS_CLASS | TP_FLAGS_PUBLIC,
    .methdefs = bytes_methods,
    .seq = &bytes_seq_methods,
    .alloc = bytes_alloc,
};

Object *kl_new_bytes(size_t size)
{
    BytesObject *bytes = mm_alloc_obj(bytes);
    INIT_OBJECT_HEAD(bytes, &bytes_type);
    bytes->size = size;
    bytes->data = mm_alloc(size);
    return (Object *)bytes;
}

#ifdef __cplusplus
}
#endif

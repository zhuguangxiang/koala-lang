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
    return no_value;
}

static TValue _bytebuf_write_byte(TValue *self, TValue *args, int nargs)
{
    ByteBufObject *bb = SELF_AS(bytebuf_type);
    ASSERT(nargs == 1);

    uint8_t value = kl_arg_uint8(0);
    buf_write_byte(&bb->buf, value);
    return no_value;
}

static TValue _bytebuf_write_str(TValue *self, TValue *args, int nargs)
{
    ByteBufObject *bb = SELF_AS(bytebuf_type);
    ASSERT(nargs == 1);

    Object *sobj = kl_arg_strobj(0);
    const char *data = STR_BUF(sobj);
    size_t len = STR_LEN(sobj);
    buf_write_nstr(&bb->buf, data, len);
    return no_value;
}

static TValue _bytebuf_write_bytes(TValue *self, TValue *args, int nargs)
{
    ByteBufObject *bb = SELF_AS(bytebuf_type);
    ASSERT(nargs == 1);

    BytesObject *bs = kl_arg_obj_as(0, bytes_type);
    char *data = (char *)bs->data + bs->offset;
    int size = bs->size;

    buf_write_nstr(&bb->buf, data, size);
    return no_value;
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

static TValue _bytebuf_to_bytes(TValue *self, TValue *args, int nargs)
{
    ByteBufObject *bb = SELF_AS(bytebuf_type);
    ASSERT(nargs == 0);

    uint32_t len = (uint32_t)BUF_LEN(bb->buf);
    Object *bs = kl_new_bytes(len);
    memcpy(BYTES_DATA(bs), BUF_STR(bb->buf), len);
    return obj_value(bs);
}

static TValue _bytebuf_clear(TValue *self, TValue *args, int nargs)
{
    ByteBufObject *bb = SELF_AS(bytebuf_type);
    ASSERT(nargs == 0);

    RESET_BUF(bb->buf);
    return no_value;
}

/* Clamp `[start, end)` to the buffer; a negative `end` means the buffer's end. */
static void clamp_range(ByteBufObject *bb, int64_t *start, int64_t *end)
{
    int64_t len = BUF_LEN(bb->buf);

    if (*end < 0 || *end > len) {
        *end = len;
    }

    if (*start < 0) {
        *start = 0;
    }
}

/* The first index of `value` in `[start, end)`, or -1. */
static int64_t bytebuf_find(ByteBufObject *bb, uint8_t value, int64_t start, int64_t end)
{
    clamp_range(bb, &start, &end);

    if (start >= end) {
        return -1;
    }

    char *base = BUF_STR(bb->buf);
    char *from = base + start;
    size_t span = (size_t)(end - start);

    void *p = memchr(from, value, span);
    if (p == NULL) {
        return -1;
    }

    return (char *)p - base;
}

/* Remove the byte at a valid `index`, shifting the tail left. */
static void bytebuf_delete_at(ByteBufObject *bb, int64_t index)
{
    char *base = BUF_STR(bb->buf);
    int64_t len = BUF_LEN(bb->buf);

    char *dst = base + index;
    char *src = dst + 1;
    size_t n = (size_t)(len - index - 1);
    memmove(dst, src, n);

    BUF_LEN(bb->buf) = len - 1;
}

static TValue _bytebuf_getitem(TValue *self, TValue *args, int nargs)
{
    ByteBufObject *bb = SELF_AS(bytebuf_type);
    ASSERT(nargs == 1);

    int64_t index = kl_arg_int64(0);
    int64_t len = BUF_LEN(bb->buf);

    if (!(index >= 0 && index < len)) {
        raise_exc_str("Buffer index out of range");
        return error_value;
    }

    char *base = BUF_STR(bb->buf);
    char v = base[index];

    return uint8_value(v);
}

static TValue _bytebuf_setitem(TValue *self, TValue *args, int nargs)
{
    ByteBufObject *bb = SELF_AS(bytebuf_type);
    ASSERT(nargs == 2);

    int64_t index = kl_arg_int64(0);
    uint8_t v = kl_arg_uint8(1);
    int64_t len = BUF_LEN(bb->buf);

    if (!(index >= 0 && index < len)) {
        raise_exc_str("Buffer index out of range");
        return error_value;
    }

    char *base = BUF_STR(bb->buf);
    base[index] = (char)v;

    return no_value;
}

static TValue _bytebuf_pop(TValue *self, TValue *args, int nargs)
{
    ByteBufObject *bb = SELF_AS(bytebuf_type);
    ASSERT(nargs == 1);

    int64_t index = kl_arg_int64(0);
    int64_t len = BUF_LEN(bb->buf);

    if (index == -1) {
        index = len - 1;
    }

    if (!(index >= 0 && index < len)) {
        raise_exc_str("Buffer pop: index out of range");
        return error_value;
    }

    char *base = BUF_STR(bb->buf);
    uint8_t value = base[index];

    bytebuf_delete_at(bb, index);

    return uint8_value(value);
}

static TValue _bytebuf_contains(TValue *self, TValue *args, int nargs)
{
    ByteBufObject *bb = SELF_AS(bytebuf_type);
    ASSERT(nargs == 1);

    uint8_t value = kl_arg_uint8(0);

    int64_t index = bytebuf_find(bb, value, 0, -1);
    return bool_value(index >= 0);
}

static TValue _bytebuf_index(TValue *self, TValue *args, int nargs)
{
    ByteBufObject *bb = SELF_AS(bytebuf_type);
    ASSERT(nargs == 3);

    uint8_t value = kl_arg_uint8(0);
    int64_t start = kl_arg_int64(1);
    int64_t end = kl_arg_int64(2);

    int64_t index = bytebuf_find(bb, value, start, end);
    return int64_value(index);
}

static TValue _bytebuf_count(TValue *self, TValue *args, int nargs)
{
    ByteBufObject *bb = SELF_AS(bytebuf_type);
    ASSERT(nargs == 3);

    uint8_t value = kl_arg_uint8(0);
    int64_t start = kl_arg_int64(1);
    int64_t end = kl_arg_int64(2);

    clamp_range(bb, &start, &end);

    char *base = BUF_STR(bb->buf);
    int64_t cnt = 0;

    for (int64_t i = start; i < end; i++) {
        uint8_t b = base[i];
        if (b == value) {
            cnt++;
        }
    }

    return int64_value(cnt);
}

static TValue _bytebuf_insert(TValue *self, TValue *args, int nargs)
{
    ByteBufObject *bb = SELF_AS(bytebuf_type);
    ASSERT(nargs == 2);

    int64_t index = kl_arg_int64(0);
    uint8_t value = kl_arg_uint8(1);
    int64_t len = BUF_LEN(bb->buf);

    if (!(index >= 0 && index <= len)) {
        raise_exc_str("Buffer.insert: index out of range");
        return error_value;
    }

    buf_reserve(&bb->buf, 1);

    char *base = BUF_STR(bb->buf);
    char *src = base + index;
    char *dst = src + 1;
    size_t n = (size_t)(len - index);
    memmove(dst, src, n);

    *src = (char)value;
    BUF_LEN(bb->buf) = len + 1;

    return no_value;
}

/* pub func extend(items Iterable[uint8]) */
static TValue _bytebuf_extend(TValue *self, TValue *args, int nargs)
{
    ByteBufObject *bb = SELF_AS(bytebuf_type);
    ASSERT(nargs == 1);

    Object *ob = kl_arg_obj(0);

    /* Fast path: another buffer, bulk copy. Extending a buffer with itself
     * falls through to the Sequence path, which never holds a pointer into
     * storage that buf_write_nstr may realloc. */
    if (IS_BYTEBUF(ob) && ob != (Object *)bb) {
        ByteBufObject *other = (ByteBufObject *)ob;
        char *data = BUF_STR(other->buf);
        int size = BUF_LEN(other->buf);

        buf_write_nstr(&bb->buf, data, size);
        return no_value;
    }

    /* Fast path: bytes, bulk copy. */
    if (IS_BYTES(ob)) {
        BytesObject *bs = (BytesObject *)ob;
        char *data = (char *)bs->data + bs->offset;
        int size = bs->size;

        buf_write_nstr(&bb->buf, data, size);
        return no_value;
    }

    /* Generic Sequence: the length is known, index each byte. */
    if (kl_is_seq(ob)) {
        TValue len = kl_call_len(args);
        if (is_error(&len)) {
            return error_value;
        }

        int64_t m = to_int64(&len);
        size_t need = (size_t)m;
        buf_reserve(&bb->buf, need);

        for (int64_t i = 0; i < m; ++i) {
            TValue idx = int64_value(i);
            TValue item = kl_call_get_item(args, &idx);
            if (is_error(&item)) {
                return error_value;
            }

            uint8_t value = to_uint8(&item);
            buf_write_byte(&bb->buf, value);
        }

        return no_value;
    }

    /* Generic Iterable: drive the iterator protocol. */
    if (kl_is_iter(ob)) {
        TValue it = kl_call_iter(args, 1);
        if (is_error(&it)) {
            return error_value;
        }

        TValue item = kl_call_next(&it);
        while (!is_error(&item) && !is_nil(&item)) {
            uint8_t value = to_uint8(&item);
            buf_write_byte(&bb->buf, value);

            item = kl_call_next(&it);
        }

        if (is_error(&item)) {
            return error_value;
        }

        return no_value;
    }

    /* The compiler rejects non-Iterable / wrong-element arguments at the call
     * site, so a well-typed `items` is always a Sequence or an Iterable and one
     * of the branches above returns. This point is unreachable. */
    UNREACHABLE();
}

/* pub func remove(value uint8) */
static TValue _bytebuf_remove(TValue *self, TValue *args, int nargs)
{
    ByteBufObject *bb = SELF_AS(bytebuf_type);
    ASSERT(nargs == 1);

    uint8_t value = kl_arg_uint8(0);

    int64_t index = bytebuf_find(bb, value, 0, -1);
    if (index < 0) {
        raise_exc_str("Buffer.remove: value not present");
        return error_value;
    }

    bytebuf_delete_at(bb, index);
    return no_value;
}

static MethodDef _bytebuf_methods[] = {
    { "__init__", _bytebuf_init },
    { "write_byte", _bytebuf_write_byte },
    { "write_str", _bytebuf_write_str },
    { "write", _bytebuf_write_bytes },
    { "__getitem__", _bytebuf_getitem },
    { "__setitem__", _bytebuf_setitem },
    { "__contains__", _bytebuf_contains },
    { "index", _bytebuf_index },
    { "count", _bytebuf_count },
    { "insert", _bytebuf_insert },
    { "extend", _bytebuf_extend },
    { "pop", _bytebuf_pop },
    { "remove", _bytebuf_remove },
    { "len", _bytebuf_len },
    { "to_str", _bytebuf_to_str },
    { "to_bytes", _bytebuf_to_bytes },
    { "clear", _bytebuf_clear },
    { NULL },
};

/* pub class Buffer { ... } */

TypeObject bytebuf_type = {
    ._type = &type_type,
    .name = "Buffer",
    .flags = TP_FLAGS_CLASS,
    .objsize = sizeof(ByteBufObject),
    .methdefs = _bytebuf_methods,
};

Object *kl_new_bytebuf(size_t size)
{
    ByteBufObject *bb = mm_alloc_obj(bb);
    INIT_OBJECT_HEAD(bb, &bytebuf_type, 0);
    buf_reserve(&bb->buf, size);
    return (Object *)bb;
}

void kl_free_bytebuf(Object *obj)
{
    ASSERT(OB_TYPE(obj) == &bytebuf_type);
    ByteBufObject *bb = (ByteBufObject *)obj;
    FINI_BUF(bb->buf);
    mm_free(obj);
}

#ifdef __cplusplus
}
#endif

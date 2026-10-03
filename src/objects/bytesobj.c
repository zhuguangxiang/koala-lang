/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#define _GNU_SOURCE // memrchr
#include "bytesobj.h"
#include <string.h>
#include "buffer.h"
#include "excobj.h"
#include "hashmap.h"
#include "listobj.h"

#ifdef __cplusplus
extern "C" {
#endif

static TValue _bytes_len(TValue *self, TValue *args, int nargs)
{
    BytesObject *bytes = SELF_AS(bytes_type);
    return int64_value(bytes->size);
}

static TValue _bytes_str(TValue *self, TValue *args, int nargs)
{
    BytesObject *bytes = SELF_AS(bytes_type);
    int64_t size = bytes->size;

    BUF(buf);
    for (int i = 0; i < size; ++i) {
        uint8_t ch = bytes->data[bytes->offset + i];
        if (ch == 0) break;
        buf_write_char(&buf, ch);
    }
    Object *sobj = kl_new_nstr(BUF_STR(buf), BUF_LEN(buf));
    FINI_BUF(buf);

    return obj_value(sobj);
}

static TValue _bytes_init(TValue *self, TValue *args, int nargs)
{
    BytesObject *bytes = SELF_AS(bytes_type);

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
    BytesObject *bytes = SELF_AS(bytes_type);

    ASSERT(nargs == 3);
    uint8_t value = kl_arg_uint8(0);
    int64_t start = kl_arg_int64(1);
    int64_t end = kl_arg_int64(2);

    uint8_t *base = bytes->data + bytes->offset;
    int64_t size = bytes->size;
    if (end < 0) end = size;
    if (start < 0) start = 0;
    if (end > size) end = size;
    if (start >= end) return int64_value(-1);

    void *ptr = memchr(base + start, value, end - start);
    if (ptr) {
        return int64_value((uint8_t *)ptr - base);
    } else {
        return int64_value(-1);
    }
}

static TValue _bytes_rindex(TValue *self, TValue *args, int nargs)
{
    BytesObject *bytes = SELF_AS(bytes_type);

    ASSERT(nargs == 3);
    uint8_t value = kl_arg_uint8(0);
    int64_t start = kl_arg_int64(1);
    int64_t end = kl_arg_int64(2);

    uint8_t *base = bytes->data + bytes->offset;
    int64_t size = bytes->size;
    if (end < 0) end = size;
    if (start < 0) start = 0;
    if (end > size) end = size;
    if (start >= end) return int64_value(-1);

    void *ptr = memrchr(base + start, value, end - start);
    if (ptr) {
        return int64_value((uint8_t *)ptr - base);
    } else {
        return int64_value(-1);
    }
}

static TValue _bytes_count(TValue *self, TValue *args, int nargs)
{
    BytesObject *bytes = SELF_AS(bytes_type);

    ASSERT(nargs == 3);
    uint8_t value = kl_arg_uint8(0);
    int64_t start = kl_arg_int64(1);
    int64_t end = kl_arg_int64(2);

    uint8_t *base = bytes->data + bytes->offset;
    int64_t size = bytes->size;
    if (end < 0) end = size;
    if (start < 0) start = 0;
    if (end > size) end = size;

    int count = 0;
    for (int64_t i = start; i < end; ++i) {
        if (base[i] == value) count++;
    }
    return int64_value(count);
}

static TValue _bytes_copy(TValue *self, TValue *args, int nargs)
{
    BytesObject *bytes = SELF_AS(bytes_type);

    ASSERT(nargs == 3);
    Object *src = to_obj(&args[0]);
    ASSERT(src && IS_BYTES(src));
    BytesObject *src_bytes = (BytesObject *)src;

    int64_t src_start = kl_arg_int64(1);
    int64_t src_end = kl_arg_int64(2);
    if (src_end < 0) src_end = src_bytes->size;
    if (src_start < 0) src_start = 0;
    if (src_end > src_bytes->size) src_end = src_bytes->size;

    int64_t len = src_end - src_start;
    if (len < 0) len = 0;
    if (len > bytes->size) return int64_value(-1);

    memmove(bytes->data + bytes->offset, src_bytes->data + src_bytes->offset + src_start, len);
    return int64_value(len);
}

static TValue _bytes_copy_str(TValue *self, TValue *args, int nargs)
{
    BytesObject *bytes = SELF_AS(bytes_type);

    ASSERT(nargs == 3);
    StringObject *src = kl_arg_obj_as(0, str_type);
    int64_t src_start = kl_arg_int64(1);
    int64_t src_end = kl_arg_int64(2);
    if (src_end < 0) src_end = src->size;
    if (src_start < 0) src_start = 0;
    if (src_end > (int64_t)src->size) src_end = (int64_t)src->size;

    int64_t len = src_end - src_start;
    if (len < 0) len = 0;
    if (len > bytes->size) return int64_value(-1);

    memmove(bytes->data + bytes->offset, src->array + src_start, len);
    return int64_value(len);
}

static TValue _bytes_fill(TValue *self, TValue *args, int nargs)
{
    BytesObject *bytes = SELF_AS(bytes_type);

    ASSERT(nargs == 3);
    uint8_t value = kl_arg_uint8(0);
    int64_t start = kl_arg_int64(1);
    int64_t end = kl_arg_int64(2);

    int64_t size = bytes->size;
    if (end < 0) end = size;
    if (start < 0) start = 0;
    if (end > size) end = size;
    if (start >= end) return nil_value;

    memset(bytes->data + bytes->offset + start, value, end - start);
    return nil_value;
}

static TValue _bytes_zero(TValue *self, TValue *args, int nargs)
{
    BytesObject *bytes = SELF_AS(bytes_type);

    ASSERT(nargs == 2);
    int64_t start = kl_arg_int64(0);
    int64_t end = kl_arg_int64(1);

    int64_t size = bytes->size;
    if (end < 0) end = size;
    if (start < 0) start = 0;
    if (end > size) end = size;
    if (start >= end) return nil_value;

    memset(bytes->data + bytes->offset + start, 0, end - start);
    return nil_value;
}

static TValue _bytes_view(TValue *self, TValue *args, int nargs)
{
    BytesObject *bytes = SELF_AS(bytes_type);

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
    BytesObject *bytes = SELF_AS(bytes_type);
    int64_t size = bytes->size;

    BUF(buf);
    for (int i = 0; i < size; ++i) {
        uint8_t ch = bytes->data[bytes->offset + i];
        if (ch == 0) break;
        buf_write_char(&buf, ch);
    }
    Object *sobj = kl_new_nstr(BUF_STR(buf), BUF_LEN(buf));
    FINI_BUF(buf);

    return obj_value(sobj);
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

static TValue _bytes_contains(TValue *self, TValue *args, int nargs)
{
    BytesObject *bytes = SELF_AS(bytes_type);

    ASSERT(nargs == 1);
    uint8_t value = kl_arg_uint8(0);
    for (int64_t i = 0; i < bytes->size; ++i) {
        if (bytes->data[bytes->offset + i] == value) {
            return bool_value(true);
        }
    }
    return bool_value(false);
}

static TValue _bytes_getslice(TValue *self, TValue *args, int nargs)
{
    BytesObject *bytes = SELF_AS(bytes_type);

    ASSERT(nargs == 1);
    SliceObject *slice = kl_arg_obj_as(0, slice_type);
    int64_t start = to_int64(&slice->start);
    int64_t end = to_int64(&slice->end);
    if (end < 0) end = bytes->size;
    ASSERT(start >= 0 && end >= 0 && start <= end && end <= bytes->size);

    BytesObject *view = mm_alloc_obj(view);
    INIT_OBJECT_HEAD(view, &bytes_type, 0);
    view->offset = bytes->offset + start;
    view->size = end - start;
    view->data = bytes->data;
    return obj_value((Object *)view);
}

static TValue _bytes_setslice(TValue *self, TValue *args, int nargs)
{
    BytesObject *bytes = SELF_AS(bytes_type);

    ASSERT(nargs == 2);
    SliceObject *slice = kl_arg_obj_as(0, slice_type);
    int64_t start = to_int64(&slice->start);
    int64_t end = to_int64(&slice->end);
    if (end < 0) end = bytes->size;
    ASSERT(start >= 0 && end >= 0 && start <= end && end <= bytes->size);

    BytesObject *value = kl_arg_obj_as(1, bytes_type);
    ASSERT(value->size == end - start);
    memcpy(bytes->data + bytes->offset + start, value->data + value->offset, end - start);

    return nil_value;
}

static TValue _bytes_eq(TValue *self, TValue *args, int nargs)
{
    BytesObject *lhs = SELF_AS(bytes_type);

    ASSERT(nargs == 1);
    BytesObject *rhs = kl_arg_obj_as(0, bytes_type);
    if (lhs->size != rhs->size) return bool_value(false);

    int r = memcmp(lhs->data + lhs->offset, rhs->data + rhs->offset, lhs->size);
    return bool_value(r == 0);
}

static TValue _bytes_ne(TValue *self, TValue *args, int nargs)
{
    TValue v = _bytes_eq(self, args, nargs);
    return bool_value(v.ival == 0);
}

static TValue _bytes_hash(TValue *self, TValue *args, int nargs)
{
    BytesObject *bytes = SELF_AS(bytes_type);
    unsigned int hash = mem_hash(bytes->data + bytes->offset, bytes->size);
    return int64_value(hash);
}

static TValue _bytes_index_bytes(TValue *self, TValue *args, int nargs)
{
    BytesObject *bytes = SELF_AS(bytes_type);

    ASSERT(nargs == 3);
    BytesObject *sub = kl_arg_obj_as(0, bytes_type);
    int64_t start = kl_arg_int64(1);
    int64_t end = kl_arg_int64(2);

    uint8_t *base = bytes->data + bytes->offset;
    int64_t size = bytes->size;
    if (end < 0) end = size;
    if (start < 0) start = 0;
    if (end > size) end = size;

    int64_t sub_size = sub->size;
    if (sub_size == 0) return int64_value(start <= end ? start : -1);
    if (start + sub_size > end) return int64_value(-1);

    uint8_t *sub_data = sub->data + sub->offset;
    for (int64_t i = start; i + sub_size <= end; ++i) {
        if (memcmp(base + i, sub_data, sub_size) == 0) return int64_value(i);
    }
    return int64_value(-1);
}

static TValue _bytes_rindex_bytes(TValue *self, TValue *args, int nargs)
{
    BytesObject *bytes = SELF_AS(bytes_type);

    ASSERT(nargs == 3);
    BytesObject *sub = kl_arg_obj_as(0, bytes_type);
    int64_t start = kl_arg_int64(1);
    int64_t end = kl_arg_int64(2);

    uint8_t *base = bytes->data + bytes->offset;
    int64_t size = bytes->size;
    if (end < 0) end = size;
    if (start < 0) start = 0;
    if (end > size) end = size;

    int64_t sub_size = sub->size;
    if (sub_size == 0) return int64_value(start <= end ? end : -1);
    if (start + sub_size > end) return int64_value(-1);

    uint8_t *sub_data = sub->data + sub->offset;
    for (int64_t i = end - sub_size; i >= start; --i) {
        if (memcmp(base + i, sub_data, sub_size) == 0) return int64_value(i);
    }
    return int64_value(-1);
}

static TValue _bytes_starts_with(TValue *self, TValue *args, int nargs)
{
    BytesObject *bytes = SELF_AS(bytes_type);

    ASSERT(nargs == 1);
    BytesObject *prefix = kl_arg_obj_as(0, bytes_type);

    if (prefix->size > bytes->size) return bool_value(false);
    int r = memcmp(bytes->data + bytes->offset, prefix->data + prefix->offset, prefix->size);
    return bool_value(r == 0);
}

static TValue _bytes_ends_with(TValue *self, TValue *args, int nargs)
{
    BytesObject *bytes = SELF_AS(bytes_type);

    ASSERT(nargs == 1);
    BytesObject *suffix = kl_arg_obj_as(0, bytes_type);

    if (suffix->size > bytes->size) return bool_value(false);
    int r = memcmp(bytes->data + bytes->offset + (bytes->size - suffix->size),
                   suffix->data + suffix->offset, suffix->size);
    return bool_value(r == 0);
}

static TValue _bytes_replace(TValue *self, TValue *args, int nargs)
{
    BytesObject *bytes = SELF_AS(bytes_type);

    ASSERT(nargs == 2);
    BytesObject *old = kl_arg_obj_as(0, bytes_type);
    BytesObject *neu = kl_arg_obj_as(1, bytes_type);

    uint8_t *base = bytes->data + bytes->offset;
    int64_t size = bytes->size;
    int64_t old_size = old->size;
    int64_t new_size = neu->size;

    if (old_size == 0) {
        Object *res = kl_bytes_from_data(base, (uint32_t)size);
        return obj_value(res);
    }

    uint8_t *old_data = old->data + old->offset;
    uint8_t *new_data = neu->data + neu->offset;

    int64_t count = 0;
    for (int64_t i = 0; i + old_size <= size;) {
        if (memcmp(base + i, old_data, old_size) == 0) {
            count++;
            i += old_size;
        } else {
            i++;
        }
    }

    if (count == 0) {
        Object *res = kl_bytes_from_data(base, (uint32_t)size);
        return obj_value(res);
    }

    int64_t out_size = size + count * (new_size - old_size);
    uint8_t *out = mm_alloc(out_size);
    int64_t oi = 0;
    for (int64_t i = 0; i < size;) {
        if (i + old_size <= size && memcmp(base + i, old_data, old_size) == 0) {
            if (new_size > 0) memcpy(out + oi, new_data, new_size);
            oi += new_size;
            i += old_size;
        } else {
            out[oi++] = base[i++];
        }
    }

    Object *res = kl_bytes_from_data(out, (uint32_t)out_size);
    mm_free(out);
    return obj_value(res);
}

static inline bool bytes_is_space(uint8_t c)
{
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

static TValue _bytes_split(TValue *self, TValue *args, int nargs)
{
    BytesObject *bytes = SELF_AS(bytes_type);

    ASSERT(nargs == 2);
    BytesObject *sep = kl_arg_obj_as(0, bytes_type);
    int64_t maxsplit = kl_arg_int64(1);

    uint8_t *base = bytes->data + bytes->offset;
    int64_t size = bytes->size;
    uint8_t *sep_data = sep->data + sep->offset;
    int64_t sep_size = sep->size;

    Object *list = kl_new_list();

    if (sep_size == 0) {
        Object *res = kl_bytes_from_data(base, (uint32_t)size);
        kl_list_append(list, obj_value(res));
        return obj_value(list);
    }

    int64_t start = 0;
    int64_t splits = 0;
    for (int64_t i = 0; i + sep_size <= size;) {
        if (maxsplit >= 0 && splits >= maxsplit) break;
        if (memcmp(base + i, sep_data, sep_size) == 0) {
            Object *res = kl_bytes_from_data(base + start, (uint32_t)(i - start));
            kl_list_append(list, obj_value(res));
            start = i + sep_size;
            splits++;
            i = start;
        } else {
            i++;
        }
    }

    Object *res = kl_bytes_from_data(base + start, (uint32_t)(size - start));
    kl_list_append(list, obj_value(res));
    return obj_value(list);
}

static TValue _bytes_rsplit(TValue *self, TValue *args, int nargs)
{
    BytesObject *bytes = SELF_AS(bytes_type);

    ASSERT(nargs == 2);
    BytesObject *sep = kl_arg_obj_as(0, bytes_type);
    int64_t maxsplit = kl_arg_int64(1);

    uint8_t *base = bytes->data + bytes->offset;
    int64_t size = bytes->size;
    uint8_t *sep_data = sep->data + sep->offset;
    int64_t sep_size = sep->size;

    Object *list = kl_new_list();

    if (sep_size == 0) {
        Object *res = kl_bytes_from_data(base, (uint32_t)size);
        kl_list_prepend(list, obj_value(res));
        return obj_value(list);
    }

    int64_t end = size;
    int64_t splits = 0;
    for (int64_t i = size - sep_size; i >= 0;) {
        if (maxsplit >= 0 && splits >= maxsplit) break;
        if (memcmp(base + i, sep_data, sep_size) == 0) {
            Object *res = kl_bytes_from_data(base + i + sep_size, (uint32_t)(end - (i + sep_size)));
            kl_list_prepend(list, obj_value(res));
            end = i;
            splits++;
            i = end - sep_size;
        } else {
            i--;
        }
    }

    Object *res = kl_bytes_from_data(base, (uint32_t)end);
    kl_list_prepend(list, obj_value(res));
    return obj_value(list);
}

static TValue _bytes_splitlines(TValue *self, TValue *args, int nargs)
{
    BytesObject *bytes = SELF_AS(bytes_type);

    ASSERT(nargs == 0);
    uint8_t *base = bytes->data + bytes->offset;
    int64_t size = bytes->size;

    Object *list = kl_new_list();
    int64_t line_start = 0;

    for (int64_t i = 0; i < size; i++) {
        uint8_t c = base[i];
        if (c == '\n') {
            Object *res = kl_bytes_from_data(base + line_start, (uint32_t)(i - line_start));
            kl_list_append(list, obj_value(res));
            line_start = i + 1;
        } else if (c == '\r') {
            Object *res = kl_bytes_from_data(base + line_start, (uint32_t)(i - line_start));
            kl_list_append(list, obj_value(res));
            if (i + 1 < size && base[i + 1] == '\n') {
                line_start = i + 2;
                i++;
            } else {
                line_start = i + 1;
            }
        }
    }

    if (line_start < size) {
        Object *res = kl_bytes_from_data(base + line_start, (uint32_t)(size - line_start));
        kl_list_append(list, obj_value(res));
    }
    return obj_value(list);
}

static TValue _bytes_strip(TValue *self, TValue *args, int nargs)
{
    BytesObject *bytes = SELF_AS(bytes_type);

    ASSERT(nargs == 0);
    uint8_t *base = bytes->data + bytes->offset;
    int64_t size = bytes->size;

    int64_t start = 0;
    while (start < size && bytes_is_space(base[start])) start++;
    int64_t end = size;
    while (end > start && bytes_is_space(base[end - 1])) end--;

    Object *res = kl_bytes_from_data(base + start, (uint32_t)(end - start));
    return obj_value(res);
}

static TValue _bytes_lstrip(TValue *self, TValue *args, int nargs)
{
    BytesObject *bytes = SELF_AS(bytes_type);

    ASSERT(nargs == 0);
    uint8_t *base = bytes->data + bytes->offset;
    int64_t size = bytes->size;

    int64_t start = 0;
    while (start < size && bytes_is_space(base[start])) start++;

    Object *res = kl_bytes_from_data(base + start, (uint32_t)(size - start));
    return obj_value(res);
}

static TValue _bytes_rstrip(TValue *self, TValue *args, int nargs)
{
    BytesObject *bytes = SELF_AS(bytes_type);

    ASSERT(nargs == 0);
    uint8_t *base = bytes->data + bytes->offset;
    int64_t size = bytes->size;

    int64_t end = size;
    while (end > 0 && bytes_is_space(base[end - 1])) end--;

    Object *res = kl_bytes_from_data(base, (uint32_t)end);
    return obj_value(res);
}

static MethodDef bytes_methods[] = {
    { "len", _bytes_len },
    { "__str__", _bytes_str },
    { "__init__", _bytes_init },
    { "__getitem__", _bytes_getitem },
    { "__setitem__", _bytes_setitem },
    { "__getslice__", _bytes_getslice },
    { "__setslice__", _bytes_setslice },
    { "__contains__", _bytes_contains },
    { "__eq__", _bytes_eq },
    { "__ne__", _bytes_ne },
    { "hash", _bytes_hash },
    { "index", _bytes_index },
    { "rindex", _bytes_rindex },
    { "count", _bytes_count },
    { "copy", _bytes_copy },
    { "copy_str", _bytes_copy_str },
    { "fill", _bytes_fill },
    { "zero", _bytes_zero },
    { "view", _bytes_view },
    { "to_str", _bytes_tostr },
    { "index_bytes", _bytes_index_bytes },
    { "rindex_bytes", _bytes_rindex_bytes },
    { "starts_with", _bytes_starts_with },
    { "ends_with", _bytes_ends_with },
    { "replace", _bytes_replace },
    { "split", _bytes_split },
    { "rsplit", _bytes_rsplit },
    { "splitlines", _bytes_splitlines },
    { "strip", _bytes_strip },
    { "lstrip", _bytes_lstrip },
    { "rstrip", _bytes_rstrip },
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

Object *kl_bytes_from_data(uint8_t *data, uint32_t size)
{
    BytesObject *bytes = mm_alloc_obj(bytes);
    INIT_OBJECT_HEAD(bytes, &bytes_type, 0);
    bytes->offset = 0;
    bytes->size = size;
    bytes->data = mm_alloc(size);
    memcpy(bytes->data, data, size);
    return (Object *)bytes;
}

#ifdef __cplusplus
}
#endif

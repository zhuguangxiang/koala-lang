/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "bytesobj.h"
#include "listobj.h"
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

static TValue str_str(TValue *self, TValue *args, int nargs) { return *self; }

//
// pub func split(sep = " ") list[str] {}
// Split a UTF-8 string by a separator and return list[str]
//
static TValue str_split(TValue *self, TValue *args, int nargs)
{
    Object *ob = SELF_AS(str_type);

    ASSERT(nargs == 1);

    char *src = STR_BUF(ob);
    char *sep = kl_arg_str(0);
    int len = STR_LEN(ob);
    int sep_len = strlen(sep);

    // Separator must not be empty, compiler should have checked this.
    ASSERT(sep_len > 0);

    Object *list = kl_new_list();

    //
    // FAST PATH: single-byte separator
    //
    if (sep_len == 1) {
        char c = sep[0];
        int start = 0;

        for (int i = 0; i < len; i++) {
            if (src[i] == c) {
                int part_len = i - start;

                TValue part = kl_val_nstr(src + start, part_len);
                kl_list_append(list, part);

                start = i + 1;
            }
        }

        // Final segment
        if (start <= len) {
            TValue part = kl_val_nstr(src + start, len - start);
            kl_list_append(list, part);
        }

        return obj_value(list);
    }

    int start = 0;

    // Scan for separator occurrences
    for (int i = 0; i <= len - sep_len; i++) {
        // Found separator
        if (memcmp(src + i, sep, sep_len) == 0) {
            int part_len = i - start;

            // Create substring (copy)
            TValue part = kl_val_nstr(src + start, part_len);
            kl_list_append(list, part);

            // Move start to the end of separator
            start = i + sep_len;

            // Skip ahead by separator length
            i += sep_len - 1;
        }
    }

    // Final segment after the last separator
    if (start <= len) {
        TValue part = kl_val_nstr(src + start, len - start);
        kl_list_append(list, part);
    }

    return obj_value(list);
}

static TValue str_to_bytes(TValue *self, TValue *args, int nargs)
{
    Object *obj = to_obj(self);
    ASSERT(obj && IS_STR(obj));

    StringObject *str = (StringObject *)obj;
    ASSERT(nargs == 0);

    Object *bs = kl_new_bytes((uint32_t)str->size);
    memcpy(((BytesObject *)bs)->data, str->array, str->size);
    return obj_value(bs);
}

static MethodDef str_methods[] = {
    { "__str__", str_str },
    { "to_bytes", str_to_bytes },
    { "split", str_split },
    { NULL },
};

TypeObject str_type = {
    ._type = &type_type,
    .name = "str",
    .flags = TP_FLAGS_CLASS | TP_FLAGS_PUBLIC,
    .methdefs = str_methods,
};

static StringObject empty_str = {
    ._type = &str_type,
    .size = 0,
    .array = "",
};

Object *kl_new_nstr(char *s, size_t len)
{
    if (len == 0) {
        return (Object *)&empty_str;
    }

    StringObject *x = mm_alloc_obj(x);
    INIT_OBJECT_HEAD(x, &str_type);
    x->size = len;

    char *data = mm_alloc(len + 1);
    memcpy(data, s, len);
    data[len] = '\0';
    x->array = data;

    return (Object *)x;
}

Object *kl_new_fmt_str(char *fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, 255, fmt, args);
    va_end(args);
    buf[len] = '\0';
    return kl_new_nstr(buf, len);
}

void kl_free_str(Object *obj)
{
    StringObject *sobj = (StringObject *)obj;
    if (sobj->size > 0) {
        mm_free(sobj->array);
    }
    mm_free(sobj);
}

#ifdef __cplusplus
}
#endif

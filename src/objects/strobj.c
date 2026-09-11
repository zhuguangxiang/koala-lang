/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "buffer.h"
#include "bytesobj.h"
#include "except.h"
#include "listobj.h"
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

static TValue _str_init(TValue *self, TValue *args, int nargs) { UNREACHABLE(); }

static TValue _str_str(TValue *self, TValue *args, int nargs) { return *self; }

//
// pub func split(sep = " ", maxsplit = -1) list[str] {}
// Split a UTF-8 string by a separator and return list[str]
//
static TValue _str_split(TValue *self, TValue *args, int nargs)
{
    Object *ob = SELF_AS(str_type);

    ASSERT(nargs == 2);

    char *src = STR_BUF(ob);
    char *sep = kl_arg_str(0);
    int len = STR_LEN(ob);
    int sep_len = strlen(sep);
    int maxsplit = kl_arg_int64(1);

    // Separator must not be empty, compiler should have checked this.
    ASSERT(sep_len > 0);

    Object *list = kl_new_list();

    // FAST PATH: single-byte separator
    if (sep_len == 1) {
        char c = sep[0];
        int start = 0;
        int splits = 0;

        for (int i = 0; i < len; i++) {
            if (src[i] == c) {
                // If maxsplit reached, stop splitting
                if (maxsplit >= 0 && splits >= maxsplit) {
                    break;
                }

                int part_len = i - start;

                TValue part = kl_val_nstr(src + start, part_len);
                kl_list_append(list, part);

                start = i + 1;
                splits++;
            }
        }

        // Final segment (always appended)
        if (start <= len) {
            TValue part = kl_val_nstr(src + start, len - start);
            kl_list_append(list, part);
        }

        return obj_value(list);
    }

    // General path: multi-byte separator

    int start = 0;
    int splits = 0;

    // Scan for separator occurrences
    for (int i = 0; i <= len - sep_len; i++) {
        // If maxsplit reached, stop scanning
        if (maxsplit >= 0 && splits >= maxsplit) {
            break;
        }

        // Found separator
        if (memcmp(src + i, sep, sep_len) == 0) {
            int part_len = i - start;

            // Create substring (copy)
            TValue part = kl_val_nstr(src + start, part_len);
            kl_list_append(list, part);

            // Move start to the end of separator
            start = i + sep_len;
            splits++;

            // Skip ahead by separator length
            i += sep_len - 1;
        }
    }

    // Final segment after the last split or after maxsplit reached
    if (start <= len) {
        TValue part = kl_val_nstr(src + start, len - start);
        kl_list_append(list, part);
    }

    return obj_value(list);
}

//
// pub func rsplit(sep = " ", maxsplit = -1) list[str] {}
// Split a UTF-8 string by a separator from the right.
//
static TValue _str_rsplit(TValue *self, TValue *args, int nargs)
{
    Object *ob = SELF_AS(str_type);

    ASSERT(nargs == 2);

    char *src = STR_BUF(ob);
    char *sep = kl_arg_str(0);
    int len = STR_LEN(ob);
    int sep_len = strlen(sep);
    int maxsplit = kl_arg_int64(1);

    ASSERT(sep_len > 0);

    Object *list = kl_new_list();

    //
    // FAST PATH: single-byte separator
    //
    if (sep_len == 1) {
        char c = sep[0];
        int end = len;
        int splits = 0;

        for (int i = len - 1; i >= 0; i--) {
            if (maxsplit >= 0 && splits >= maxsplit) break;

            if (src[i] == c) {
                int part_len = end - (i + 1);

                TValue part = kl_val_nstr(src + i + 1, part_len);
                kl_list_prepend(list, part);

                end = i;
                splits++;
            }
        }

        // Final segment (always appended)
        if (end >= 0) {
            TValue part = kl_val_nstr(src, end);
            kl_list_prepend(list, part);
        }

        return obj_value(list);
    }

    //
    // General path: multi-byte separator
    //
    int end = len;
    int splits = 0;

    for (int i = len - sep_len; i >= 0; i--) {
        if (maxsplit >= 0 && splits >= maxsplit) break;

        if (memcmp(src + i, sep, sep_len) == 0) {
            int part_len = end - (i + sep_len);

            TValue part = kl_val_nstr(src + i + sep_len, part_len);
            kl_list_prepend(list, part);

            end = i;
            splits++;

            i -= (sep_len - 1);
        }
    }

    // Final segment
    if (end >= 0) {
        TValue part = kl_val_nstr(src, end);
        kl_list_prepend(list, part);
    }

    return obj_value(list);
}

// pub func splitlines() list[str] {}
static TValue _str_splitlines(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);

    StringObject *s = SELF_AS(str_type);
    char *src = STR_BUF(s);
    size_t size = STR_LEN(s);

    Object *list = kl_new_list();

    size_t line_start = 0;

    for (size_t i = 0; i < size; i++) {
        char c = src[i];

        if (c == '\n') {
            // [line_start, i)
            TValue part = kl_val_nstr(src + line_start, i - line_start);
            kl_list_append(list, part);
            line_start = i + 1;
        } else if (c == '\r') {
            // [line_start, i)
            TValue part = kl_val_nstr(src + line_start, i - line_start);
            kl_list_append(list, part);

            if (i + 1 < size && src[i + 1] == '\n') {
                // handle \r\n
                line_start = i + 2;
                i++; // skip '\n'
            } else {
                line_start = i + 1;
            }
        }
    }

    // final line (if any)
    if (line_start < size) {
        TValue part = kl_val_nstr(src + line_start, size - line_start);
        kl_list_append(list, part);
    }

    return obj_value(list);
}

static TValue _str_to_bytes(TValue *self, TValue *args, int nargs)
{
    Object *obj = to_obj(self);
    ASSERT(obj && IS_STR(obj));

    StringObject *str = (StringObject *)obj;
    ASSERT(nargs == 0);

    Object *bs = kl_new_bytes((uint32_t)STR_LEN(str));
    memcpy(((BytesObject *)bs)->data, STR_BUF(str), STR_LEN(str));
    return obj_value(bs);
}

static TValue _str_empty(TValue *self, TValue *args, int nargs)
{
    StringObject *str = SELF_AS(str_type);
    return bool_value(str->size == 0);
}

#define DEFINE_STR_CMP_CODE(op) \
    do { \
        StringObject *s = SELF_AS(str_type); \
        ASSERT(nargs == 1); \
        Object *ob = to_obj(args); \
        ASSERT(IS_STR(ob)); \
        StringObject *o = (StringObject *)ob; \
        int n = s->size < o->size ? s->size : o->size; \
        int r = memcmp(s->array, o->array, n); \
        if (r == 0) r = (s->size - o->size); \
        bool x = r op 0; \
        return bool_value(x); \
    } while (0)

static TValue _str_equal(TValue *self, TValue *args, int nargs) { DEFINE_STR_CMP_CODE(==); }

static TValue _str_ne(TValue *self, TValue *args, int nargs)
{
    TValue v = _str_equal(self, args, nargs);
    v = bool_value(v.ival == 0);
    return v;
}

static TValue _str_lt(TValue *self, TValue *args, int nargs) { DEFINE_STR_CMP_CODE(<); }

static TValue _str_le(TValue *self, TValue *args, int nargs) { DEFINE_STR_CMP_CODE(<=); }

static TValue _str_gt(TValue *self, TValue *args, int nargs)
{
    // gt = NOT (le)
    TValue le = _str_le(self, args, nargs);
    return bool_value(le.ival == 0);
}

static TValue _str_ge(TValue *self, TValue *args, int nargs)
{
    // ge = NOT (lt)
    TValue lt = _str_lt(self, args, nargs);
    return bool_value(lt.ival == 0);
}

static TValue _str_hash(TValue *self, TValue *args, int nargs)
{
    StringObject *str = SELF_AS(str_type);
    return int64_value(str_hash(str->array));
}

TValue kl_format(TValue *self, TValue *args, int nargs);

static TValue _str_format(TValue *self, TValue *args, int nargs)
{
    StringObject *s = SELF_AS(str_type);

    if (nargs == 0) return *self;

    ASSERT(nargs == 1);
    TValue _args[] = { *self, *args };
    TValue v = kl_format(NULL, _args, 2);
    return v;
}

static TValue _str_add(TValue *self, TValue *args, int nargs)
{
    StringObject *s = SELF_AS(str_type);
    ASSERT(nargs == 1);
    Object *ob;
    if (is_val(args)) {
        ob = kl_to_str(args);
    } else {
        ob = to_obj(args);
    }
    ASSERT(IS_STR(ob));
    StringObject *o = (StringObject *)ob;

    BUF(buf);
    buf_write_nstr(&buf, s->array, s->size);
    buf_write_nstr(&buf, o->array, o->size);
    return kl_val_str_from_buf(buf);
}

static TValue _str_mul(TValue *self, TValue *args, int nargs)
{
    StringObject *s = SELF_AS(str_type);
    ASSERT(nargs == 1);
    int n = kl_arg_int64(0);
    ASSERT(n >= 0);

    BUF(buf);

    for (int i = 0; i < n; i++) {
        buf_write_nstr(&buf, s->array, s->size);
    }

    return kl_val_str_from_buf(buf);
}

static TValue _str_join(TValue *self, TValue *args, int nargs)
{
    StringObject *s = SELF_AS(str_type);
    ASSERT(nargs == 1);
    Object *ob = to_obj(args);

    BUF(buf);

    if (IS_LIST(ob)) {
        // FAST PATH: list
        ListObject *list = (ListObject *)ob;

        size_t i;

        for (i = list->start; i < list->end - 1; i++) {
            TValue *_v = list->array + i;
            Object *_ob = to_obj(_v);
            ASSERT(IS_STR(_ob));
            buf_write_nstr(&buf, STR_BUF(_ob), STR_LEN(_ob));
            // seperator
            buf_write_nstr(&buf, STR_BUF(s), STR_LEN(s));
        }

        // last one
        TValue *_v = list->array + i;
        Object *_ob = to_obj(_v);
        ASSERT(IS_STR(_ob));
        buf_write_nstr(&buf, STR_BUF(_ob), STR_LEN(_ob));
    } else {
        NYI();
    }

    return kl_val_str_from_buf(buf);
}

// pub func index(value str, start = 0, end = -1) int {}
static TValue _str_index(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 3);

    StringObject *s = SELF_AS(str_type);
    StringObject *sub = kl_arg_strobj(0);

    int start = kl_arg_int64(1);
    int end = kl_arg_int64(2);

    const char *src = STR_BUF(s);
    size_t size = STR_LEN(s);

    const char *pat = STR_BUF(sub);
    size_t pat_size = STR_LEN(sub);

    // normalize end
    if (end < 0 || end > (int)size) end = (int)size;

    // clamp start
    if (start < 0) start = 0;
    if (start > end) start = end;

    // empty pattern → always match at start
    if (pat_size == 0) {
        return int64_value(start);
    }

    // search range
    size_t search_len = end - start;

    // memmem search
    const char *pos = memmem(src + start, search_len, pat, pat_size);

    if (!pos) return int64_value(-1);

    // return byte index (Koala str index = byte index)
    return int64_value((int)(pos - src));
}

// pub func rindex(value str, start = 0, end = -1) int {}
static TValue _str_rindex(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 3);

    StringObject *s = SELF_AS(str_type);
    StringObject *sub = kl_arg_strobj(0);

    int start = kl_arg_int64(1);
    int end = kl_arg_int64(2);

    const char *src = STR_BUF(s);
    size_t size = STR_LEN(s);

    const char *pat = STR_BUF(sub);
    size_t pat_size = STR_LEN(sub);

    // normalize end
    if (end < 0 || end > (int)size) end = (int)size;

    // clamp start
    if (start < 0) start = 0;
    if (start > end) start = end;

    // empty pattern → match at end (Python semantics)
    if (pat_size == 0) return int64_value(end);

    // search range
    size_t search_len = end - start;

    // rindex: scan from right to left
    // last possible match starts at: end - pat_size
    for (int i = end - pat_size; i >= start; i--) {
        if (memcmp(src + i, pat, pat_size) == 0) {
            return int64_value(i);
        }
    }

    return int64_value(-1);
}

// pub func count(value str, start = 0, end = -1) int {}
static TValue _str_count(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 3);

    StringObject *s = SELF_AS(str_type);
    StringObject *sub = kl_arg_strobj(0);

    int64_t start = kl_arg_int64(1);
    int64_t end = kl_arg_int64(2);

    const char *src = STR_BUF(s);
    size_t size = STR_LEN(s);

    const char *pat = STR_BUF(sub);
    size_t pat_size = STR_LEN(sub);

    // normalize end
    if (end < 0 || end > (int64_t)size) end = (int64_t)size;

    // clamp start
    if (start < 0) start = 0;
    if (start > end) start = end;

    // empty pattern → Python semantics: count of "" is (end - start + 1)
    if (pat_size == 0) {
        return int64_value((end - start) + 1);
    }

    size_t search_len = end - start;
    int count = 0;

    // scan from left to right
    size_t i = start;
    while (i + pat_size <= (size_t)end) {
        if (memcmp(src + i, pat, pat_size) == 0) {
            count++;
            i += pat_size; // non-overlapping matches (Python semantics)
        } else {
            i++;
        }
    }

    return int64_value(count);
}

// pub func substr(start = 0, end = -1) str {}
static TValue _str_substr(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);

    StringObject *s = SELF_AS(str_type);

    int start = kl_arg_int64(0);
    int end = kl_arg_int64(1);

    char *src = STR_BUF(s);
    size_t size = STR_LEN(s);

    // normalize end
    if (end < 0 || end > (int)size) end = (int)size;

    // clamp start
    if (start < 0) start = 0;
    if (start > end) start = end;

    size_t len = end - start;

    // create new string (copy bytes)
    TValue val = kl_val_nstr(src + start, len);
    return val;
}

// pub func to_int() int {}
static TValue _str_to_int(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);

    StringObject *s = SELF_AS(str_type);
    const char *buf = STR_BUF(s);
    size_t len = STR_LEN(s);

    int ok = 0;
    int64_t val = str_to_int(buf, len, &ok);

    if (!ok) kl_panic("invalid integer literal");

    return int64_value(val);
}

// pub func to_int_or(default int) int {}
static TValue _str_to_int_or(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);

    int64_t def = kl_arg_int64(0);

    StringObject *s = SELF_AS(str_type);
    const char *buf = STR_BUF(s);
    size_t len = STR_LEN(s);

    int ok = 0;
    int64_t val = str_to_int(buf, len, &ok);

    if (!ok) return int64_value(def);

    return int64_value(val);
}

// pub func to_float() float64 {}
static TValue _str_to_float(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);

    StringObject *s = SELF_AS(str_type);
    const char *buf = STR_BUF(s);
    size_t len = STR_LEN(s);

    int ok = 0;
    double val = str_to_float(buf, len, &ok);

    if (!ok) kl_panic("invalid float literal");

    return float64_value(val);
}

// pub func to_float_or(default float64) float64 {}
static TValue _str_to_float_or(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);

    double def = kl_arg_float64(0);

    StringObject *s = SELF_AS(str_type);
    const char *buf = STR_BUF(s);
    size_t len = STR_LEN(s);

    int ok = 0;
    double val = str_to_float(buf, len, &ok);

    if (!ok) return float64_value(def);

    return float64_value(val);
}

// pub func upper() str {}
static TValue _str_upper(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);

    StringObject *s = SELF_AS(str_type);
    const char *buf = STR_BUF(s);
    size_t len = STR_LEN(s);

    if (len == 0) return obj_value(kl_new_nstr("", 0));

    char *out = mm_alloc(len);
    for (size_t i = 0; i < len; i++) {
        unsigned char c = buf[i];
        if (c >= 'a' && c <= 'z')
            out[i] = c - 32; // ASCII upper
        else
            out[i] = c;
    }

    Object *sobj = kl_new_nstr(out, len);
    mm_free(out);
    return obj_value(sobj);
}

// pub func lower() str {}
static TValue _str_lower(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);

    StringObject *s = SELF_AS(str_type);
    const char *buf = STR_BUF(s);
    size_t len = STR_LEN(s);

    if (len == 0) return obj_value(kl_new_nstr("", 0));

    char *out = mm_alloc(len);
    for (size_t i = 0; i < len; i++) {
        unsigned char c = buf[i];
        if (c >= 'A' && c <= 'Z')
            out[i] = c + 32; // ASCII lower
        else
            out[i] = c;
    }

    Object *sobj = kl_new_nstr(out, len);
    mm_free(out);
    return obj_value(sobj);
}

// pub func capitalize() str {}
static TValue _str_capitalize(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);

    StringObject *s = SELF_AS(str_type);
    const char *buf = STR_BUF(s);
    size_t len = STR_LEN(s);

    if (len == 0) return obj_value(kl_new_nstr("", 0));

    char *out = mm_alloc(len);

    // first char
    unsigned char c0 = buf[0];
    if (c0 >= 'a' && c0 <= 'z')
        out[0] = c0 - 32;
    else
        out[0] = c0;

    // rest
    for (size_t i = 1; i < len; i++) {
        unsigned char c = buf[i];
        if (c >= 'A' && c <= 'Z')
            out[i] = c + 32;
        else
            out[i] = c;
    }

    Object *sobj = kl_new_nstr(out, len);
    mm_free(out);
    return obj_value(sobj);
}

static TValue _str_isdigit(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);

    StringObject *s = SELF_AS(str_type);
    const char *buf = STR_BUF(s);
    size_t len = STR_LEN(s);

    if (len == 0) return bool_value(false);

    for (size_t i = 0; i < len; i++) {
        unsigned char c = buf[i];
        if (c < '0' || c > '9') return bool_value(false);
    }
    return bool_value(true);
}

static TValue _str_isalpha(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);

    StringObject *s = SELF_AS(str_type);
    const char *buf = STR_BUF(s);
    size_t len = STR_LEN(s);

    if (len == 0) return bool_value(false);

    for (size_t i = 0; i < len; i++) {
        unsigned char c = buf[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))) return bool_value(false);
    }
    return bool_value(true);
}

static TValue _str_isalnum(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);

    StringObject *s = SELF_AS(str_type);
    const char *buf = STR_BUF(s);
    size_t len = STR_LEN(s);

    if (len == 0) return bool_value(false);

    for (size_t i = 0; i < len; i++) {
        unsigned char c = buf[i];
        if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')))
            return bool_value(false);
    }
    return bool_value(true);
}

static TValue _str_isspace(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);

    StringObject *s = SELF_AS(str_type);
    const char *buf = STR_BUF(s);
    size_t len = STR_LEN(s);

    if (len == 0) return bool_value(false);

    for (size_t i = 0; i < len; i++) {
        unsigned char c = buf[i];
        if (!(c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f'))
            return bool_value(false);
    }
    return bool_value(true);
}

static TValue _str_islower(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);

    StringObject *s = SELF_AS(str_type);
    const char *buf = STR_BUF(s);
    size_t len = STR_LEN(s);

    if (len == 0) return bool_value(false);

    for (size_t i = 0; i < len; i++) {
        unsigned char c = buf[i];
        if (!(c >= 'a' && c <= 'z')) return bool_value(false);
    }
    return bool_value(true);
}

static TValue _str_isupper(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);

    StringObject *s = SELF_AS(str_type);
    const char *buf = STR_BUF(s);
    size_t len = STR_LEN(s);

    if (len == 0) return bool_value(false);

    for (size_t i = 0; i < len; i++) {
        unsigned char c = buf[i];
        if (!(c >= 'A' && c <= 'Z')) return bool_value(false);
    }
    return bool_value(true);
}

static TValue _str_center(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs >= 1 && nargs <= 2);

    StringObject *s = SELF_AS(str_type);
    const char *buf = STR_BUF(s);
    size_t len = STR_LEN(s);

    int64_t width = kl_arg_int64(0);
    const char *fill = " ";
    size_t fill_len = 1;

    if (nargs == 2) {
        StringObject *fs = kl_arg_strobj(1);
        fill = STR_BUF(fs);
        fill_len = STR_LEN(fs);

        if (fill_len != 1) kl_panic("center(): fill must be a single character");
    }

    // width <= len → return original
    if (width <= (int64_t)len) return obj_value(s);

    int64_t total = width - (int64_t)len;
    int64_t left = total / 2;
    int64_t right = total - left;

    size_t out_len = (size_t)width;
    char *out = mm_alloc(out_len);

    // fill left
    for (int64_t i = 0; i < left; i++) out[i] = fill[0];

    // copy original
    memcpy(out + left, buf, len);

    // fill right
    for (int64_t i = 0; i < right; i++) out[left + len + i] = fill[0];

    Object *sobj = kl_new_nstr(out, out_len);
    mm_free(out);
    return obj_value(sobj);
}

static TValue _str_ljust(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs >= 1 && nargs <= 2);

    StringObject *s = SELF_AS(str_type);
    const char *buf = STR_BUF(s);
    size_t len = STR_LEN(s);

    int64_t width = kl_arg_int64(0);
    const char *fill = " ";
    size_t fill_len = 1;

    if (nargs == 2) {
        StringObject *fs = kl_arg_strobj(1);
        fill = STR_BUF(fs);
        fill_len = STR_LEN(fs);
        if (fill_len != 1) kl_panic("ljust(): fill must be a single character");
    }

    if (width <= (int64_t)len) return obj_value(s);

    int64_t pad = width - (int64_t)len;
    size_t out_len = (size_t)width;

    char *out = mm_alloc(out_len);

    memcpy(out, buf, len);

    for (int64_t i = 0; i < pad; i++) out[len + i] = fill[0];

    Object *sobj = kl_new_nstr(out, out_len);
    mm_free(out);
    return obj_value(sobj);
}

static TValue _str_rjust(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs >= 1 && nargs <= 2);

    StringObject *s = SELF_AS(str_type);
    const char *buf = STR_BUF(s);
    size_t len = STR_LEN(s);

    int64_t width = kl_arg_int64(0);
    const char *fill = " ";
    size_t fill_len = 1;

    if (nargs == 2) {
        StringObject *fs = kl_arg_strobj(1);
        fill = STR_BUF(fs);
        fill_len = STR_LEN(fs);
        if (fill_len != 1) kl_panic("rjust(): fill must be a single character");
    }

    if (width <= (int64_t)len) return obj_value(s);

    int64_t pad = width - (int64_t)len;
    size_t out_len = (size_t)width;

    char *out = mm_alloc(out_len);

    for (int64_t i = 0; i < pad; i++) out[i] = fill[0];

    memcpy(out + pad, buf, len);

    Object *sobj = kl_new_nstr(out, out_len);
    mm_free(out);
    return obj_value(sobj);
}

static inline bool is_space(unsigned char c)
{
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f');
}

static TValue _str_strip(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);

    StringObject *s = SELF_AS(str_type);
    const char *buf = STR_BUF(s);
    size_t len = STR_LEN(s);

    if (len == 0) return obj_value(kl_new_nstr("", 0));

    size_t start = 0;
    while (start < len && is_space((unsigned char)buf[start])) start++;

    if (start == len) return obj_value(kl_new_nstr("", 0));

    size_t end = len - 1;
    while (end > start && is_space((unsigned char)buf[end])) end--;

    size_t out_len = end - start + 1;
    char *out = mm_alloc(out_len);

    memcpy(out, buf + start, out_len);

    Object *sobj = kl_new_nstr(out, out_len);
    mm_free(out);
    return obj_value(sobj);
}

static TValue _str_lstrip(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);

    StringObject *s = SELF_AS(str_type);
    const char *buf = STR_BUF(s);
    size_t len = STR_LEN(s);

    if (len == 0) return obj_value(kl_new_nstr("", 0));

    size_t start = 0;
    while (start < len && is_space((unsigned char)buf[start])) start++;

    size_t out_len = len - start;
    char *out = mm_alloc(out_len);

    memcpy(out, buf + start, out_len);

    Object *sobj = kl_new_nstr(out, out_len);
    mm_free(out);
    return obj_value(sobj);
}

static TValue _str_rstrip(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);

    StringObject *s = SELF_AS(str_type);
    const char *buf = STR_BUF(s);
    size_t len = STR_LEN(s);

    if (len == 0) return obj_value(kl_new_nstr("", 0));

    size_t end = len;
    while (end > 0 && is_space((unsigned char)buf[end - 1])) end--;

    size_t out_len = end;
    char *out = mm_alloc(out_len);

    memcpy(out, buf, out_len);

    Object *sobj = kl_new_nstr(out, out_len);
    mm_free(out);
    return obj_value(sobj);
}

// pub func startswith(prefix str) bool {}
static TValue _str_startswith(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);

    StringObject *s = SELF_AS(str_type);
    const char *buf = STR_BUF(s);
    size_t len = STR_LEN(s);

    StringObject *p = kl_arg_strobj(0);
    const char *pre = STR_BUF(p);
    size_t pre_len = STR_LEN(p);

    if (pre_len > len) return bool_value(false);

    if (memcmp(buf, pre, pre_len) == 0) return bool_value(true);

    return bool_value(false);
}

// pub func endswith(suffix str) bool {}
static TValue _str_endswith(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);

    StringObject *s = SELF_AS(str_type);
    const char *buf = STR_BUF(s);
    size_t len = STR_LEN(s);

    StringObject *p = kl_arg_strobj(0);
    const char *suf = STR_BUF(p);
    size_t suf_len = STR_LEN(p);

    if (suf_len > len) return bool_value(false);

    if (memcmp(buf + (len - suf_len), suf, suf_len) == 0) return bool_value(true);

    return bool_value(false);
}

// pub func removeprefix(prefix str) str {}
static TValue _str_removeprefix(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);

    StringObject *s = SELF_AS(str_type);
    const char *buf = STR_BUF(s);
    size_t len = STR_LEN(s);

    StringObject *p = kl_arg_strobj(0);
    const char *pre = STR_BUF(p);
    size_t pre_len = STR_LEN(p);

    if (pre_len == 0) return obj_value(s);

    if (pre_len > len) return obj_value(s);

    if (memcmp(buf, pre, pre_len) != 0) return obj_value(s);

    size_t out_len = len - pre_len;
    char *out = mm_alloc(out_len);

    memcpy(out, buf + pre_len, out_len);

    Object *sobj = kl_new_nstr(out, out_len);
    mm_free(out);
    return obj_value(sobj);
}

// pub func removesuffix(suffix str) str {}
static TValue _str_removesuffix(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);

    StringObject *s = SELF_AS(str_type);
    const char *buf = STR_BUF(s);
    size_t len = STR_LEN(s);

    StringObject *p = kl_arg_strobj(0);
    const char *suf = STR_BUF(p);
    size_t suf_len = STR_LEN(p);

    if (suf_len == 0) return obj_value(s);

    if (suf_len > len) return obj_value(s);

    if (memcmp(buf + (len - suf_len), suf, suf_len) != 0) return obj_value(s);

    size_t out_len = len - suf_len;
    char *out = mm_alloc(out_len);

    memcpy(out, buf, out_len);

    Object *sobj = kl_new_nstr(out, out_len);
    mm_free(out);
    return obj_value(sobj);
}

static TValue _str_replace(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);

    StringObject *s = SELF_AS(str_type);
    const char *buf = STR_BUF(s);
    size_t len = STR_LEN(s);

    StringObject *o = kl_arg_strobj(0);
    const char *old = STR_BUF(o);
    size_t old_len = STR_LEN(o);

    StringObject *n = kl_arg_strobj(1);
    const char *new = STR_BUF(n);
    size_t new_len = STR_LEN(n);

    if (old_len == 0) return obj_value(s);

    // Count occurrences of old
    size_t count = 0;
    for (size_t i = 0; i + old_len <= len;) {
        if (memcmp(buf + i, old, old_len) == 0) {
            count++;
            i += old_len;
        } else {
            i++;
        }
    }

    if (count == 0) return obj_value(s);

    size_t out_len = len + count * (new_len - old_len);
    char *out = mm_alloc(out_len);

    size_t oi = 0;
    for (size_t i = 0; i < len;) {
        if (i + old_len <= len && memcmp(buf + i, old, old_len) == 0) {
            memcpy(out + oi, new, new_len);
            oi += new_len;
            i += old_len;
        } else {
            out[oi++] = buf[i++];
        }
    }

    Object *sobj = kl_new_nstr(out, out_len);
    mm_free(out);
    return obj_value(sobj);
}

static TValue _str_zfill(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);

    StringObject *s = SELF_AS(str_type);
    const char *buf = STR_BUF(s);
    size_t len = STR_LEN(s);

    int64_t width = kl_arg_int64(0);

    if (width <= (int64_t)len) return obj_value(s);

    int64_t pad = width - (int64_t)len;

    // Check sign
    bool has_sign = false;
    char sign = 0;

    if (len > 0 && (buf[0] == '+' || buf[0] == '-')) {
        has_sign = true;
        sign = buf[0];
    }

    size_t out_len = (size_t)width;
    char *out = mm_alloc(out_len);

    size_t oi = 0;

    if (has_sign) {
        out[oi++] = sign;
        pad++; // because sign stays at front, pad increases by 1
    }

    for (int64_t i = 0; i < pad; i++) out[oi++] = '0';

    memcpy(out + oi, buf + (has_sign ? 1 : 0), len - (has_sign ? 1 : 0));

    Object *sobj = kl_new_nstr(out, out_len);
    mm_free(out);
    return obj_value(sobj);
}

static TValue _str_len(TValue *self, TValue *args, int nargs)
{
    StringObject *s = SELF_AS(str_type);
    return int64_value((int64_t)s->size);
}

static TValue _str_getitem(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    int64_t index = kl_arg_int64(0);
    StringObject *s = SELF_AS(str_type);
    if (index < 0) index += s->size;
    if (index < 0 || index >= (int64_t)s->size) panic("string index out of range");
    return kl_val_nstr(s->array + index, 1);
}

static TValue _str_getslice(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);

    StringObject *sobj = SELF_AS(str_type);
    SliceObject *slice = kl_arg_slice(0);

    int64_t start = to_int64(&slice->start);
    int64_t end = to_int64(&slice->end);
    int64_t step = to_int64(&slice->step);

    if (step == 0) {
        raise_exc_str("step cannot be zero");
        return error_value;
    }

    /* Normalize raw slice parameters into a valid [start, end) range,
     * handling None defaults, negative indices, and clamping.
     * Returns: >0 valid slice, ==0 empty slice, <0 invalid arguments. */
    int r = slice_adjust(&start, &end, step, STR_LEN(sobj));
    if (r < 0) {
        raise_exc_str("invalid slice");
        return error_value;
    }

    if (r == 0) {
        return kl_val_nstr("", 0);
    }

    /* Compute the number of elements in the result.
     * Regardless of step sign, the count is ceil(distance / abs_step).
     *   positive step: dist = end - start   (start < end)
     *   negative step: dist = start - end   (start > end)
     * Unified formula: len = (dist + abs_step - 1) / abs_step
     * The "+abs_step-1" term is essential for ceiling division via integer
     * truncation; without it, e.g. 11/2 yields 5 instead of 6, silently
     * dropping the element at index 0 when stepping backwards. */
    int64_t abs_step = step > 0 ? step : -step;
    int64_t dist = step > 0 ? (end - start) : (start - end);
    int64_t len = (dist + abs_step - 1) / abs_step;

    /* Guard against zero or negative length from upstream boundary errors,
     * preventing mm_alloc(0) or negative-size allocation UB. */
    if (len <= 0) {
        return kl_val_nstr("", 0);
    }

    /* Fast path: step==1 is the most common slice operation.
     * Return a zero-copy view directly into the source string buffer. */
    if (step == 1) {
        return kl_val_nstr(sobj->array + start, len);
    }

    /* General path: copy elements one by one into a new buffer.
     * A single loop handles both positive and negative steps because
     * "idx += step" naturally advances or retreats based on step sign,
     * eliminating the need for separate branches. */
    char *buf = mm_alloc(len);
    int64_t idx = start;
    for (int64_t i = 0; i < len; i++) {
        buf[i] = sobj->array[idx];
        idx += step;
    }
    TValue ret = kl_val_nstr(buf, len);
    mm_free(buf);
    return ret;
}

static TValue _str_contains(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);

    StringObject *s = SELF_AS(str_type);
    const char *buf = STR_BUF(s);
    size_t len = STR_LEN(s);

    Object *ob = kl_arg_obj(0);
    ASSERT(IS_STR(ob)); // compiler ensure the type, no need to panic
    StringObject *sub = (StringObject *)ob;
    const char *pat = STR_BUF(sub);
    size_t pat_len = STR_LEN(sub);

    if (pat_len == 0) return bool_value(true);
    if (pat_len > len) return bool_value(false);

    for (size_t i = 0; i + pat_len <= len; i++) {
        if (memcmp(buf + i, pat, pat_len) == 0) return bool_value(true);
    }

    return bool_value(false);
}

static MethodDef _str_methods[] = {
    { "__init__", _str_init },
    { "__len__", _str_len },
    { "empty", _str_empty },
    { "__getitem__", _str_getitem },
    { "__getslice__", _str_getslice },
    { "__contains__", _str_contains },
    { "index", _str_index },
    { "rindex", _str_rindex },
    { "count", _str_count },
    { "__eq__", _str_equal },
    { "__ne__", _str_ne },
    { "__lt__", _str_lt },
    { "__le__", _str_le },
    { "__gt__", _str_gt },
    { "__ge__", _str_ge },
    { "__hash__", _str_hash },
    { "__str__", _str_str },
    { "substr", _str_substr },
    { "to_int", _str_to_int },
    { "to_int_or", _str_to_int_or },
    { "to_float", _str_to_float },
    { "to_float_or", _str_to_float_or },
    { "format", _str_format },
    { "__add__", _str_add },
    { "__mul__", _str_mul },
    { "join", _str_join },
    { "to_bytes", _str_to_bytes },
    { "split", _str_split },
    { "rsplit", _str_rsplit },
    { "splitlines", _str_splitlines },
    { "upper", _str_upper },
    { "lower", _str_lower },
    { "capitalize", _str_capitalize },
    { "strip", _str_strip },
    { "lstrip", _str_lstrip },
    { "rstrip", _str_rstrip },
    { "startswith", _str_startswith },
    { "endswith", _str_endswith },
    { "removeprefix", _str_removeprefix },
    { "removesuffix", _str_removesuffix },
    { "replace", _str_replace },
    { "zfill", _str_zfill },
    { "ljust", _str_ljust },
    { "rjust", _str_rjust },
    { "center", _str_center },
    { "isdigit", _str_isdigit },
    { "isalpha", _str_isalpha },
    { "isalnum", _str_isalnum },
    { "isspace", _str_isspace },
    { "islower", _str_islower },
    { "isupper", _str_isupper },
    { NULL },
};

/* pub class str : Sequence & Comparable { ... } */
DEFINE_TYPE(str, TP_FLAGS_CLASS, sizeof(StringObject), _str_methods, NULL);

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
    INIT_OBJECT_HEAD(x, &str_type, 0);
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

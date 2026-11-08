/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "buffer.h"
#include "bytesobj.h"
#include "excobj.h"
#include "listobj.h"
#include "object.h"
#include "tupleobj.h"
#include "utf8.h"

#ifdef __cplusplus
extern "C" {
#endif

static size_t str_byte_offset(StringObject *s, int64_t ci);
static int64_t str_char_index(StringObject *s, size_t offset);
static int utf8_width(unsigned char lead);

static TValue _str_init(TValue *self, TValue *args, int nargs) { UNREACHABLE(); }

static TValue _str_fmt(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    Object *ob = SELF_AS(str_type);
    Formatter *fobj = kl_arg_obj_as(0, fmt_type);
    buf_write_nstr(&fobj->buf, STR_BUF(ob), STR_LEN(ob));
    return no_value;
}

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

/* Append one part of a join to `buf`, preceded by the separator unless it is
 * the first part. The element type is checked by the compiler (`Iterable[str]`
 * is the declared parameter type), so a non-`str` part is a type error at
 * compile time and only asserted here. */
static void _join_append(Buffer *buf, StringObject *sep, TValue *item, int64_t index)
{
    Object *str = to_obj(item);
    ASSERT(IS_STR(str));

    if (index > 0) {
        buf_write_nstr(buf, STR_BUF(sep), STR_LEN(sep));
    }

    buf_write_nstr(buf, STR_BUF(str), STR_LEN(str));
}

/* Append the parts of a `list` to `buf`, with `sep` between them. */
static int _join_list(Buffer *buf, StringObject *sep, Object *ob)
{
    /* Number of parts appended so far, which decides where the separators go. */
    int64_t index = 0;
    int len = kl_list_len(ob);

    for (int i = 0; i < len; i++) {
        TValue *v = kl_list_get(ob, i);
        _join_append(buf, sep, v, index++);
    }

    return 0;
}

/* Append the parts of a `tuple` to `buf`, with `sep` between them. */
static int _join_tuple(Buffer *buf, StringObject *sep, Object *ob)
{
    /* Number of parts appended so far, which decides where the separators go. */
    int64_t index = 0;
    int len = (int)TUPLE_SIZE(ob);

    for (int i = 0; i < len; i++) {
        TValue *v = TUPLE_ITEMS(ob) + i;
        _join_append(buf, sep, v, index++);
    }

    return 0;
}

/* Append a `Sequence[str]` to `buf`, read index by index through `len()` and
 * `[i]`, which avoids allocating an iterator for a random-access sequence.
 * Return 0 on success, or -1 with an exception raised if the sequence failed. */
static int _join_seq(Buffer *buf, StringObject *sep, TValue *seq)
{
    TValue count = kl_call_len(seq);
    if (is_error(&count)) return -1;
    int64_t len = to_int64(&count);

    /* Number of parts appended so far, which decides where the separators go. */
    int64_t index = 0;

    for (int64_t i = 0; i < len; i++) {
        TValue idx = int64_value(i);
        TValue part = kl_call_get_item(seq, &idx);
        if (is_error(&part)) return -1;
        _join_append(buf, sep, &part, index++);
    }

    return 0;
}

/* Append an `Iterable[str]` that is not a sequence to `buf`, drained through
 * `iter()` and `next()`, which yields nil at the end. Return 0 on success, or
 * -1 with an exception raised if the iterable failed. */
static int _join_iter(Buffer *buf, StringObject *sep, TValue *seq)
{
    TValue it = kl_call_iter(seq, 1);
    if (is_error(&it)) return -1;

    /* Number of parts appended so far, which decides where the separators go. */
    int64_t index = 0;

    for (;;) {
        TValue part = kl_call_next(&it);
        if (is_error(&part)) return -1;
        if (is_nil(&part)) break;
        _join_append(buf, sep, &part, index++);
    }

    return 0;
}

// pub func join(seq Iterable[str]) str
static TValue _str_join(TValue *self, TValue *args, int nargs)
{
    StringObject *sep = SELF_AS(str_type);

    ASSERT(nargs == 1);
    Object *seq = to_obj(args);

    /* The parts are copied once into a growing buffer and once more into the
     * fixed-size result, whose length is therefore known before it is built:
     * no pre-pass to sum the sizes. Joining nothing yields `""`.
     */
    BUF(buf);

    int ret = 0;

    if (IS_LIST(seq)) {
        /* Fast path: a `list` keeps its elements in a contiguous array. */
        ret = _join_list(&buf, sep, seq);
    } else if (IS_TUPLE(seq)) {
        ret = _join_tuple(&buf, sep, seq);
    } else if (kl_is_seq(seq)) {
        ret = _join_seq(&buf, sep, args);
    } else if (kl_is_iter(seq)) {
        ret = _join_iter(&buf, sep, args);
    } else {
        UNREACHABLE();
    }

    if (ret < 0) {
        FINI_BUF(buf);
        return error_value;
    }

    /* The buffer is copied into the result and released here, so an empty join
     * is the shared `""` and the caller owns nothing extra. */
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
    int nchars = (int)STR_NCHARS(s);

    const char *pat = STR_BUF(sub);
    size_t pat_size = STR_LEN(sub);

    // normalize end
    if (end < 0 || end > nchars) end = nchars;

    // clamp start
    if (start < 0) start = 0;
    if (start > end) start = end;

    // empty pattern → always match at start
    if (pat_size == 0) {
        return int64_value(start);
    }

    // search the bytes of characters [start, end): a valid UTF-8 pattern only matches at the
    // start of a character
    size_t from = str_byte_offset(s, start);
    size_t to = str_byte_offset(s, end);
    const char *pos = memmem(src + from, to - from, pat, pat_size);

    if (!pos) return int64_value(-1);

    // the character index of the match
    return int64_value(str_char_index(s, (size_t)(pos - src)));
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
    int nchars = (int)STR_NCHARS(s);

    const char *pat = STR_BUF(sub);
    size_t pat_size = STR_LEN(sub);

    // normalize end
    if (end < 0 || end > nchars) end = nchars;

    // clamp start
    if (start < 0) start = 0;
    if (start > end) start = end;

    // empty pattern → match at end (Python semantics)
    if (pat_size == 0) return int64_value(end);

    // the bytes of characters [start, end)
    int64_t from = (int64_t)str_byte_offset(s, start);
    int64_t to = (int64_t)str_byte_offset(s, end);

    // rindex: scan from right to left
    // last possible match starts at: to - pat_size
    for (int64_t i = to - (int64_t)pat_size; i >= from; i--) {
        if (memcmp(src + i, pat, pat_size) == 0) {
            return int64_value(str_char_index(s, (size_t)i));
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
    int64_t nchars = (int64_t)STR_NCHARS(s);

    const char *pat = STR_BUF(sub);
    size_t pat_size = STR_LEN(sub);

    // normalize end
    if (end < 0 || end > nchars) end = nchars;

    // clamp start
    if (start < 0) start = 0;
    if (start > end) start = end;

    // empty pattern → Python semantics: count of "" is (end - start + 1)
    if (pat_size == 0) {
        return int64_value((end - start) + 1);
    }

    int count = 0;

    // scan the bytes of characters [start, end) from left to right
    size_t i = str_byte_offset(s, start);
    size_t to = str_byte_offset(s, end);
    while (i + pat_size <= to) {
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
    int nchars = (int)STR_NCHARS(s);

    // normalize end
    if (end < 0 || end > nchars) end = nchars;

    // clamp start
    if (start < 0) start = 0;
    if (start > end) start = end;

    // create new string (copy the bytes of the characters)
    size_t from = str_byte_offset(s, start);
    size_t to = str_byte_offset(s, end);
    TValue val = kl_val_nstr(src + from, to - from);
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

    if (!ok) {
        raise_exc_str("invalid integer literal");
        return error_value;
    }

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

    if (!ok) {
        raise_exc_str("invalid float literal");
        return error_value;
    }

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

/* The fill character of center / ljust / rjust: its bytes in *fill and *fill_len. False when it
   is not one character. */
static bool pad_fill(TValue *args, int nargs, const char **fill, size_t *fill_len)
{
    *fill = " ";
    *fill_len = 1;
    if (nargs < 2) return true;
    StringObject *fs = kl_arg_strobj(1);
    if (STR_NCHARS(fs) != 1) return false;
    *fill = STR_BUF(fs);
    *fill_len = STR_LEN(fs);
    return true;
}

/* `s` with `left` fill characters before it and `right` after it. */
static Object *pad_str(StringObject *s, const char *fill, size_t fill_len, int64_t left,
                       int64_t right)
{
    size_t out_len = s->size + (size_t)(left + right) * fill_len;
    char *out = mm_alloc(out_len);
    size_t n = 0;
    for (int64_t i = 0; i < left; i++) {
        memcpy(out + n, fill, fill_len);
        n += fill_len;
    }
    memcpy(out + n, s->array, s->size);
    n += s->size;
    for (int64_t i = 0; i < right; i++) {
        memcpy(out + n, fill, fill_len);
        n += fill_len;
    }
    Object *sobj = kl_new_nstr(out, out_len);
    mm_free(out);
    return sobj;
}

static TValue _str_center(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs >= 1 && nargs <= 2);

    StringObject *s = SELF_AS(str_type);
    int64_t width = kl_arg_int64(0);
    const char *fill;
    size_t fill_len;
    if (!pad_fill(args, nargs, &fill, &fill_len)) {
        raise_exc_str("center(): fill must be a single character");
        return error_value;
    }

    // width <= the number of characters → return original
    int64_t len = (int64_t)s->nchars;
    if (width <= len) return obj_value(s);

    int64_t total = width - len;
    int64_t left = total / 2;
    int64_t right = total - left;
    return obj_value(pad_str(s, fill, fill_len, left, right));
}

static TValue _str_ljust(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs >= 1 && nargs <= 2);

    StringObject *s = SELF_AS(str_type);
    int64_t width = kl_arg_int64(0);
    const char *fill;
    size_t fill_len;
    if (!pad_fill(args, nargs, &fill, &fill_len)) {
        raise_exc_str("ljust(): fill must be a single character");
        return error_value;
    }

    int64_t len = (int64_t)s->nchars;
    if (width <= len) return obj_value(s);
    return obj_value(pad_str(s, fill, fill_len, 0, width - len));
}

static TValue _str_rjust(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs >= 1 && nargs <= 2);

    StringObject *s = SELF_AS(str_type);
    int64_t width = kl_arg_int64(0);
    const char *fill;
    size_t fill_len;
    if (!pad_fill(args, nargs, &fill, &fill_len)) {
        raise_exc_str("rjust(): fill must be a single character");
        return error_value;
    }

    int64_t len = (int64_t)s->nchars;
    if (width <= len) return obj_value(s);
    return obj_value(pad_str(s, fill, fill_len, width - len, 0));
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

    // the width counts characters; the zeros are one byte each
    int64_t width = kl_arg_int64(0);

    if (width <= (int64_t)s->nchars) return obj_value(s);

    int64_t pad = width - (int64_t)s->nchars;

    // Check sign
    bool has_sign = false;
    char sign = 0;

    if (len > 0 && (buf[0] == '+' || buf[0] == '-')) {
        has_sign = true;
        sign = buf[0];
    }

    size_t out_len = len + (size_t)pad;
    char *out = mm_alloc(out_len);

    size_t oi = 0;

    // the sign stays first, the zeros go after it
    if (has_sign) out[oi++] = sign;

    for (int64_t i = 0; i < pad; i++) out[oi++] = '0';

    /* The sign character has already been copied above, skip it in the payload. */
    size_t skip = has_sign ? 1 : 0;
    memcpy(out + oi, buf + skip, len - skip);

    Object *sobj = kl_new_nstr(out, out_len);
    mm_free(out);
    return obj_value(sobj);
}

/* The number of characters. */
static TValue _str_len(TValue *self, TValue *args, int nargs)
{
    StringObject *s = SELF_AS(str_type);
    return int64_value((int64_t)s->nchars);
}

static TValue _str_getitem(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    // FIXME:
    // int64_t index = kl_arg_int64(0);
    int64_t index = kl_arg_int(0);
    StringObject *s = SELF_AS(str_type);
    if (index < 0) index += (int64_t)s->nchars;
    if (index < 0 || index >= (int64_t)s->nchars) {
        raise_exc_str("str index out of range");
        return error_value;
    }
    // the character at `index`
    size_t off = str_byte_offset(s, index);
    int width = utf8_width((unsigned char)s->array[off]);
    return kl_val_nstr(s->array + off, (size_t)width);
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
    int r = slice_adjust(&start, &end, step, STR_NCHARS(sobj));
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
        size_t from = str_byte_offset(sobj, start);
        size_t to = str_byte_offset(sobj, start + len);
        return kl_val_nstr(sobj->array + from, to - from);
    }

    /* General path: copy the characters one by one into a new buffer.
     * A single loop handles both positive and negative steps because
     * "idx += step" naturally advances or retreats based on step sign,
     * eliminating the need for separate branches. */
    char *buf = mm_alloc((size_t)len * 4);
    size_t n = 0;
    int64_t idx = start;
    for (int64_t i = 0; i < len; i++) {
        size_t off = str_byte_offset(sobj, idx);
        int width = utf8_width((unsigned char)sobj->array[off]);
        memcpy(buf + n, sobj->array + off, (size_t)width);
        n += (size_t)width;
        idx += step;
    }
    TValue ret = kl_val_nstr(buf, n);
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

static TValue _str_view(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    int64_t start = kl_arg_int64(0);
    int64_t end = kl_arg_int64(1);
    StringObject *sobj = SELF_AS(str_type);

    /* end == -1 is the documented "to the end" convention; map it to the
     * slice sentinel so slice_adjust treats it as unspecified instead of the
     * relative index len - 1 (which silently dropped the last character). */
    if (end == -1) end = INT64_MIN;

    TValue items[3] = { int64_value(start), int64_value(end), int64_value(1) };
    Object *slice = kl_new_slice(items);
    TValue _args[] = { obj_value(slice) };
    return _str_getslice(self, _args, 1);
}

static MethodDef _str_methods[] = {
    { "__init__", _str_init },
    { "len", _str_len },
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
    { "hash", _str_hash },
    { "fmt", _str_fmt },
    { "substr", _str_substr },
    { "to_int", _str_to_int },
    { "to_int_or", _str_to_int_or },
    { "to_float", _str_to_float },
    { "to_float_or", _str_to_float_or },
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
    { "starts_with", _str_startswith },
    { "ends_with", _str_endswith },
    { "remove_prefix", _str_removeprefix },
    { "remove_suffix", _str_removesuffix },
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
    { "view", _str_view },
    { NULL },
};

/* pub class str : Sequence & Comparable { ... } */
TypeObject str_type = {
    ._type = &type_type,
    .name = "str",
    .flags = TP_FLAGS_CLASS,
    .objsize = sizeof(StringObject),
    .methdefs = _str_methods,
};

static StringObject empty_str = {
    ._type = &str_type,
    .size = 0,
    .nchars = 0,
    .ascii = 1,
    .array = "",
};

/* The UTF-8 encoding of U+FFFD, which takes the place of an invalid byte. */
static const char REPLACEMENT[] = "\xEF\xBF\xBD";

/* Count the characters of s[0:len]. Sets *ascii when they are all ASCII and *valid when s is
   valid UTF-8; an invalid byte counts as one character, the U+FFFD it becomes. */
static size_t count_chars(const char *s, size_t len, int *ascii, int *valid)
{
    size_t i = 0;
    size_t nchars = 0;
    *ascii = 1;
    *valid = 1;
    while (i < len) {
        if ((unsigned char)s[i] < 0x80) {
            i++;
            nchars++;
            continue;
        }
        *ascii = 0;
        uint32_t rune;
        int w = utf8_decode(s + i, len - i, &rune);
        if (w == 1 && rune == UTF8_RUNE_ERROR) *valid = 0;
        i += (size_t)w;
        nchars++;
    }
    return nchars;
}

/* s[0:len] with each invalid byte replaced by U+FFFD, in a new buffer of *out_len bytes. */
static char *repair_utf8(const char *s, size_t len, size_t *out_len)
{
    char *out = mm_alloc(len * 3 + 1);
    size_t n = 0;
    size_t i = 0;
    while (i < len) {
        uint32_t rune;
        int w = utf8_decode(s + i, len - i, &rune);
        if (w == 1 && rune == UTF8_RUNE_ERROR) {
            memcpy(out + n, REPLACEMENT, 3);
            n += 3;
        } else {
            memcpy(out + n, s + i, (size_t)w);
            n += (size_t)w;
        }
        i += (size_t)w;
    }
    out[n] = '\0';
    *out_len = n;
    return out;
}

/* Characters per entry of StringObject.offsets. */
#define OFFSET_STEP 64

/* The bytes of the character that starts with `lead`, in valid UTF-8. */
static int utf8_width(unsigned char lead)
{
    if (lead < 0x80) return 1;
    if (lead < 0xE0) return 2;
    if (lead < 0xF0) return 3;
    return 4;
}

/* Byte offset `from`, moved forward over `count` characters. */
static size_t skip_chars(const char *buf, size_t from, size_t count)
{
    size_t off = from;
    for (size_t i = 0; i < count; i++) off += (size_t)utf8_width((unsigned char)buf[off]);
    return off;
}

/* Build s->offsets: the byte offset of characters 0, 64, 128, ... */
static void build_offsets(StringObject *s)
{
    size_t entries = s->nchars / OFFSET_STEP + 1;
    s->offsets = mm_alloc(entries * sizeof(uint32_t));
    size_t off = 0;
    for (size_t k = 0; k < entries; k++) {
        s->offsets[k] = (uint32_t)off;
        if (k + 1 < entries) off = skip_chars(s->array, off, OFFSET_STEP);
    }
}

/* The byte offset of character `ci`, clamped to 0..nchars (nchars gives the end). */
static size_t str_byte_offset(StringObject *s, int64_t ci)
{
    if (ci <= 0) return 0;
    if ((size_t)ci >= s->nchars) return s->size;
    if (s->ascii) return (size_t)ci;

    size_t target = (size_t)ci;
    size_t off;
    if (target >= s->cached_char && target - s->cached_char < OFFSET_STEP) {
        // a little after the last lookup: a scan going forward
        off = skip_chars(s->array, s->cached_byte, target - s->cached_char);
    } else if (target < s->cached_char && s->cached_char - target < OFFSET_STEP) {
        // a little before it: a scan going backward, over the continuation bytes
        off = s->cached_byte;
        for (size_t i = target; i < s->cached_char; i++) {
            do {
                off--;
            } while (((unsigned char)s->array[off] & 0xC0) == 0x80);
        }
    } else {
        if (!s->offsets) build_offsets(s);
        size_t k = target / OFFSET_STEP;
        off = skip_chars(s->array, s->offsets[k], target - k * OFFSET_STEP);
    }
    s->cached_char = target;
    s->cached_byte = off;
    return off;
}

/* The character index of byte offset `offset`, which starts a character. */
static int64_t str_char_index(StringObject *s, size_t offset)
{
    if (s->ascii) return (int64_t)offset;
    if (offset >= s->size) return (int64_t)s->nchars;

    size_t ci = 0;
    size_t off = 0;
    if (offset >= s->cached_byte) {
        ci = s->cached_char;
        off = s->cached_byte;
    } else if (s->nchars >= OFFSET_STEP) {
        if (!s->offsets) build_offsets(s);
        // the last entry at or before `offset`
        size_t lo = 0;
        size_t hi = s->nchars / OFFSET_STEP;
        while (lo < hi) {
            size_t mid = (lo + hi + 1) / 2;
            if (s->offsets[mid] <= offset) {
                lo = mid;
            } else {
                hi = mid - 1;
            }
        }
        ci = lo * OFFSET_STEP;
        off = s->offsets[lo];
    }
    while (off < offset) {
        off += (size_t)utf8_width((unsigned char)s->array[off]);
        ci++;
    }
    return (int64_t)ci;
}

size_t kl_str_byte_offset(Object *s, int64_t ci)
{
    ASSERT(IS_STR(s));
    return str_byte_offset((StringObject *)s, ci);
}

int64_t kl_str_char_index(Object *s, size_t offset)
{
    ASSERT(IS_STR(s));
    return str_char_index((StringObject *)s, offset);
}

Object *kl_new_nstr(char *s, size_t len)
{
    if (len == 0) {
        return (Object *)&empty_str;
    }

    StringObject *x = mm_alloc_obj(x);
    INIT_OBJECT_HEAD(x, &str_type, 0);

    int ascii;
    int valid;
    x->nchars = count_chars(s, len, &ascii, &valid);
    x->ascii = ascii;

    if (valid) {
        char *data = mm_alloc(len + 1);
        memcpy(data, s, len);
        data[len] = '\0';
        x->array = data;
        x->size = len;
    } else {
        x->array = repair_utf8(s, len, &x->size);
    }

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

    // the shared empty string singleton is static, never free it
    if (sobj == &empty_str) {
        return;
    }

    if (sobj->size > 0) {
        mm_free(sobj->array);
    }
    if (sobj->offsets) mm_free(sobj->offsets);
    mm_free(sobj);
}

#ifdef __cplusplus
}
#endif

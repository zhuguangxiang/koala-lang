/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2023 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "atom.h"
#include "symbol.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _TypeIntern {
    TypeSpec type;
    HashMapEntry hnode;
} TypeIntern;

TypeSpec *no_type_spec(void)
{
    TypeSpec *ts = mm_alloc_obj(ts);
    ts->kind = TYPE_NO_TYPE;
    ts->sym_id = -1;
    return ts;
}

TypeSpec *int_type_spec(int width, int sign)
{
    TypeSpec *ts = mm_alloc_obj(ts);
    ts->kind = TYPE_INT;
    ts->int_flt_info.width = width;
    ts->int_flt_info.sign = sign;
    ts->sym_id = -1;
    return ts;
}

TypeSpec *bool_type_spec(void)
{
    TypeSpec *ts = mm_alloc_obj(ts);
    ts->kind = TYPE_BOOL;
    ts->sym_id = -1;
    return ts;
}

TypeSpec *str_type_spec(void)
{
    TypeSpec *ts = mm_alloc_obj(ts);
    ts->kind = TYPE_STR;
    ts->sym_id = -1;
    return ts;
}

TypeSpec *object_type_spec(void)
{
    TypeSpec *ts = mm_alloc_obj(ts);
    ts->kind = TYPE_OBJECT;
    ts->sym_id = -1;
    return ts;
}

TypeSpec *va_list_type_spec(void)
{
    TypeSpec *ts = mm_alloc_obj(ts);
    ts->kind = TYPE_VA_LIST;
    ts->sym_id = -1;
    return ts;
}

TypeSpec *generic_var_type_spec(char *name, int index, int sym_id)
{
    TypeSpec *ts = mm_alloc_obj(ts);
    ts->kind = TYPE_GENERIC_VAR;
    ts->generic_var.name = name;
    ts->generic_var.index = index;
    ts->sym_id = sym_id;
    return ts;
}

TypeSpec *specialized_type_spec(char *full_pkg, char *name, Vector *args, int sym_id)
{
    TypeSpec *ts = mm_alloc_obj(ts);
    ts->kind = TYPE_SPECIALIZED;
    ts->specialized.pkg = full_pkg;
    ts->specialized.name = name;
    ts->specialized.args = args;
    ts->sym_id = sym_id;
    return ts;
}

TypeSpec *unresolved_type_spec(TypeIdent *pkg, TypeIdent name, Vector *args)
{
    TypeSpec *ts = mm_alloc_obj(ts);
    ts->kind = TYPE_UNRESOLVED;
    if (pkg) {
        ts->unresolved.pkg = *pkg;
    }
    ts->unresolved.name = name;
    ts->unresolved.args = args;
    ts->sym_id = -1;
    return ts;
}

int type_spec_to_str(TypeSpec *ts, Buffer *buf)
{
    if (!ts) return 0;

    if (ts->kind == TYPE_INT) {
        if (ts->int_flt_info.width == 1) {
            char ch = ts->int_flt_info.sign ? 'c' : 'C';
            buf_write_char(buf, ch);
        } else if (ts->int_flt_info.width == 2) {
            char ch = ts->int_flt_info.sign ? 's' : 'S';
            buf_write_char(buf, ch);
        } else if (ts->int_flt_info.width == 4) {
            char ch = ts->int_flt_info.sign ? 'i' : 'I';
            buf_write_char(buf, ch);
        } else {
            char ch = ts->int_flt_info.sign ? 'j' : 'J';
            buf_write_char(buf, ch);
        }
    } else if (ts->kind == TYPE_STR) {
        buf_write_char(buf, 'u');
    } else if (ts->kind == TYPE_VA_LIST) {
        buf_write_str(buf, "...");
    } else if (ts->kind == TYPE_NO_TYPE) {
        // do-nothing
    } else if (ts->kind == TYPE_BOOL) {
        buf_write_char(buf, 'z');
    } else if (ts->kind == TYPE_OBJECT) {
        buf_write_char(buf, 'o');
    } else if (ts->kind == TYPE_GENERIC_VAR) {
        buf_write_str(buf, ts->generic_var.name);
    } else if (ts->kind == TYPE_SPECIALIZED) {
        buf_write_char(buf, 'L');
        buf_write_str(buf, ts->specialized.name);
        if (vector_size(ts->specialized.args) > 0) {
            buf_write_char(buf, '<');
            TypeSpec *_ts;
            vector_foreach_object(_ts, ts->specialized.args)
            {
                type_spec_to_str(_ts, buf);
            }
            buf_write_char(buf, '>');
        }
        buf_write_char(buf, ';');
    } else {
        UNREACHABLE();
    }

    return 0;
}

static void __add_arg(TypeSpec *ts, TypeSpec *arg)
{
    Vector *args = ts->unresolved.args;
    if (args == NULL) {
        args = vector_create_ptr();
        ts->unresolved.args = args;
    }
    vector_push_back(args, arg);
}

static TypeSpec *__to_unresolved_type(char *s, int len)
{
    char *dot = strchr(s, '.');
    ASSERT(dot != NULL);
    char *path = atom_nstr(s, dot - s);
    char *type = atom_nstr(dot + 1, len - (dot - s) - 1);
    TypeIdent _pkg = { .name = path };
    TypeIdent _name = { .name = type };
    return unresolved_type_spec(&_pkg, _name, NULL);
}

static TypeSpec *__to_typespec(char **str)
{
    char *s = *str;

    if (!s || s[0] == 0) return NULL;

    char ch = *s;
    char *k;
    TypeSpec *ts;
    TypeSpec *arg;

    switch (ch) {
        case 'L': {
            s++;
            k = s;
            while (*s != ';' && *s != '<' && *s != '\0') s++;
            ts = __to_unresolved_type(k, s - k);
            if (*s == '<') {
                s++;
                while (*s != '>' && *s != '\0') {
                    arg = __to_typespec(&s);
                    if (arg) __add_arg(ts, arg);
                }
                if (*s == '>') s++;
            }
            if (*s == ';') s++;
            break;
        }
        case 'c': {
            ts = int_type_spec(1, 1);
            s++;
            break;
        }
        case 'C': {
            ts = int_type_spec(1, 0);
            s++;
            break;
        }
        case 's': {
            ts = int_type_spec(2, 1);
            s++;
            break;
        }
        case 'S': {
            ts = int_type_spec(2, 0);
            s++;
            break;
        }
        case 'i': {
            ts = int_type_spec(4, 1);
            s++;
            break;
        }
        case 'I': {
            ts = int_type_spec(4, 0);
            s++;
            break;
        }
        case 'j': {
            ts = int_type_spec(8, 1);
            s++;
            break;
        }
        case 'J': {
            ts = int_type_spec(8, 0);
            s++;
            break;
        }
        case 'u': {
            ts = str_type_spec();
            s++;
            break;
        }
        case 'z': {
            ts = bool_type_spec();
            s++;
            break;
        }
        case 'o': {
            ts = object_type_spec();
            s++;
            break;
        }
        case '.': {
            if (!strncmp(s, "...", 3)) {
                ts = va_list_type_spec();
                s += 3;
            }
            break;
        }
        default: {
            UNREACHABLE();
            break;
        }
    }

    *str = s;
    return ts;
}

TypeSpec *type_spec_from_str(const char *s) { return __to_typespec((char **)&s); }

void type_spec_print(TypeSpec *ts, Buffer *buf)
{
    if (!ts) return;

    if (ts->kind == TYPE_NO_TYPE) {
        buf_write_str(buf, "<NO-TYPE>");
    } else if (ts->kind == TYPE_INT) {
        if (!ts->int_flt_info.sign) {
            buf_write_char(buf, 'u');
        }
        buf_write_str(buf, "int");
        buf_write_int64(buf, ts->int_flt_info.width * 8);
    } else if (ts->kind == TYPE_STR) {
        buf_write_str(buf, "str");
    } else if (ts->kind == TYPE_VA_LIST) {
        buf_write_str(buf, "...");
    } else if (ts->kind == TYPE_BOOL) {
        buf_write_str(buf, "bool");
    } else if (ts->kind == TYPE_OBJECT) {
        buf_write_str(buf, "object");
    } else if (ts->kind == TYPE_UNRESOLVED) {
        buf_write_str(buf, ts->unresolved.name.name);
    } else if (ts->kind == TYPE_GENERIC_VAR) {
        buf_write_str(buf, ts->generic_var.name);
    } else if (ts->kind == TYPE_SPECIALIZED) {
        buf_write_str(buf, ts->specialized.name);
    } else {
        UNREACHABLE();
    }
}

static void __typespec_str_print(char **str, Buffer *buf)
{
    char *s = *str;

    if (!s || s[0] == 0) {
        buf_write_str(buf, "unk");
        return;
    }

    char ch = *s;
    char *k;
    TypeSpec *ts;
    TypeSpec *arg;

    switch (ch) {
        case 'L': {
            s++;
            k = s;
            while (*s != ';' && *s != '<' && *s != '\0') s++;
            buf_write_nstr(buf, k, s - k);
            if (*s == '<') {
                buf_write_char(buf, '[');
                s++;
                int i = 0;
                while (*s != '>' && *s != '\0') {
                    if (i != 0) buf_write_str(buf, ", ");
                    __typespec_str_print(&s, buf);
                    i++;
                }
                if (*s == '>') {
                    buf_write_char(buf, ']');
                    s++;
                }
            }
            if (*s == ';') s++;
            break;
        }
        case 'c': {
            buf_write_str(buf, "int8");
            s++;
            break;
        }
        case 'C': {
            buf_write_str(buf, "uint8");
            s++;
            break;
        }
        case 's': {
            buf_write_str(buf, "int16");
            s++;
            break;
        }
        case 'S': {
            buf_write_str(buf, "uint16");
            s++;
            break;
        }
        case 'i': {
            buf_write_str(buf, "int32");
            s++;
            break;
        }
        case 'I': {
            buf_write_str(buf, "uint32");
            s++;
            break;
        }
        case 'j': {
            buf_write_str(buf, "int64");
            s++;
            break;
        }
        case 'J': {
            buf_write_str(buf, "uint64");
            s++;
            break;
        }
        case 'u': {
            buf_write_str(buf, "str");
            s++;
            break;
        }
        case 'z': {
            buf_write_str(buf, "bool");
            s++;
            break;
        }
        case 'o': {
            buf_write_str(buf, "object");
            s++;
            break;
        }
        case '.': {
            if (!strncmp(s, "...", 3)) {
                buf_write_str(buf, "...");
                s += 3;
            }
            break;
        }
        default: {
            buf_write_char(buf, ch);
            s++;
            break;
        }
    }

    *str = s;
}

void type_spec_str_print(char *s, Buffer *buf) { __typespec_str_print(&s, buf); }

#ifdef __cplusplus
}
#endif

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

static HashMap type_map;
static Vector type_list;

static int type_spec_equal(TypeSpec *ts1, TypeSpec *ts2)
{
    if (ts1 == ts2) return 1;
    if (!ts1 || !ts2) return 0;
    if (ts1->kind != ts2->kind) return 0;
    int eq = (ts1->signature == ts2->signature);
    if (eq) return 1;
    eq = !strcmp(ts1->signature, ts2->signature);
    return eq;
}

static unsigned int type_spec_hash(TypeSpec *ts) { return str_hash(ts->signature); }

void type_spec_free(TypeSpec *ts)
{
    if (!ts) return;

    if (ts->type_id >= 0) {
        // interned type spec, do not free
        return;
    }

    // free args
    if (ts->kind == TYPE_SPECIALIZED) {
        Vector *args = ts->specialized.args;
        if (args) {
            TypeSpec *arg;
            vector_foreach_object(arg, args) { type_spec_free(arg); }
            vector_destroy(args);
            ts->specialized.args = NULL;
        }
    } else if (ts->kind == TYPE_UNRESOLVED) {
        Vector *args = ts->unresolved.args;
        if (args) {
            TypeSpec *arg;
            vector_foreach_object(arg, args) { type_spec_free(arg); }
            vector_destroy(args);
            ts->unresolved.args = NULL;
        }
    }

    mm_free(ts);
}

TypeSpec *type_spec_get_by_id(int type_id)
{
    return vector_get_object(&type_list, type_id);
}

static TypeSpec *_no_type_spec(void)
{
    TypeSpec *ts = mm_alloc_obj(ts);
    ts->kind = TYPE_NO_TYPE;
    ts->signature = atom_str("");
    ts->sym_id = -1;
    hashmap_entry_init(&ts->hnode, type_spec_hash(ts));
    return ts;
}

static TypeSpec *_int_type_spec(int width, int sign)
{
    TypeSpec *ts = mm_alloc_obj(ts);
    ts->kind = TYPE_INT;
    ts->int_flt_info.width = width;
    ts->int_flt_info.sign = sign;
    ts->sym_id = -1;
    if (width == 1) {
        ts->signature = sign ? atom_str("c") : atom_str("C");
    } else if (width == 2) {
        ts->signature = sign ? atom_str("s") : atom_str("S");
    } else if (width == 4) {
        ts->signature = sign ? atom_str("i") : atom_str("I");
    } else {
        ts->signature = sign ? atom_str("j") : atom_str("J");
    }
    hashmap_entry_init(&ts->hnode, type_spec_hash(ts));
    return ts;
}

static TypeSpec *_float_type_spec(int width)
{
    TypeSpec *ts = mm_alloc_obj(ts);
    ts->kind = TYPE_FLOAT;
    ts->int_flt_info.width = width;
    ts->sym_id = -1;
    if (width == 2) {
        ts->signature = atom_str("h");
    } else if (width == 4) {
        ts->signature = atom_str("f");
    } else {
        ts->signature = atom_str("d");
    }
    hashmap_entry_init(&ts->hnode, type_spec_hash(ts));
    return ts;
}

static TypeSpec *_bfloat16_type_spec(void)
{
    TypeSpec *ts = mm_alloc_obj(ts);
    ts->kind = TYPE_BFLOAT16;
    ts->sym_id = -1;
    ts->signature = atom_str("b");
    hashmap_entry_init(&ts->hnode, type_spec_hash(ts));
    return ts;
}

static TypeSpec *_bool_type_spec(void)
{
    TypeSpec *ts = mm_alloc_obj(ts);
    ts->kind = TYPE_BOOL;
    ts->signature = atom_str("z");
    ts->sym_id = -1;
    hashmap_entry_init(&ts->hnode, type_spec_hash(ts));
    return ts;
}

static TypeSpec *_str_type_spec(void)
{
    TypeSpec *ts = mm_alloc_obj(ts);
    ts->kind = TYPE_STR;
    ts->signature = atom_str("u");
    ts->sym_id = -1;
    hashmap_entry_init(&ts->hnode, type_spec_hash(ts));
    return ts;
}

static TypeSpec *_object_type_spec(void)
{
    TypeSpec *ts = mm_alloc_obj(ts);
    ts->kind = TYPE_OBJECT;
    ts->signature = atom_str("o");
    ts->sym_id = -1;
    hashmap_entry_init(&ts->hnode, type_spec_hash(ts));
    return ts;
}

static TypeSpec *_va_list_type_spec(void)
{
    TypeSpec *ts = mm_alloc_obj(ts);
    ts->kind = TYPE_VA_LIST;
    ts->signature = atom_str("...");
    ts->sym_id = -1;
    hashmap_entry_init(&ts->hnode, type_spec_hash(ts));
    return ts;
}

static TypeSpec *_type_type_spec(void)
{
    TypeSpec *ts = mm_alloc_obj(ts);
    ts->kind = TYPE_TYPE;
    ts->signature = atom_str("Lbuiltin.type;");
    ts->sym_id = -1;
    hashmap_entry_init(&ts->hnode, type_spec_hash(ts));
    return ts;
}

static TypeSpec *_range_type_spec(void)
{
    TypeSpec *ts = mm_alloc_obj(ts);
    ts->kind = TYPE_RANGE;
    ts->signature = atom_str("Lbuiltin.range;");
    ts->sym_id = -1;
    hashmap_entry_init(&ts->hnode, type_spec_hash(ts));
    return ts;
}

void update_builtin_type_specs(HashMap *stbl)
{
    // Update builtin type specs with the provided symbol table
    TypeSpec *ts;
    Symbol *sym;

    ts = int8_type_spec();
    sym = stbl_get(stbl, "int8");
    if (sym) ts->sym_id = sym->id;

    ts = uint8_type_spec();
    sym = stbl_get(stbl, "uint8");
    if (sym) ts->sym_id = sym->id;

    ts = int16_type_spec();
    sym = stbl_get(stbl, "int16");
    if (sym) ts->sym_id = sym->id;

    ts = uint16_type_spec();
    sym = stbl_get(stbl, "uint16");
    if (sym) ts->sym_id = sym->id;

    ts = int32_type_spec();
    sym = stbl_get(stbl, "int32");
    if (sym) ts->sym_id = sym->id;

    ts = uint32_type_spec();
    sym = stbl_get(stbl, "uint32");
    if (sym) ts->sym_id = sym->id;

    ts = int64_type_spec();
    sym = stbl_get(stbl, "int64");
    if (sym) ts->sym_id = sym->id;

    ts = uint64_type_spec();
    sym = stbl_get(stbl, "uint64");
    if (sym) ts->sym_id = sym->id;
}

void typespec_init(void)
{
    hashmap_init(&type_map, (HashMapEqualFunc)type_spec_equal);
    vector_init_ptr(&type_list);
    /*
    0: TYPE_NO_TYPE
    1: TYPE_INT
    2: TYPE_BOOL
    3: TYPE_STR
    4: TYPE_OBJECT
    5: TYPE_VA_LIST
    6: TYPE_FLOAT
    7: TYPE_BFLOAT16
    */
    TypeSpec *ts;

    ts = _no_type_spec();
    ts->type_id = 0;
    vector_push_back(&type_list, &ts);
    hashmap_put(&type_map, ts);

    ts = _int_type_spec(1, 1);
    ts->type_id = 1;
    vector_push_back(&type_list, &ts);
    hashmap_put(&type_map, ts);

    ts = _int_type_spec(1, 0);
    ts->type_id = 2;
    vector_push_back(&type_list, &ts);
    hashmap_put(&type_map, ts);

    ts = _int_type_spec(2, 1);
    ts->type_id = 3;
    vector_push_back(&type_list, &ts);
    hashmap_put(&type_map, ts);

    ts = _int_type_spec(2, 0);
    ts->type_id = 4;
    vector_push_back(&type_list, &ts);
    hashmap_put(&type_map, ts);

    ts = _int_type_spec(4, 1);
    ts->type_id = 5;
    vector_push_back(&type_list, &ts);
    hashmap_put(&type_map, ts);

    ts = _int_type_spec(4, 0);
    ts->type_id = 6;
    vector_push_back(&type_list, &ts);
    hashmap_put(&type_map, ts);

    ts = _int_type_spec(8, 1);
    ts->type_id = 7;
    vector_push_back(&type_list, &ts);
    hashmap_put(&type_map, ts);

    ts = _int_type_spec(8, 0);
    ts->type_id = 8;
    vector_push_back(&type_list, &ts);
    hashmap_put(&type_map, ts);

    ts = _bool_type_spec();
    ts->type_id = 9;
    vector_push_back(&type_list, &ts);
    hashmap_put(&type_map, ts);

    ts = _str_type_spec();
    ts->type_id = 10;
    vector_push_back(&type_list, &ts);
    hashmap_put(&type_map, ts);

    ts = _object_type_spec();
    ts->type_id = 11;
    vector_push_back(&type_list, &ts);
    hashmap_put(&type_map, ts);

    ts = _va_list_type_spec();
    ts->type_id = 12;
    vector_push_back(&type_list, &ts);
    hashmap_put(&type_map, ts);

    ts = _float_type_spec(2);
    ts->type_id = 13;
    vector_push_back(&type_list, &ts);
    hashmap_put(&type_map, ts);

    ts = _float_type_spec(4);
    ts->type_id = 14;
    vector_push_back(&type_list, &ts);
    hashmap_put(&type_map, ts);

    ts = _float_type_spec(8);
    ts->type_id = 15;
    vector_push_back(&type_list, &ts);
    hashmap_put(&type_map, ts);

    ts = _bfloat16_type_spec();
    ts->type_id = 16;
    vector_push_back(&type_list, &ts);
    hashmap_put(&type_map, ts);

    ts = _type_type_spec();
    ts->type_id = 17;
    vector_push_back(&type_list, &ts);
    hashmap_put(&type_map, ts);

    ts = _range_type_spec();
    ts->type_id = 18;
    vector_push_back(&type_list, &ts);
    hashmap_put(&type_map, ts);
}

TypeSpec *type_spec_intern(TypeSpec *ts)
{
    TypeSpec *old_ts = hashmap_get(&type_map, ts);
    if (old_ts) {
        ASSERT(old_ts->type_id >= 0);
        type_spec_free(ts);
        return old_ts;
    } else {
        hashmap_put(&type_map, ts);
        ASSERT(ts->type_id < 0);
        ts->type_id = vector_size(&type_list);
        vector_push_back(&type_list, &ts);
        return ts;
    }
}

TypeSpec *generic_var_type_spec(char *name, int index, int sym_id, char *owner)
{
    TypeSpec *ts = mm_alloc_obj(ts);
    ts->kind = TYPE_GENERIC_VAR;
    ts->generic_var.name = name;
    ts->generic_var.index = index;
    ts->sym_id = sym_id;
    ts->generic_var.owner = owner;
    ts->type_id = -1;
    BUF(buf);
    type_spec_to_str(ts, &buf);
    ts->signature = atom_nstr(BUF_STR(buf), BUF_LEN(buf));
    FINI_BUF(buf);
    hashmap_entry_init(&ts->hnode, type_spec_hash(ts));
    return type_spec_intern(ts);
}

TypeSpec *specialized_type_spec(char *full_pkg, char *name, Vector *args, int sym_id)
{
    TypeSpec *ts = mm_alloc_obj(ts);
    ts->kind = TYPE_SPECIALIZED;
    ts->specialized.pkg = full_pkg;
    ts->specialized.name = name;
    ts->specialized.args = NULL;
    if (args != NULL) {
        Vector *arg_copy = vector_create_ptr();
        TypeSpec *arg;
        vector_foreach_object(arg, args) { vector_push_back(arg_copy, &arg); }
        ts->specialized.args = arg_copy;
    }
    ts->type_id = -1;
    ts->sym_id = sym_id;
    BUF(buf);
    type_spec_to_str(ts, &buf);
    ts->signature = atom_nstr(BUF_STR(buf), BUF_LEN(buf));
    FINI_BUF(buf);
    hashmap_entry_init(&ts->hnode, type_spec_hash(ts));
    return type_spec_intern(ts);
}

TypeSpec *unresolved_type_spec(TypeIdent *pkg, TypeIdent name, Vector *args)
{
    TypeSpec *ts = mm_alloc_obj(ts);
    ts->kind = TYPE_UNRESOLVED;
    if (pkg) ts->unresolved.pkg = *pkg;
    ts->unresolved.name = name;
    ts->unresolved.args = args;
    ts->sym_id = -1;
    ts->type_id = -1;
    return ts;
}

TypeSpec *union_type_spec(TypeSpec *first, TypeSpec *second)
{
    TypeSpec *ts = mm_alloc_obj(ts);
    ts->kind = TYPE_UNION;
    ts->union_type.args = vector_create_ptr();
    if (first) vector_push_back(ts->union_type.args, &first);
    if (second) vector_push_back(ts->union_type.args, &second);
    ts->sym_id = -1;
    ts->type_id = -1;
    return ts;
}

void union_type_spec_add_arg(TypeSpec *ts, TypeSpec *arg)
{
    if (ts->kind != TYPE_UNION) {
        return;
    }
    vector_push_back(ts->union_type.args, &arg);
}

static int cmp_typespec_by_type_id(const void *a, const void *b)
{
    TypeSpec **ts1 = (TypeSpec **)a;
    TypeSpec **ts2 = (TypeSpec **)b;
    return (*ts1)->type_id - (*ts2)->type_id;
}

TypeSpec *union_type_spec_intern(Vector *args)
{
    TypeSpec *ts = mm_alloc_obj(ts);
    ts->kind = TYPE_UNION;
    ts->union_type.args = args;
    ts->sym_id = -1;
    ts->type_id = -1;

    // sort args by type_id to ensure uniqueness
    // so that Union[A, B] and Union[B, A] are the same

    qsort(args->objs, args->size, args->obj_size, cmp_typespec_by_type_id);

    BUF(buf);
    type_spec_to_str(ts, &buf);
    ts->signature = atom_nstr(BUF_STR(buf), BUF_LEN(buf));
    FINI_BUF(buf);
    return type_spec_intern(ts);
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
    } else if (ts->kind == TYPE_FLOAT) {
        if (ts->int_flt_info.width == 2) {
            buf_write_char(buf, 'h');
        } else if (ts->int_flt_info.width == 4) {
            buf_write_char(buf, 'f');
        } else {
            buf_write_char(buf, 'd');
        }
    } else if (ts->kind == TYPE_BFLOAT16) {
        buf_write_char(buf, 'b');
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
        buf_write_char(buf, 'T');
        buf_write_str(buf, ts->generic_var.owner);
        buf_write_char(buf, ':');
        buf_write_str(buf, ts->generic_var.name);
        buf_write_char(buf, ';');
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
    } else if (ts->kind == TYPE_UNION) {
        buf_write_char(buf, 'U');
        TypeSpec *arg;
        vector_foreach_object(arg, ts->union_type.args) { type_spec_to_str(arg, buf); }
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
    char *path = NULL;
    char *type = NULL;
    if (dot) {
        path = atom_nstr(s, dot - s);
        type = atom_nstr(dot + 1, len - (dot - s) - 1);
    } else {
        type = atom_nstr(s, len);
    }
    TypeIdent _pkg = { .name = path };
    TypeIdent _name = { .name = type };
    if (dot) {
        return unresolved_type_spec(&_pkg, _name, NULL);
    } else {
        return unresolved_type_spec(NULL, _name, NULL);
    }
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
        case 'T': {
            s++;
            k = s;
            int len1 = 0;
            while (*s != ':' && *s != '\0') s++;
            len1 = s - k;
            s++; // skip ':'
            char *k2 = s;
            while (*s != ';' && *s != '\0') s++;
            char *owner = atom_nstr(k, len1);
            char *name = atom_nstr(k2, s - k2);
            ts = generic_var_type_spec(name, -1, -1, owner);
            if (*s == ';') s++;
            break;
        }
        case 'c': {
            ts = int8_type_spec();
            s++;
            break;
        }
        case 'C': {
            ts = uint8_type_spec();
            s++;
            break;
        }
        case 's': {
            ts = int16_type_spec();
            s++;
            break;
        }
        case 'S': {
            ts = uint16_type_spec();
            s++;
            break;
        }
        case 'i': {
            ts = int32_type_spec();
            s++;
            break;
        }
        case 'I': {
            ts = uint32_type_spec();
            s++;
            break;
        }
        case 'j': {
            ts = int64_type_spec();
            s++;
            break;
        }
        case 'J': {
            ts = uint64_type_spec();
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
        case 'U': {
            s++;
            ts = union_type_spec(NULL, NULL);
            while (*s != ';' && *s != '\0') {
                arg = __to_typespec(&s);
                if (arg) union_type_spec_add_arg(ts, arg);
            }
            if (*s == ';') s++;
            break;
        }
        case 'h': {
            ts = float16_type_spec();
            s++;
            break;
        }
        case 'f': {
            ts = float32_type_spec();
            s++;
            break;
        }
        case 'd': {
            ts = float64_type_spec();
            s++;
            break;
        }
        case 'b': {
            ts = bfloat16_type_spec();
            s++;
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
    } else if (ts->kind == TYPE_FLOAT) {
        buf_write_str(buf, "float");
        buf_write_int64(buf, ts->int_flt_info.width * 8);
    } else if (ts->kind == TYPE_BFLOAT16) {
        buf_write_str(buf, "bfloat16");
    } else if (ts->kind == TYPE_TYPE) {
        buf_write_str(buf, "type");
    } else if (ts->kind == TYPE_RANGE) {
        buf_write_str(buf, "range");
    } else if (ts->kind == TYPE_UNION) {
        TypeSpec *arg;
        int i = 0;
        vector_foreach_object(arg, ts->union_type.args)
        {
            if (i != 0) buf_write_str(buf, " | ");
            type_spec_print(arg, buf);
            i++;
        }
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
        case 'T': {
            s++;
            while (*s != ':' && *s != '\0') s++;
            s++;
            char *k2 = s;
            while (*s != ';' && *s != '\0') s++;
            buf_write_nstr(buf, k2, s - k2);
            if (*s == ';') s++;
            break;
        }
        case 'U': {
            s++;
            int i = 0;
            while (*s != ';' && *s != '\0') {
                if (i != 0) buf_write_str(buf, " | ");
                __typespec_str_print(&s, buf);
                i++;
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
        case 'h': {
            buf_write_str(buf, "float16");
            s++;
            break;
        }
        case 'f': {
            buf_write_str(buf, "float32");
            s++;
            break;
        }
        case 'd': {
            buf_write_str(buf, "float64");
            s++;
            break;
        }
        case 'b': {
            buf_write_str(buf, "bfloat16");
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

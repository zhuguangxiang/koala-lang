/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2023 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

// clang-format off
#include "symbol.h"
#include "typespec.h"
#include "mm.h"
// clang-format on

#ifdef __cplusplus
extern "C" {
#endif

TypeSpec *no_type_spec(void)
{
    TypeSpec *ts = mm_alloc_obj(ts);
    ts->kind = TYPE_NO_TYPE;
    return ts;
}

TypeSpec *int_type_spec(int width, int sign)
{
    TypeSpec *ts = mm_alloc_obj(ts);
    ts->kind = TYPE_INT;
    ts->int_flt_info.width = width;
    ts->int_flt_info.sign = sign;
    return ts;
}

TypeSpec *bool_type_spec(void)
{
    TypeSpec *ts = mm_alloc_obj(ts);
    ts->kind = TYPE_BOOL;
    return ts;
}

TypeSpec *str_type_spec(void)
{
    TypeSpec *ts = mm_alloc_obj(ts);
    ts->kind = TYPE_STR;
    return ts;
}

TypeSpec *object_type_spec(void)
{
    TypeSpec *ts = mm_alloc_obj(ts);
    ts->kind = TYPE_OBJECT;
    return ts;
}

TypeSpec *va_list_type_spec(void)
{
    TypeSpec *ts = mm_alloc_obj(ts);
    ts->kind = TYPE_VA_LIST;
    return ts;
}

TypeSpec *generic_var_type_spec(char *name, int index)
{
    TypeSpec *ts = mm_alloc_obj(ts);
    ts->kind = TYPE_GENERIC_VAR;
    ts->generic_var.name = name;
    ts->generic_var.index = index;
    return ts;
}

TypeSpec *specialized_type_spec(char *full_pkg, char *name, Vector *args)
{
    TypeSpec *ts = mm_alloc_obj(ts);
    ts->kind = TYPE_SPECIALIZED;
    ts->specialized.pkg = full_pkg;
    ts->specialized.name = name;
    ts->specialized.args = args;
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
    return ts;
}

/**
 * Determines if a TypeSpec is a value type.
 * Value types (primitive types like int, float, bool) require exact
 * memory layout matching and are strictly invariant as generic parameters.
 */
static int is_value_type(TypeSpec *type)
{
    if (!type) return 0;
    switch (type->kind) {
        case TYPE_INT:
        case TYPE_FLOAT:
        case TYPE_BFLOAT16:
        case TYPE_BOOL:
            return 1;
        default:
            return 0;
    }
}

/**
 * Checks for strict identity between two types.
 * Required for value-type generic parameters to ensure binary compatibility.
 */
static int type_spec_equal_strict(TypeSpec *a, TypeSpec *b)
{
    if (a == b) return 1;
    if (a->kind != b->kind || a->sym_id != b->sym_id) return 0;

    // Numerical values must have identical representation (sign and width)
    if (a->kind == TYPE_INT || a->kind == TYPE_FLOAT || a->kind == TYPE_BFLOAT16) {
        return a->int_flt_info.width == b->int_flt_info.width &&
               a->int_flt_info.sign == b->int_flt_info.sign;
    }
    return 1;
}

/**
 * Checks if the 'src' type is compatible with the 'dst' type.
 *
 * Rules for High-Performance Type System:
 * 1. Top Type: TYPE_OBJECT is the root and accepts any type.
 * 2. Strict Kind Matching: Except for Object, kinds must match (e.g., no implicit
 * int-to-float).
 * 3. Numerical Widening: For INT/FLOAT, source width <= destination width is allowed.
 *    Note: Semantically compatible but requires explicit 'cast' instructions during
 * codegen due to binary representation (memory layout) mismatch.
 * 4. Generic Variance:
 *    - Reference Types (Classes): Covariant (e.g., List[Dog] -> List[Animal]).
 *    - Value Types (Primitives): Invariant (Generic parameters must be strictly
 * compatible).
 */
int type_spec_compatible(TypeSpec *dst, TypeSpec *src)
{
    if (!dst || !src) return 0;

    if (dst == src) return 1;

    // Rule 1: TYPE_OBJECT is the Top Type (Root of the type hierarchy)
    if (dst->kind == TYPE_OBJECT) return 1;

    // Rule 2: Strict kind matching (Semantic barrier)
    if (dst->kind != src->kind) return 0;

    // Rule 3: Numeric Widening Logic (INT/FLOAT/BFLOAT16)
    // Semantically compatible, but memory layout is incompatible.
    // Insert 'cast' instructions during code generation.
    if (dst->kind == TYPE_INT || dst->kind == TYPE_FLOAT) {
        if (dst->int_flt_info.sign != src->int_flt_info.sign) return 0;
        if (dst->int_flt_info.width > src->int_flt_info.width) return 0;
        return 1;
    }

    if (dst->kind == TYPE_FLOAT || dst->kind == TYPE_BFLOAT16) {
        // float16/bfloat16 can be promoted to float32/64
        return dst->int_flt_info.width >= src->int_flt_info.width;
    }

    // Rule 4: Symbol ID and Inheritance Check
    if (dst->sym_id != src->sym_id) {
        // Check if 'src' is a subtype of 'dst' in the symbol table
        // return is_subtype_of(src->sym_id, dst->sym_id);
        // TODO:
        UNREACHABLE();
        return 0;
    }

    if (dst->kind == TYPE_GENERIC_VAR) {
        return dst->generic_var.index == src->generic_var.index;
    }

    // Rule 5: Structural Recursion for Specialized Types (Generics)

    if (dst->kind == TYPE_SPECIALIZED) {
        if (dst->sym_id != src->sym_id) return 0;
        int d_args_size = vector_size(dst->specialized.args);
        int s_args_size = vector_size(src->specialized.args);
        if (d_args_size != s_args_size) return 0;

        // Handle Variance based on storage model

        for (int i = 0; i < d_args_size; i++) {
            TypeSpec **d_arg_p = vector_get(dst->specialized.args, i);
            TypeSpec **s_arg_p = vector_get(src->specialized.args, i);
            TypeSpec *d_arg = *d_arg_p;
            TypeSpec *s_arg = *s_arg_p;
            // generic parameters must be strictly compatible(invariant).
            // List[int32] and List[int64] are not compatible.
            // List[Dog] and List[Animal] are compatible.
            if (is_value_type(d_arg)) {
                if (!type_spec_equal_strict(d_arg, s_arg)) return 0;
            } else {
                // reference type
                if (!type_spec_compatible(d_arg, s_arg)) return 0;
            }
        }

        return 1;
    }

    UNREACHABLE();
    return 0;
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
        buf_write_char(buf, '<');
        buf_write_str(buf, ts->generic_var.name);
        buf_write_char(buf, ';');
    } else if (ts->kind == TYPE_SPECIALIZED) {
        buf_write_char(buf, 'L');
        buf_write_str(buf, ts->specialized.name);
        buf_write_char(buf, ';');
    } else {
        UNREACHABLE();
    }

    return 0;
}

TypeSpec *type_spec_from_str(const char *s)
{
    if (!s || s[0] == 0) return NULL;

    TypeSpec *ty = NULL;

    int len = strlen(s);

    if (len == 1) {
        int width;
        int sign;
        char ch = s[0];
        switch (ch) {
            case 'c':
                width = 1;
                sign = 1;
                break;
            case 'C':
                width = 1;
                sign = 0;
                break;
            case 's':
                width = 2;
                sign = 1;
                break;
            case 'S':
                width = 2;
                sign = 0;
                break;
            case 'i':
                width = 4;
                sign = 1;
                break;
            case 'I':
                width = 4;
                sign = 0;
                break;
            case 'j':
                width = 8;
                sign = 1;
                break;
            case 'J':
                width = 8;
                sign = 0;
                break;
            case 'u':
                return str_type_spec();
                break;
            case 'z':
                return bool_type_spec();
                break;
            case 'o':
                return object_type_spec();
                break;
            default:
                UNREACHABLE();
                break;
        }
        ty = int_type_spec(width, sign);
        return ty;
    }

    if (!strcmp(s, "...")) {
        return va_list_type_spec();
    }

    UNREACHABLE();
    return NULL;
}

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

void type_spec_str_print(char *s, Buffer *buf)
{
    if (!s || s[0] == 0) {
        buf_write_str(buf, "unk");
        return;
    }

    int len = strlen(s);

    if (len == 1) {
        char ch = s[0];
        switch (ch) {
            case 'c':
                buf_write_str(buf, "int8");
                break;
            case 'C':
                buf_write_str(buf, "uint8");
                break;
            case 's':
                buf_write_str(buf, "int16");
                break;
            case 'S':
                buf_write_str(buf, "uint16");
                break;
            case 'i':
                buf_write_str(buf, "int32");
                break;
            case 'I':
                buf_write_str(buf, "uint32");
                break;
            case 'j':
                buf_write_str(buf, "int64");
                break;
            case 'J':
                buf_write_str(buf, "uint64");
                break;
            case 'f':
                buf_write_str(buf, "float");
                break;
            case 'z':
                buf_write_str(buf, "bool");
                break;
            case 'u':
                buf_write_str(buf, "str");
                break;
            case 'o':
                buf_write_str(buf, "object");
                break;
            default:
                break;
        }
        return;
    }

    if (!strcmp(s, "...")) {
        buf_write_str(buf, "...");
        return;
    }

    if (s[0] == '<') {
        char *_s = s + 1;
        while (*_s != ';') _s++;
        buf_write_nstr(buf, s + 1, _s - (s + 1));
        return;
    }

    if (s[0] == 'L') {
        char *_s = s + 1;
        while (*_s != ';') _s++;
        buf_write_nstr(buf, s + 1, _s - (s + 1));
        return;
    }

    UNREACHABLE();
}

#ifdef __cplusplus
}
#endif

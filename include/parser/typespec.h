/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2023 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_TYPESPEC_H_
#define _KOALA_TYPESPEC_H_

#include "buffer.h"
#include "hashmap.h"
#include "loc.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _TypeKind {
    TYPE_NO_TYPE,
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_BFLOAT16,
    TYPE_BOOL,
    TYPE_STR,
    TYPE_ANY,
    TYPE_VA_LIST,
    TYPE_TYPE,
    TYPE_RANGE,
    TYPE_UNION,
    TYPE_UNRESOLVED,
    TYPE_GENERIC_VAR,
    TYPE_SPECIALIZED,
    TYPE_KLASS,
    TYPE_PROTO,
    TYPE_OPTIONAL,
    TYPE_MANGLED, // only for loading from klc
} TypeKind;

typedef struct _TypeIdent {
    char *name;
    Loc loc;
} TypeIdent;

#define MOD_ID(_name, _s, _l)  TypeIdent _name = { _s, _l }
#define NAME_ID(_name, _s, _l) TypeIdent _name = { _s, _l }

typedef struct _TypeSpec {
    HashMapEntry hnode;
    TypeKind kind;
    int checked;
    int sym_id;
    int type_id;
    char *signature;
    Loc loc;
    union {
        // int/float width
        struct {
            int width;
            int sign;
        } int_flt_info;

        // T
        struct {
            char *owner;
            char *name;
            int index;
        } generic_var;

        // Open: args include T
        // List[T], not include List[int]
        struct {
            char *pkg;
            char *name;
            Vector *args;
        } specialized;

        // T, Bar, Bar[T], Bar[int]
        struct {
            TypeIdent pkg;
            TypeIdent name;
            Vector *args;
        } unresolved;

        // A | B | C
        struct {
            Vector *args;
        } union_type;

        // Closed: klass type for definition
        // don't have any T args, all args are concrete types
        // don't save parameter types here
        // include List[int], not include List[T]
        struct {
            char *pkg;
            char *name;
        } klass_type;

        // proto type
        struct {
            Vector *args;
            struct _TypeSpec *ret;
        } proto_type;

        // optional
        struct {
            struct _TypeSpec *src;
        } opt;

        // mangled type
        struct {
            char *name;
            Vector *args;
        } mangled;
    };
} TypeSpec;

#define type_spec_loc(ty, _loc) (ty)->loc = (_loc)

TypeSpec *generic_var_type_spec(char *name, int index, int sym_id, char *owner);
TypeSpec *specialized_type_spec(char *full_pkg, char *name, Vector *args, int sym_id);
TypeSpec *unresolved_type_spec(TypeIdent *pkg, TypeIdent name, Vector *args);
TypeSpec *union_type_spec(TypeSpec *first, TypeSpec *second);
void union_type_spec_add_arg(TypeSpec *ts, TypeSpec *arg);
TypeSpec *union_type_spec_intern(Vector *args);
TypeSpec *klass_type_spec(char *path, char *name);
TypeSpec *func_type_spec(Vector *args, TypeSpec *ret);
TypeSpec *func_type_spec_from_arginfo(Vector *arg_infos, TypeSpec *ret);
TypeSpec *optional_type_spec(TypeSpec *src);
TypeSpec *optional_type_spec_intern(TypeSpec *src);

static inline int type_is_optional(TypeSpec *ts) { return ts->kind == TYPE_OPTIONAL; }

int type_spec_to_str(TypeSpec *ts, Buffer *buf);
TypeSpec *type_spec_from_str(const char *s);

void type_spec_print(TypeSpec *ts, Buffer *buf);
void type_spec_str_print(char *s, Buffer *buf);

void typespec_init(void);
void typespec_fini(void);
TypeSpec *type_spec_intern(TypeSpec *ts);
TypeSpec *type_spec_get_by_id(int type_id);

static inline TypeSpec *no_type_spec(void) { return type_spec_get_by_id(0); }
static inline TypeSpec *int8_type_spec(void) { return type_spec_get_by_id(1); }
static inline TypeSpec *uint8_type_spec(void) { return type_spec_get_by_id(2); }
static inline TypeSpec *int16_type_spec(void) { return type_spec_get_by_id(3); }
static inline TypeSpec *uint16_type_spec(void) { return type_spec_get_by_id(4); }
static inline TypeSpec *int32_type_spec(void) { return type_spec_get_by_id(5); }
static inline TypeSpec *uint32_type_spec(void) { return type_spec_get_by_id(6); }
static inline TypeSpec *int64_type_spec(void) { return type_spec_get_by_id(7); }
static inline TypeSpec *uint64_type_spec(void) { return type_spec_get_by_id(8); }
static inline TypeSpec *bool_type_spec(void) { return type_spec_get_by_id(9); }
static inline TypeSpec *str_type_spec(void) { return type_spec_get_by_id(10); }
static inline TypeSpec *object_type_spec(void) { return type_spec_get_by_id(11); }
static inline TypeSpec *va_list_type_spec(void) { return type_spec_get_by_id(12); }
static inline TypeSpec *float16_type_spec(void) { return type_spec_get_by_id(13); }
static inline TypeSpec *float32_type_spec(void) { return type_spec_get_by_id(14); }
static inline TypeSpec *float64_type_spec(void) { return type_spec_get_by_id(15); }
static inline TypeSpec *bfloat16_type_spec(void) { return type_spec_get_by_id(16); }
static inline TypeSpec *type_type_spec(void) { return type_spec_get_by_id(17); }
static inline TypeSpec *range_type_spec(void) { return type_spec_get_by_id(18); }

void update_builtin_types(HashMap *stbl);

void type_spec_free(TypeSpec *ts);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_TYPESPEC_H_ */

/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2023 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_TYPESPEC_H_
#define _KOALA_TYPESPEC_H_

#include "buffer.h"
#include "hashmap.h"
#include "loc.h"

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
    TYPE_OBJECT,
    TYPE_VA_LIST,
    TYPE_TYPE,
    TYPE_RANGE,
    TYPE_UNRESOLVED,
    TYPE_GENERIC_VAR,
    TYPE_SPECIALIZED,
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
            char *name;
            int index;
        } generic_var;

        // List[int]
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
    };
} TypeSpec;

#define type_spec_loc(ty, _loc) (ty)->loc = (_loc)

TypeSpec *generic_var_type_spec(char *name, int index, int sym_id);
TypeSpec *specialized_type_spec(char *full_pkg, char *name, Vector *args, int sym_id);
TypeSpec *unresolved_type_spec(TypeIdent *pkg, TypeIdent name, Vector *args);

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

void type_spec_free(TypeSpec *ts);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_TYPESPEC_H_ */

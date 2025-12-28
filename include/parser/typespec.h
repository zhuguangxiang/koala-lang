/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2023 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_TYPESPEC_H_
#define _KOALA_TYPESPEC_H_

#include "buffer.h"
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
    TypeKind kind;
    intptr_t sym_id;
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

TypeSpec *no_type_spec(void);
TypeSpec *int_type_spec(int width, int sign);
TypeSpec *bool_type_spec(void);
TypeSpec *str_type_spec(void);
TypeSpec *object_type_spec(void);
TypeSpec *va_list_type_spec(void);
TypeSpec *generic_var_type_spec(char *name, int index, void *sym_id);
TypeSpec *specialized_type_spec(char *full_pkg, char *name, Vector *args);
TypeSpec *unresolved_type_spec(TypeIdent *pkg, TypeIdent name, Vector *args);

int type_spec_to_str(TypeSpec *ts, Buffer *buf);
TypeSpec *type_spec_from_str(const char *s);

void type_spec_print(TypeSpec *ts, Buffer *buf);
void type_spec_str_print(char *s, Buffer *buf);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_TYPESPEC_H_ */

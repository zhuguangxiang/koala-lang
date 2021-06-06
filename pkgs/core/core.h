/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 */

#ifndef _KOALA_CORE_H_
#define _KOALA_CORE_H_

#include "util/common.h"
#include "util/hashmap.h"
#include "util/list.h"
#include "util/vector.h"

#ifdef __cplusplus
extern "C" {
#endif

// clang-format off

typedef struct _TypeInfo   TypeInfo;
typedef struct _Object     Object;
typedef struct _TypeParam  TypeParam;
typedef struct _FieldInfo  FieldInfo;
typedef struct _MethodInfo MethodInfo;
typedef struct _ProtoInfo  ProtoInfo;
typedef struct _CodeInfo   CodeInfo;
typedef struct _VarInfo    VarInfo;
typedef struct _MbrInfo    MbrInfo;
typedef struct _MethodDef  MethodDef;

// clang-format on

// ------ object ------------------------------------------------------------

// The macro indicates which is an `Object`
#define OBJECT_HEAD TypeInfo *type;

// The macro indicates an `Object` with type parameters
#define GENERIC_OBJECT_HEAD OBJECT_HEAD uint32 tp_map;

struct _Object {
    OBJECT_HEAD
};

// ------ type parameter ----------------------------------------------------

#define TP_REF  1
#define TP_I8   2
#define TP_I16  3
#define TP_I32  4
#define TP_I64  5
#define TP_F16  6
#define TP_F32  7
#define TP_F64  8
#define TP_BOOL 9
#define TP_CHAR 10
#define TP_MASK 15

#define tp_index(tp, idx) (((tp) >> ((idx)*4)) & TP_MASK)

#define is_ref(tp, idx)  (tp_index(tp, idx) == TP_REF)
#define is_i8(tp, idx)   (tp_index(tp, idx) == TP_I8)
#define is_i16(tp, idx)  (tp_index(tp, idx) == TP_I16)
#define is_i32(tp, idx)  (tp_index(tp, idx) == TP_I32)
#define is_i64(tp, idx)  (tp_index(tp, idx) == TP_I64)
#define is_f16(tp, idx)  (tp_index(tp, idx) == TP_F16)
#define is_f32(tp, idx)  (tp_index(tp, idx) == TP_F32)
#define is_f64(tp, idx)  (tp_index(tp, idx) == TP_F64)
#define is_bool(tp, idx) (tp_index(tp, idx) == TP_BOOL)
#define is_char(tp, idx) (tp_index(tp, idx) == TP_CHAR)

// clang-format off

#define TP_0(t0) (t0)
#define TP_1(t0, t1) (TP_1(t0) + (t1) << 4)
#define TP_2(t0, t1, t2) (TP_2(t0, t1) + (t2) << 8)
#define TP_3(t0, t1, t2, t3) (TP_3(t0, t1, t2) + (t3) << 12)
#define TP_4(t0, t1, t2, t3, t4) (TP_4(t0, t1, t2, t3) + (t4) << 16)
#define TP_5(t0, t1, t2, t3, t4, t5) (TP_5(t0, t1, t2, t3, t4) + (t5) << 20)
#define TP_6(t0, t1, t2, t3, t4, t5, t6) \
    (TP_6(t0, t1, t2, t3, t4, t5) + (t6) << 24)
#define TP_7(t0, t1, t2, t3, t4, t5, t6, t7) \
    (TP_7(t0, t1, t2, t3, t4, t5, t6) + (t7) << 28)

// clang-format on

int tp_size(uint32 tp_map, int index);

// generic type T: its upper bound of T is Any

int32 generic_any_hash(uintptr obj, int isref);
int8 generic_any_equal(uintptr obj, uintptr other, int isref);
Object *generic_any_class(uintptr obj, int tpkind);
Object *generic_any_tostr(uintptr obj, int tpkind);

// ------ type info ---------------------------------------------------------

// type flags

#define TF_CLASS 1
#define TF_TRAIT 2
#define TF_ENUM  3
#define TF_FINAL 0x10

#define type_is_class(type) (((type)->flags & 0x0F) == TF_CLASS)
#define type_is_trait(type) (((type)->flags & 0x0F) == TF_TRAIT)
#define type_is_enum(type)  (((type)->flags & 0x0F) == TF_ENUM)
#define type_is_final(type) ((type)->flags & TF_FINAL)

// type
struct _TypeInfo {
    // virtual table
    uintptr **vtbl;
    // object map
    int *objmap;
    // type name
    char *name;
    // type flags
    int flags;
    // type parameters
    Vector *params;
    // self methods
    Vector *methods;
    // self fields
    Vector *fields;
    // first class or trait
    TypeInfo *base;
    // other traits
    Vector *traits;
    // line resolution order
    Vector *lro;
    // all symbols
    HashMap *mtbl;
};

// type parameter
struct _TypeParam {
    char name[8];
    TypeInfo *bound;
};

// ------ member of typeinfo or module --------------------------------------

// clang-format off

#define MEMBER_HEAD HashMapEntry entry; char *name; void *parent; int8 kind;

// clang-format on

#define MBR_FIELD_KIND 1
#define MBR_KFUNC_KIND 2
#define MBR_CFUNC_KIND 3
#define MBR_PROTO_KIND 4
#define MBR_TYPE_KIND  5
#define MBR_VAR_KIND   6

struct _FieldInfo {
    MEMBER_HEAD
    int offset;
};

#define METH_KFUNC_KIND 1
#define METH_CFUNC_KIND 2

struct _MethodInfo {
    MEMBER_HEAD
    int8 inherit;
    void *ptr;
};

struct _ProtoInfo {
    MEMBER_HEAD
};

struct _CodeInfo {
    Vector locvars;
    Vector freevars;
    Vector upvars;
    // ConstPool *consts;
    uint32 size;
    uint8 codes[0];
};

struct _VarInfo {
    uintptr val[2];
};

struct _MbrInfo {
    MEMBER_HEAD
};

HashMap *mbr_map_new(void);
MbrInfo *mbr_new_field(char *name, uint32 offset);
MbrInfo *mbr_new_kfunc(char *name, CodeInfo *code);
MbrInfo *mbr_new_cfunc(char *name, void *cfunc);
MbrInfo *mbr_new_proto(char *name);
MbrInfo *mbr_new_type(TypeInfo *type);
MbrInfo *mbr_new_gvar(char *name);

TypeInfo *type_new(char *name, int flags, Vector *params, TypeInfo *base,
                   Vector *traits);
#define type_new_simple(name, flags) type_new(name, flags, nil, nil, nil)
// void type_add_field(TypeInfo *type, FieldInfo *field);
// void type_add_kfunc(TypeInfo *type, CodeInfo *code);
// void type_add_cfunc(TypeInfo *type, CFuncInfo *cfunc);
// void type_add_proto(TypeInfo *type, ProtoInfo *proto);
void type_ready(TypeInfo *type);
void type_show(TypeInfo *type);

// ------ method define -----------------------------------------------------

struct _MethodDef {
    char *name;
    char *ptype;
    char *rtype;
    char *fname;
    void *faddr;
};

// clang-format off

#define METHOD_DEF(name, ptype, rtype, func) \
    { name, ptype, rtype, #func, func }

// clang-format on

void type_add_methoddefs(TypeInfo *type, MethodDef *def, int size);

// ------ array -------------------------------------------------------------

Object *array_new(uint32 tp_map);
void array_reserve(Object *self, int32 count);
void array_append(Object *self, uintptr val);
int32 array_length(Object *self);
void array_set(Object *self, uint32 index, uintptr val);
uintptr array_get(Object *self, uint32 index);
void array_print(Object *self);

// ------ map ---------------------------------------------------------------

Object *map_new(uint32 tp_map);
bool map_put_absent(Object *self, uintptr key, uintptr val);
void map_put(Object *self, uintptr key, uintptr val, uintptr *old_val);
bool map_get(Object *self, uintptr key, uintptr *val);
bool map_remove(Object *self, uintptr key, uintptr *val);

// string

Object *string_new(char *s);

// ------ reflect -----------------------------------------------------------

// class & field & method

Object *class_new(TypeInfo *type);
Object *field_new();
Object *method_new();

// ------ package -----------------------------------------------------------

void pkg_add_type(char *path, TypeInfo *type);
void pkg_add_var(char *path, char *name);
void pkg_add_cfunc(char *path, char *name, void *cfunc);
void pkg_add_kfunc(char *path, char *name, CodeInfo *code);

void init_core_pkg(void);
void fini_core_pkg(void);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_CORE_H_ */

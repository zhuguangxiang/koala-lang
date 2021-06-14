/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_CORE_H_
#define _KOALA_CORE_H_

#include "util/common.h"
#include "util/hashmap.h"
#include "util/list.h"
#include "util/vector.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uintptr anyref;
typedef uintptr objref;
typedef struct _VTable VTable;
typedef struct _TypeInfo TypeInfo;
typedef struct _TypeDesc TypeDesc;
typedef struct _TypeProto TypeProto;
typedef struct _TypeKlass TypeKlass;

typedef struct _FieldNode FieldNode;
typedef struct _FuncNode FuncNode;
typedef struct _ProtoNode ProtoNode;
typedef struct _TypeNode TypeNode;
typedef struct _VarNode VarNode;
typedef struct _PkgNode PkgNode;
typedef struct _MNode MNode;

typedef struct _CodeInfo CodeInfo;
typedef struct _RelInfo RelInfo;
typedef struct _MethodDef MethodDef;

/*
object layout, like c++ object layout

+-----------+
|  *vtbl_0  |   <-- object
+-----------+
|   ....    |   <-- trait
+-----------+
|  *vtbl_n  |   <-- trait
+-----------+
|  struct   |
+-----------+

*/

/*----------------------------------------------------------------------------*\
|*                    type parameter(TP) for generic type                     *|
\*----------------------------------------------------------------------------*/

#define TP_I8_KIND   1
#define TP_I16_KIND  2
#define TP_I32_KIND  3
#define TP_I64_KIND  4
#define TP_F32_KIND  5
#define TP_F64_KIND  6
#define TP_BOOL_KIND 7
#define TP_CHAR_KIND 8
#define TP_REF_KIND  9
#define TP_KIND_MASK 15

#define tp_index(tp, idx) (((tp) >> ((idx)*4)) & TP_KIND_MASK)

#define tp_is_i8(tp, idx)   (tp_index(tp, idx) == TP_I8_KIND)
#define tp_is_i16(tp, idx)  (tp_index(tp, idx) == TP_I16_KIND)
#define tp_is_i32(tp, idx)  (tp_index(tp, idx) == TP_I32_KIND)
#define tp_is_i64(tp, idx)  (tp_index(tp, idx) == TP_I64_KIND)
#define tp_is_f32(tp, idx)  (tp_index(tp, idx) == TP_F32_KIND)
#define tp_is_f64(tp, idx)  (tp_index(tp, idx) == TP_F64_KIND)
#define tp_is_bool(tp, idx) (tp_index(tp, idx) == TP_BOOL_KIND)
#define tp_is_char(tp, idx) (tp_index(tp, idx) == TP_CHAR_KIND)
#define tp_is_ref(tp, idx)  (tp_index(tp, idx) == TP_REF_KIND)

/* clang-format off */

#define TP_1(t0) (t0)
#define TP_2(t0, t1) (TP_1(t0) + (t1) << 4)
#define TP_3(t0, t1, t2) (TP_2(t0, t1) + (t2) << 8)
#define TP_4(t0, t1, t2, t3) (TP_3(t0, t1, t2) + (t3) << 12)
#define TP_5(t0, t1, t2, t3, t4) (TP_4(t0, t1, t2, t3) + (t4) << 16)
#define TP_6(t0, t1, t2, t3, t4, t5) (TP_5(t0, t1, t2, t3, t4) + (t5) << 20)
#define TP_7(t0, t1, t2, t3, t4, t5, t6) \
    (TP_6(t0, t1, t2, t3, t4, t5) + (t6) << 24)
#define TP_8(t0, t1, t2, t3, t4, t5, t6, t7) \
    (TP_7(t0, t1, t2, t3, t4, t5, t6) + (t7) << 28)

/* clang-format on */

/* var obj T, where the upper bound of T is Any */

int32 tp_any_hash(anyref obj, int ref);
bool tp_any_equal(anyref obj, anyref other, int ref);

/* get type parameter size */
static inline int tp_size(uint32 tp_map, int index)
{
    static int __size__[] = { 0, 1, 2, 4, 8, 4, 8, 1, 4, PTR_SIZE };
    return __size__[tp_index(tp_map, index)];
}

/*----------------------------------------------------------------------------*\
|*                type info, virtual table and type descriptor                *|
\*----------------------------------------------------------------------------*/

/* type flags */

#define TF_CLASS 1
#define TF_TRAIT 2
#define TF_ENUM  3
#define TF_FINAL 0x10

#define type_is_class(type) (((type)->flags & 0x0F) == TF_CLASS)
#define type_is_trait(type) (((type)->flags & 0x0F) == TF_TRAIT)
#define type_is_enum(type)  (((type)->flags & 0x0F) == TF_ENUM)
#define type_is_final(type) ((type)->flags & TF_FINAL)

/* type info */

struct _TypeInfo {
    /* type name */
    char *name;
    /* type flags */
    int flags;
    /* count vtbl */
    int num_vtbl;
    /* virtual table */
    VTable **vtbl;
    // object map
    int *objmap;
    /* type parameters */
    Vector *params;
    /* self methods */
    Vector *methods;
    /* self fields */
    Vector *fields;
    /* first class or trait */
    TypeInfo *base;
    /* other traits */
    Vector *traits;
    /* line resolution order */
    Vector *lro;
    /* all symbols */
    HashMap *mtbl;
};

/* virtual table */

struct _VTable {
    /* object's type */
    TypeInfo *type;
    /* offset of object head ptr */
    int head;
    /* offset of data struct */
    int data;
    /* number of functions */
    int num;
    /* function array, terminated with nil */
    FuncNode *func[1];
};

#define __GET_VTBL(ptr) (*(VTable **)(ptr))
#define __GET_HEAD(ptr) ((ptr) + __GET_VTBL(ptr)->head)
#define __GET_TYPE(ptr) (__GET_VTBL(ptr)->type)

/* type descriptor */

#define DESC_I8_KIND    1
#define DESC_I16_KIND   2
#define DESC_I32_KIND   3
#define DESC_I64_KIND   4
#define DESC_F32_KIND   5
#define DESC_F64_KIND   6
#define DESC_BOOL_KIND  7
#define DESC_CHAR_KIND  8
#define DESC_STR_KIND   9
#define DESC_ANY_KIND   10
#define DESC_ARRAY_KIND 11
#define DESC_MAP_KIND   12
#define DESC_PROTO_KIND 13
#define DESC_KLASS_KIND 14

struct _TypeDesc {
    int kind;
};

struct _TypeProto {
    int kind;
    TypeDesc *ret;
    Vector *params;
};

struct _TypeKlass {
    int kind;
    char *path;
    char *name;
};

/*----------------------------------------------------------------------------*\
|*                    member node for typeinfo and package                    *|
\*----------------------------------------------------------------------------*/

/* clang-format off */

#define MNODE_HEAD HashMapEntry entry; char *name; int8 kind;

/* clang-format on */

#define MNODE_FIELD_KIND 1
#define MNODE_KFUNC_KIND 2
#define MNODE_CFUNC_KIND 3
#define MNODE_PROTO_KIND 4
#define MNODE_TYPE_KIND  5
#define MNODE_VAR_KIND   6
#define MNODE_PKG_KIND   7

struct _FieldNode {
    MNODE_HEAD
    TypeDesc *desc;
    int offset;
};

struct _FuncNode {
    MNODE_HEAD
    int8 inherit;
    int16 slot;
    TypeDesc *desc;
    PkgNode *pkg;
    uintptr ptr;
};

struct _ProtoNode {
    MNODE_HEAD
    TypeDesc *desc;
};

struct _TypeNode {
    MNODE_HEAD
    TypeInfo *type;
};

struct _VarNode {
    MNODE_HEAD
    TypeDesc *desc;
    anyref val;
};

struct _PkgNode {
    MNODE_HEAD
    HashMap map;
    Vector *rel;
    Vector *gc_map;
};

struct _MNode {
    MNODE_HEAD
};

struct _CodeInfo {
    Vector locvars;
    Vector freevars;
    Vector upvars;
    uint32 size;
    uint8 codes[0];
};

struct _RelInfo {
    uintptr addr;
    char *path;
    char *name;
};

TypeInfo *type_new(char *name, int flags);
int type_set_base(TypeInfo *type, TypeInfo *base);
int type_add_typeparam(TypeInfo *type, char *name, TypeInfo *bound);
int type_add_trait(TypeInfo *type, TypeInfo *trait);
int type_add_field(TypeInfo *ty, char *name, TypeDesc *desc);
int type_add_kfunc(TypeInfo *ty, char *name, TypeDesc *desc, CodeInfo *code);
int type_add_cfunc(TypeInfo *ty, char *name, TypeDesc *desc, void *ptr);
int type_add_proto(TypeInfo *type, char *name, TypeDesc *desc);
void type_ready(TypeInfo *type);
void type_show(TypeInfo *type);
int type_get_func_slot(TypeInfo *type, char *name);
FuncNode *object_get_func(objref obj, int offset);

/* c method define */

struct _MethodDef {
    char *name;
    char *ptype;
    char *rtype;
    char *fname;
    void *faddr;
};

/* clang-format off */

#define METHOD_DEF(name, ptype, rtype, func) \
    { name, ptype, rtype, #func, func }

#define type_add_methdefs(type, def) do { \
    for (int i = 0; i < COUNT_OF(def); i++) \
        type_add_cfunc(type, (def + i)->name, NULL, (def + i)->faddr); \
} while (0)

/* clang-format on */

/*----------------------------------------------------------------------------*\
|*                       intrinsic objects & package                          *|
\*----------------------------------------------------------------------------*/

/* array object */

objref array_new(uint32 tp_map);
void array_reserve(objref self, int32 count);
void array_append(objref self, anyref val);
int32 array_length(objref self);
/* if index is out of bounds, panic() */
void array_set(objref self, uint32 index, anyref val);
/* if index is out of bounds, panic() */
anyref array_get(objref self, uint32 index);
void array_print(objref self);

/* map object */

objref map_new(uint32 tp_map);
bool map_put_absent(objref self, anyref key, anyref val);
void map_put(objref self, anyref key, anyref val, anyref *old);
bool map_get(objref self, anyref key, anyref *val);
bool map_remove(objref self, anyref key, anyref *val);

/* string object */

objref string_new(char *s);
void string_show(objref self);

/* reflect(class, field, method, package) object */

objref class_new(objref obj);

/* package operations */

void pkg_add_type(char *path, TypeInfo *type);
void pkg_add_var(char *path, char *name, TypeDesc *desc);
void pkg_add_cfunc(char *path, char *name, TypeDesc *desc, void *ptr);
void pkg_add_kfunc(char *path, char *name, TypeDesc *desc, CodeInfo *code);

/* core pkg("/") initialize and finalize */

void init_core(void);
void fini_core(void);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_CORE_H_ */

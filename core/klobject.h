/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_TYPE_H_
#define _KOALA_TYPE_H_

#include "util/hashmap.h"
#include "util/typedesc.h"
#include "util/vector.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _KlNode KlNode;
typedef struct _KlField KlField;
typedef struct _KlFunc KlFunc;
typedef struct _KlProto KlProto;
typedef struct _KlCode KlCode;
typedef struct _KlType KlType;
typedef struct _KlFuncTbl KlFuncTbl;
typedef struct _KlValue KlValue;

#define KL_FIELD_NODE 1
#define KL_FUNC_NODE  2
#define KL_IFUNC_NODE 3
#define KL_TYPE_NODE  4

// clang-format off
#define KL_NODE_HEAD HashMapEntry entry; char *name; TypeDesc *desc; \
    void *owner; uint8 kind;
// clang-format on

struct _KlNode {
    KL_NODE_HEAD
};

KlField *mtbl_add_field(HashMap *m, char *name);
KlFunc *mtbl_add_func(HashMap *m, char *name);
int mtbl_add_type(HashMap *m, KlType *ty);
int mtbl_equal_func(void *m1, void *m2);

// koala field(variable)
struct _KlField {
    KL_NODE_HEAD
    // field or var index
    uint16 offset;
};

// koala function
struct _KlFunc {
    KL_NODE_HEAD
    // is inherit?
    uint8 inherit;
    // virtual table slot
    uint16 slot;
    // cfunc or kfunc
    int cfunc;
    // cfunc or KlCode
    uintptr ptr;
};

// koala interface function
struct _KlProto {
    KL_NODE_HEAD
    // virtual table slot
    uint16 slot;
};

// koala code
struct _KlCode {
    // local vars
    Vector *locvec;
    // closure, up vars
    Vector *upvec;
    // constants pool(literal string etc)
    void *consts;
    // modules pool(global variable)
    void *modules;
    // code size
    int size;
    // byte codes
    uint8 *codes;
};

#define TP_CLASS 1
#define TP_TRAIT 2
#define TP_ENUM  4
#define TP_MOD   8
#define TP_FINAL 16

#define TP_IS_CLASS(tp) ((tp)->flags & TP_CLASS)
#define TP_IS_TRAIT(tp) ((tp)->flags & TP_TRAIT)
#define TP_IS_ENUM(tp)  ((tp)->flags & TP_ENUM)
#define TP_IS_MOD(tp)   ((tp)->flags & TP_MOD)
#define TP_IS_FINAL(tp) ((tp)->flags & TP_FINAL)

// type info(meta info)
struct _KlType {
    KL_NODE_HEAD
    // type flags
    uint8 flags;
    // object gc flag
    uint8 gc;
    // number of virtual table
    uint16 num_vtbl;
    // virtual table
    KlFuncTbl *vtbl;

    // class/trait linar resolution order
    Vector *lro;
    // member map, include inherited members
    HashMap *mtbl;
    // singleton(module) index
    uint32 instance;

    // below are static meta info
    // first class/trait
    KlType *base;
    // other traits
    Vector *traits;
    // type params
    Vector *params;
    // self methods
    Vector *methods;
    // self fields
    Vector *fields;
    // self types(module)
    Vector *types;
};

int type_set_base(KlType *ty, KlType *base);
int type_add_bound(KlType *ty, char *name, KlType *bound);
int type_add_trait(KlType *ty, KlType *trait);
int type_add_field(KlType *ty, char *name, TypeDesc *desc);
int type_add_kfunc(KlType *ty, char *name, TypeDesc *desc, KlCode *code);
int type_add_cfunc(KlType *ty, char *name, TypeDesc *desc, void *cfunc);
int type_add_ifunc(KlType *ty, char *name, TypeDesc *desc);
HashMap *type_get_mtbl(KlType *ty);
void type_ready(KlType *type);

#if !defined(NLOG)
void type_show(KlType *type);
#else
#define type_show(ty) ((void *)0)
#endif

// virtual table
struct _KlFuncTbl {
    KlType *type;
    int num;
    KlFunc **func;
};

// fat layout
struct _KlValue {
    union {
        void *obj;
        int8 i8val;
        int16 i16val;
        int32 i32val;
        int64 i64val;
        float f32val;
        double f64val;
        int32 cval;
        int8 bval;
    };
    KlFuncTbl *vtbl;
};

typedef struct _MethodDef MethodDef;

/* c method defined struct */
struct _MethodDef {
    char *name;
    char *ptype;
    char *rtype;
    void *cfunc;
};

void type_add_methdefs(KlType *ty, MethodDef *def);

/*----------------------------------------------------------------------------*/

typedef struct _KlString KlString;
typedef struct _KlArray KlArray;
typedef struct _KlWrapper KlWrapper;

struct _KlString {
    char **data;
    int cap;
    int start;
    int end;
};

struct _KlArray {
    KlWrapper *wrap;
    KlFuncTbl *vtbl;
    int start;
    int end;
};

struct _KlWrapper {
    char *data;
    int cap;
};

void mod_add_type(char *path, KlType *type);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_TYPE_H_ */

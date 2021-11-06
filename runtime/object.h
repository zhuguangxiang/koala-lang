/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_OBJECT_H_
#define _KOALA_OBJECT_H_

#include "gc.h"
#include "typedesc.h"
#include "util/hashmap.h"
#include "util/vector.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _TypeInfo TypeInfo;
typedef struct _VTable VTable;
typedef struct _ObjectMap ObjectMap;
typedef struct _MNode MNode;
typedef struct _KlFunc KlFunc;
typedef struct _KlCode KlCode;
typedef struct _KlField KlField;
typedef struct _KlProto KlProto;
typedef struct _KlType KlType;
typedef struct _KlVar KlVar;
typedef struct _KlValue KlValue;
typedef struct _KlValue KlNumber;
typedef struct _KlChar KlChar;
typedef struct _KlString KlString;
typedef struct _KlSlice KlSlice;
typedef struct _PkgInfo PkgInfo;
typedef struct _KlPackage KlPackage;
typedef struct _MethodDef MethodDef;

#define TP_CLASS 1
#define TP_TRAIT 2
#define TP_ENUM  4
#define TP_FINAL 8

#define TP_IS_CLASS(tp) ((tp)->flags & TP_CLASS)
#define TP_IS_TRAIT(tp) ((tp)->flags & TP_TRAIT)
#define TP_IS_ENUM(tp)  ((tp)->flags & TP_ENUM)
#define TP_IS_FINAL(tp) ((tp)->flags & TP_FINAL)

/* type info */
struct _TypeInfo {
    /* type's name */
    char *name;
    /* type's flag, one of TP_XXX  */
    uint8 flags;
    /* count vtbl */
    int num_vtbl;
    /* virtual table */
    VTable **vtbl;
    /* object map for gc */
    ObjectMap *obj_map;
    /* type parameters */
    Vector *params;
    /* self methods */
    Vector *methods;
    /* self fields */
    Vector *fields;
    /* first class/trait */
    TypeInfo *base;
    /* other traits */
    Vector *traits;
    /* line resolution order */
    Vector *lro;
    /* member's map */
    HashMap *mtbl;
};

/* virtual table */
struct _VTable {
    TypeInfo *type;
    int num;
    KlFunc *func[0];
};

struct _ObjectMap {
    int num;
    int offset[0];
};

/* clang-format off */
#define MNODE_HEAD HashMapEntry entry; char *name; TypeDesc *desc; int8 kind;
/* clang-format on */

#define MNODE_FIELD_KIND 1
#define MNODE_KFUNC_KIND 2
#define MNODE_CFUNC_KIND 3
#define MNODE_IFUNC_KIND 4
#define MNODE_TYPE_KIND  5
#define MNODE_VAR_KIND   6

struct _MNode {
    MNODE_HEAD
};

struct _KlField {
    MNODE_HEAD
    uint32 offset;
};

struct _KlFunc {
    MNODE_HEAD
    /* is inherit? */
    int8 inherit;
    /* main vtbl slot */
    int16 slot;
    /* owner: type or module */
    void *owner;
    /* cfunc or klcode */
    uintptr ptr;
};

struct _KlCode {
    /* local vars */
    Vector *locvec;
    /* closure, up vars */
    Vector *upvec;
    /* constants pool */
    void *consts;
    /* byte codes */
    int size;
    uint8 *codes;
};

struct _KlProto {
    MNODE_HEAD
    uint32 offset;
};

struct _KlType {
    MNODE_HEAD
    TypeInfo *type;
};

struct _KlVar {
    MNODE_HEAD
    uint32 offset;
};

int type_set_base(TypeInfo *type, TypeInfo *base);
int type_add_bound(TypeInfo *type, char *name, TypeInfo *bound);
int type_add_trait(TypeInfo *type, TypeInfo *trait);
int type_add_field(TypeInfo *ty, char *name, TypeDesc *desc);
int type_add_kfunc(TypeInfo *ty, char *name, TypeDesc *desc, KlCode *code);
int type_add_cfunc(TypeInfo *ty, char *name, TypeDesc *desc, void *cfunc);
int type_add_ifunc(TypeInfo *type, char *name, TypeDesc *desc);
void type_ready(TypeInfo *type);
#if !defined(NLOG)
void type_show(TypeInfo *type);
#else
#define type_show(ty) ((void *)0)
#endif

/* c method defined struct */
struct _MethodDef {
    char *name;
    char *ptype;
    char *rtype;
    void *cfunc;
};

void type_add_methdefs(TypeInfo *ty, MethodDef *def);

extern TypeInfo any_type;
extern TypeInfo int8_type;
extern TypeInfo int16_type;
extern TypeInfo int32_type;
extern TypeInfo int64_type;
extern TypeInfo float32_type;
extern TypeInfo float64_type;
extern TypeInfo bool_type;
extern TypeInfo char_type;
extern TypeInfo string_type;
extern TypeInfo array_type;
extern TypeInfo map_type;

extern TypeInfo class_type;
extern TypeInfo field_type;
extern TypeInfo method_type;

extern VTable *int8_vtbl;
extern VTable *int16_vtbl;
extern VTable *int32_vtbl;
extern VTable *int64_vtbl;
extern VTable *float32_vtbl;
extern VTable *float64_vtbl;
extern VTable *bool_vtbl;

struct _KlValue {
    uintptr value;
    VTable *vtbl;
};

struct _KlSlice {
    int index;
    int length;
    GcArray(char) gcarr;
};

struct _KlHeapValue {
    KlValue values[0];
};

struct _PkgInfo {
    HashMapEntry entry;
    char *path;
    HashMap mbrs;
    Vector vars;
    Vector funcs;
    Vector types;
};

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_OBJECT_H_ */

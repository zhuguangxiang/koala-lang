/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_OBJECT_H_
#define _KOALA_OBJECT_H_

#include "common.h"
#include "hashmap.h"
#include "queue.h"
#include "typespec.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _TypeObject;

/* clang-format off */
#define OBJECT_HEAD struct _TypeObject *ob_type;
/* clang-format on */

typedef struct _Object {
    OBJECT_HEAD
} Object;

#define OBJECT_HEAD_INIT(_type) .ob_type = (_type)

#define INIT_OBJECT_HEAD(_ob, _type) (_ob)->ob_type = (_type)

#define OB_TYPE(_ob) (((Object *)(_ob))->ob_type)

#define IS_TYPE(ob, type) (OB_TYPE(ob) == type)

typedef struct _Value {
    union {
        uintptr_t tag;
        void *vtbl;
    };
    union {
        int64_t ival;
        double fval;
        int bval;
        void *obj;
    };
} Value;

#define TAG_NONE  0 // none value
#define TAG_ERROR 1 // error value, used for exception handling
#define TAG_BOOL  2 // bool value

#define TAG_INT_START 3 // int value start, up to TAG_UINT64
#define TAG_INT8      3 // int8 value
#define TAG_INT16     4 // int16 value
#define TAG_INT32     5 // int32 value
#define TAG_INT64     6 // int64 value
#define TAG_UINT8     7 // uint8 value
#define TAG_UINT16    8 // uint16 value
#define TAG_UINT32    9 // uint32 value
#define TAG_UINT64    10 // uint64 value
#define TAG_INT_END   10 // int value end

#define TAG_FLOAT_START 11 // float value start, up to TAG_FLOAT64
#define TAG_BFLOAT16    11 // bfloat16 value
#define TAG_FLOAT16     12 // float16 value
#define TAG_FLOAT32     13 // float32 value
#define TAG_FLOAT64     14 // float64 value
#define TAG_FLOAT_END   14 // float value end

#define TAG_MAX 64 // beyond this value, it's an object pointer

#define is_none(x)     ((x)->tag == TAG_NONE)
#define is_error(x)    ((x)->tag == TAG_ERROR)
#define is_bool(x)     ((x)->tag == TAG_BOOL)
#define is_int8(x)     ((x)->tag == TAG_INT8)
#define is_int16(x)    ((x)->tag == TAG_INT16)
#define is_int32(x)    ((x)->tag == TAG_INT32)
#define is_int64(x)    ((x)->tag == TAG_INT64)
#define is_uint8(x)    ((x)->tag == TAG_UINT8)
#define is_uint16(x)   ((x)->tag == TAG_UINT16)
#define is_uint32(x)   ((x)->tag == TAG_UINT32)
#define is_uint64(x)   ((x)->tag == TAG_UINT64)
#define is_float16(x)  ((x)->tag == TAG_FLOAT16)
#define is_float32(x)  ((x)->tag == TAG_FLOAT32)
#define is_float64(x)  ((x)->tag == TAG_FLOAT64)
#define is_bfloat16(x) ((x)->tag == TAG_BFLOAT16)

#define is_int(x)   (((x)->tag >= TAG_INT_START) && ((x)->tag <= TAG_INT_END))
#define is_float(x) (((x)->tag >= TAG_FLOAT_START) && ((x)->tag <= TAG_FLOAT_END))

#define is_bfloat16(x) ((x)->tag == TAG_BFLOAT16)
#define is_value(x)    ((x)->tag < TAG_MAX)
#define is_obj(x)      ((x)->tag >= TAG_MAX)

/* clang-format off */
#define none_value          (Value){ .tag = TAG_NONE,   .ival = 0 }
#define error_value         (Value){ .tag = TAG_ERROR,  .ival = -1 }
#define bool_value(x)       (Value){ .tag = TAG_BOOL,   .bval = (int)(x) }
#define int8_value(x)       (Value){ .tag = TAG_INT8,    .ival = (int8_t)(x) }
#define int16_value(x)      (Value){ .tag = TAG_INT16,   .ival = (int16_t)(x) }
#define int32_value(x)      (Value){ .tag = TAG_INT32,   .ival = (int32_t)(x) }
#define int64_value(x)      (Value){ .tag = TAG_INT64,   .ival = (int64_t)(x) }
#define uint8_value(x)      (Value){ .tag = TAG_UINT8,   .ival = (uint8_t)(x) }
#define uint16_value(x)     (Value){ .tag = TAG_UINT16,  .ival = (uint16_t)(x) }
#define uint32_value(x)     (Value){ .tag = TAG_UINT32,  .ival = (uint32_t)(x) }
#define uint64_value(x)     (Value){ .tag = TAG_UINT64,  .ival = (uint64_t)(x) }
#define float16_value(x)    (Value){ .tag = TAG_FLOAT16,  .fval = (double)(x) }
#define float32_value(x)    (Value){ .tag = TAG_FLOAT32,  .fval = (double)(x) }
#define float64_value(x)    (Value){ .tag = TAG_FLOAT64,  .fval = (double)(x) }
#define bfloat16_value(x)   (Value){ .tag = TAG_BFLOAT16, .fval = (double)(x) }
#define obj_value(x)        (Value){ .vtbl = (x), .obj  = (x) }

#define to_bool(v)     ({ ASSERT(is_bool(v)); (v)->bval; })
#define to_int8(v)     ({ ASSERT(is_int8(v)); (v)->ival; })
#define to_int16(v)    ({ ASSERT(is_int16(v)); (v)->ival; })
#define to_int32(v)    ({ ASSERT(is_int32(v)); (v)->ival; })
#define to_int64(v)    ({ ASSERT(is_int64(v)); (v)->ival; })
#define to_uint8(v)    ({ ASSERT(is_uint8(v)); (v)->ival; })
#define to_uint16(v)   ({ ASSERT(is_uint16(v)); (v)->ival; })
#define to_uint32(v)   ({ ASSERT(is_uint32(v)); (v)->ival; })
#define to_uint64(v)   ({ ASSERT(is_uint64(v)); (v)->ival; })
#define to_float16(v)  ({ ASSERT(is_float16(v)); (v)->fval; })
#define to_float32(v)  ({ ASSERT(is_float32(v)); (v)->fval; })
#define to_float64(v)  ({ ASSERT(is_float64(v)); (v)->fval; })
#define to_bfloat16(v) ({ ASSERT(is_bfloat16(v)); (v)->fval; })
#define to_obj(v)      ({ ASSERT(is_obj(v)); (v)->obj; })

/* clang-format on */

typedef void (*MarkFunc)(Object *, Queue *);

typedef Value (*HashFunc)(Value *self);
typedef Value (*RichCmpFunc)(Value *lhs, Value *rhs, int op);
typedef Value (*StrFunc)(Value *self);

typedef Value (*GetIterFunc)(Object *self);
typedef Value (*IterNextFunc)(Object *self);

typedef Object *(*AllocFunc)(struct _TypeObject *tp);
typedef int (*InitFunc)(Value *self, Value *args, int nargs, Object *names);
typedef void (*FiniFunc)(Object *self);

typedef Value (*CallFunc)(Value *self, Value *args, int nargs, Object *names);

typedef Value (*CFuncNoArgs)(Value *);
typedef Value (*CFuncOneArg)(Value *, Value *);
typedef Value (*CFuncVarArgs)(Value *, Value *, int);
typedef Value (*CFuncVarArgsNames)(Value *, Value *, int, Object *);

typedef struct _MemberDef {
    /* The name of field/global */
    const char *name;
    /* type */
    TypeSpec *ts;
    /* offset */
    int offset;
} MemberDef;

typedef struct _MethodDef {
    /* The name of function/method */
    char *name;
    /* The C function */
    void *cfunc;
    /* flags */
    int flags;
} MethodDef;

/* Value fn(Value *self) */
#define METH_NO_ARGS 1
/* Value fn(Value *self, Value *oth) */
#define METH_ONE_ARG 2
/* Value fn(Value *self, Value *args, int nargs) */
#define METH_VAR_ARGS 3
/* Value fn(Value *self, Value *args, int nargs, Object *names) */
#define METH_VAR_NAMES 4

typedef struct _BaseDef {
    struct _TypeObject *tp;
} BaseDef;

/*
typedef struct _NumberMethods {
    BinaryFunc nb_add;
    BinaryFunc nb_sub;
    BinaryFunc nb_mul;
    BinaryFunc nb_div;
    BinaryFunc nb_mod;

    BinaryFunc nb_and;
    BinaryFunc nb_or;
    BinaryFunc nb_xor;

    BinaryFunc nb_shl;
    BinaryFunc nb_shr;
    BinaryFunc nb_ushr;
} NumberMethods;

typedef struct _SeqMapMethods {
    LenFunc sm_len;
    BinaryFunc sm_add;
    ContainFunc sm_contain;
    RepeatFunc sm_repeat;
    BinaryFunc sm_get_item;
    BinaryFunc sm_gets;
    BinaryFunc sm_sets;
} SeqMapMethods;
*/

typedef struct _VTable {
    struct _TypeObject *type;
    Vector methods;
} VTable;

#define TP_FLAGS_CLASS    (1 << 0)
#define TP_FLAGS_TRAIT    (1 << 1)
#define TP_FLAGS_HEAP     (1 << 2)
#define TP_FLAGS_READY    (1 << 3)
#define TP_FLAGS_READYING (1 << 4)

typedef struct _TypeObject {
    OBJECT_HEAD
    /* type name */
    char *name;
    /* TP_FLAGS_XXX */
    int flags;

    /* owner */
    Object *module;

    /* gc mark */
    MarkFunc mark;

    /* allocate function */
    AllocFunc alloc;
    /* init function */
    InitFunc init;
    /* fini function */
    FiniFunc fini;
    /* call function */
    CallFunc call;

    /* hashable */
    HashFunc hash;
    /* 0:false, 1: true */
    RichCmpFunc cmp;
    /* printable */
    StrFunc str;

    /* traits */
    BaseDef *basedefs;
    /* members */
    MemberDef *members;
    /* method defs */
    MethodDef *methdefs;

    /* fields */
    Vector fields;
    /* methods */
    Vector methods;
    /* field/method mapping */
    HashMap map;

    /* self vtable */
    VTable *vtbl;
    /* vtables */
    Vector vtables;
    /* vtable mapping */
    HashMap vtable_map;

    /* traits */
    Vector bases;
    /* primary inheritance path */
    Vector pip;
    /* linear resolved order */
    Vector lro;
    /* second chain map */
    Vector scm;
} TypeObject;

#define FUNCTION_HEAD \
    OBJECT_HEAD \
    Object *module; \
    TypeObject *cls; \
    int ready;

typedef struct _FuncObject {
    FUNCTION_HEAD
} FuncObject;

extern TypeObject type_type;
extern TypeObject any_type;
extern TypeObject bool_type;
extern TypeObject none_type;
extern TypeObject int8_type;
extern TypeObject int16_type;
extern TypeObject int32_type;
extern TypeObject int64_type;
extern TypeObject uint8_type;
extern TypeObject uint16_type;
extern TypeObject uint32_type;
extern TypeObject uint64_type;
#define int_type int64_type
extern TypeObject float_type;
extern TypeObject Iterable_type;
extern TypeObject Iterator_type;
extern TypeObject Collection_type;
extern TypeObject Sequence_type;
extern TypeObject MutableSequence_type;
extern TypeObject Number_type;

TypeObject *object_typeof(Value *val);

static inline CallFunc object_callable(Value *val)
{
    TypeObject *tp = object_typeof(val);
    if (!tp) return NULL;
    return tp->call;
}

Value object_call(Value *self, Value *args, int nargs, Object *names);
Value object_tostr(Value *self);
Object *object_lookup(Value *obj, char *name);

int type_ready(TypeObject *tp);

void init_sym_tbl(HashMap *map);
Object *sym_tbl_find(HashMap *map, char *name, int len);
void sym_tbl_add(HashMap *map, char *name, int len, Object *obj);

Value intf_not_impl(Value *self);
Value intf_not_impl_arg(Value *self, Value *arg);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_OBJECT_H_ */

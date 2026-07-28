/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_OBJECT_H_
#define _KOALA_OBJECT_H_

#include "codespec.h"
#include "common.h"
#include "hashmap.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------+
 |  Object & TValue                                                          |
 +---------------------------------------------------------------------------*/

#define OBJECT_HEAD struct _TypeObject *_type;

typedef struct _Object {
    OBJECT_HEAD
} Object;

/* Assigns the type of a newly allocated object. */
#define INIT_OBJECT_HEAD(ob, type) (ob)->_type = (type)

/** Retrieve the TypeObject of any Koala object. */
#define OB_TYPE(ob) ((ob)->_type)

/** Check whether an object is of a specific type. */
#define IS_TYPE(ob, type) (OB_TYPE(ob) == type)

typedef struct _TValue {
    union {
        uintptr_t tag; // primitive tag OR reference marker
        void *itab;    // itable for traits
    };
    union {
        int64_t ival; // integer payload
        double fval;  // floating‑point payload
        int bval;     // boolean payload
        Object *obj;  // heap object pointer
    };
} TValue;

/*---------------------------------------------------------------------------+
 |   Tag Constants                                                           |
 +---------------------------------------------------------------------------*/

#define TAG_NONE  0
#define TAG_ERROR 1
#define TAG_BOOL  2

#define TAG_INT8   0b1000
#define TAG_INT16  0b1001
#define TAG_INT32  0b1010
#define TAG_INT64  0b1011
#define TAG_UINT8  0b1100
#define TAG_UINT16 0b1101
#define TAG_UINT32 0b1110
#define TAG_UINT64 0b1111

#define TAG_FLOAT16 0b010001
#define TAG_FLOAT32 0b010010
#define TAG_FLOAT64 0b010011

// beyond this value, it's an object pointer
#define TAG_VAL_MAX 64
#define TAG_OBJECT  (TAG_VAL_MAX + 1)

/*---------------------------------------------------------------------------+
 |   Type Checking                                                           |
 +---------------------------------------------------------------------------*/

#define is_none(x)  ((x)->tag == TAG_NONE)
#define is_error(x) ((x)->tag == TAG_ERROR)
#define is_bool(x)  ((x)->tag == TAG_BOOL)

/* Signed integers */
#define is_int8(x)  ((x)->tag == TAG_INT8)
#define is_int16(x) ((x)->tag == TAG_INT16)
#define is_int32(x) ((x)->tag == TAG_INT32)
#define is_int64(x) ((x)->tag == TAG_INT64)

/* Unsigned integers */
#define is_uint8(x)  ((x)->tag == TAG_UINT8)
#define is_uint16(x) ((x)->tag == TAG_UINT16)
#define is_uint32(x) ((x)->tag == TAG_UINT32)
#define is_uint64(x) ((x)->tag == TAG_UINT64)

/* Floating point */
#define is_float16(x) ((x)->tag == TAG_FLOAT16)
#define is_float32(x) ((x)->tag == TAG_FLOAT32)
#define is_float64(x) ((x)->tag == TAG_FLOAT64)
// #define is_bfloat16(x) ((x)->tag == TAG_BFLOAT16)

/* Category checks */
#define is_int(x)   (((x)->tag & 0b1100) == 0b1000)
#define is_uint(x)  (((x)->tag & 0b1100) == 0b1100)
#define is_float(x) (((x)->tag >= TAG_FLOAT16) && ((x)->tag <= TAG_FLOAT64))

/* Primitive vs reference */
#define is_val(x)  ((x)->tag < TAG_VAL_MAX)
#define is_intf(x) ((x)->tag > TAG_OBJECT)
#define is_ref(x)  ((x)->tag >= TAG_OBJECT)

/*---------------------------------------------------------------------------+
 |   Value Construction                                                      |
 +---------------------------------------------------------------------------*/

/* clang-format off */
#define none_value          (TValue){ .tag = TAG_NONE,   .ival = 0 }
#define error_value         (TValue){ .tag = TAG_ERROR,  .ival = -1 }
#define bool_value(x)       (TValue){ .tag = TAG_BOOL,   .bval = (int)(x) }

/* Signed integers */
#define int8_value(x)       (TValue){ .tag = TAG_INT8,    .ival = (int8_t)(x) }
#define int16_value(x)      (TValue){ .tag = TAG_INT16,   .ival = (int16_t)(x) }
#define int32_value(x)      (TValue){ .tag = TAG_INT32,   .ival = (int32_t)(x) }
#define int64_value(x)      (TValue){ .tag = TAG_INT64,   .ival = (int64_t)(x) }

/* Unsigned integers */
#define uint8_value(x)      (TValue){ .tag = TAG_UINT8,   .ival = (uint8_t)(x) }
#define uint16_value(x)     (TValue){ .tag = TAG_UINT16,  .ival = (uint16_t)(x) }
#define uint32_value(x)     (TValue){ .tag = TAG_UINT32,  .ival = (uint32_t)(x) }
#define uint64_value(x)     (TValue){ .tag = TAG_UINT64,  .ival = (uint64_t)(x) }

/* Floating point (TEMP: stored as double; real impl should preserve bit pattern) */
#define float16_value(x)    (TValue){ .tag = TAG_FLOAT16,  .fval = (double)(x) }
#define float32_value(x)    (TValue){ .tag = TAG_FLOAT32,  .fval = (double)(x) }
#define float64_value(x)    (TValue){ .tag = TAG_FLOAT64,  .fval = (double)(x) }
// #define bfloat16_value(x)   (TValue){ .tag = TAG_BFLOAT16, .fval = (double)(x) }

/* Reference value */
#define obj_value(x)         (TValue){ .tag = TAG_OBJECT, .obj = (x) }
#define intf_value(_itab, x) (TValue){ .itab = (_itab), .obj = (x) }

/*---------------------------------------------------------------------------+
 |   Value Extraction                                                        |
 +---------------------------------------------------------------------------*/

#define to_bool(v)     ({ ASSERT(is_bool(v)); (v)->bval; })

/* Signed integers */
#define to_int8(v)     ({ ASSERT(is_int8(v)); (int8_t)(v)->ival; })
#define to_int16(v)    ({ ASSERT(is_int16(v)); (int16_t)(v)->ival; })
#define to_int32(v)    ({ ASSERT(is_int32(v)); (int32_t)(v)->ival; })
#define to_int64(v)    ({ ASSERT(is_int64(v)); (int64_t)(v)->ival; })

/* Unsigned integers */
#define to_uint8(v)    ({ ASSERT(is_uint8(v)); (uint8_t)(v)->ival; })
#define to_uint16(v)   ({ ASSERT(is_uint16(v)); (uint16_t)(v)->ival; })
#define to_uint32(v)   ({ ASSERT(is_uint32(v)); (uint32_t)(v)->ival; })
#define to_uint64(v)   ({ ASSERT(is_uint64(v)); (uint64_t)(v)->ival; })

/* Floating point (TEMP: stored as double; real impl should preserve bit pattern) */
#define to_float16(v)  ({ ASSERT(is_float16(v)); (v)->fval; })
#define to_float32(v)  ({ ASSERT(is_float32(v)); (v)->fval; })
#define to_float64(v)  ({ ASSERT(is_float64(v)); (v)->fval; })
// #define to_bfloat16(v) ({ ASSERT(is_bfloat16(v)); (v)->fval; })

/* Reference */
#define to_obj(v)      ({ ASSERT(is_ref(v)); (v)->obj; })

/* clang-format on */

/*---------------------------------------------------------------------------+
 |  Koala Instance Object Layout                                             |
 +---------------------------------------------------------------------------*/

/* clang-format off */
#define INST_OBJECT_HEAD OBJECT_HEAD size_t size;
/* clang-format on */

typedef struct _InstObject {
    INST_OBJECT_HEAD
    TValue fields[0];
} InstObject;

Object *kl_new_instance(struct _TypeObject *tp);

/*---------------------------------------------------------------------------+
 |  Type Object                                                              |
 +---------------------------------------------------------------------------*/

typedef struct _MemberDef {
    /* The name of field/global */
    char *name;
    /* type */
    int type;
    /* offset */
    int offset;
} MemberDef;

#define M_TYPE_INT 0
#define M_TYPE_STR 1
#define M_TYPE_OBJ 2

#define M_OFFSET(tp, m) offsetof(tp, m)

typedef TValue (*NativeFunc)(TValue *self, TValue *args, int nargs);

typedef struct _MethodDef {
    /* The name of func/method */
    char *name;
    /* The C function */
    NativeFunc cfunc;
} MethodDef;

typedef Object *(*AllocFunc)(struct _TypeObject *tp);
typedef int (*InitFunc)(TValue *self, TValue *args, int nargs);
typedef void (*FiniFunc)(Object *self);

typedef unsigned int (*HashFunc)(TValue *self);
typedef TValue (*RichCmpFunc)(TValue *lhs, TValue *rhs, int op);
typedef TValue (*StrFunc)(TValue *self);
typedef TValue (*CallFunc)(TValue *self, TValue *args, int nargs);

typedef size_t (*LenFunc)(TValue *self);
typedef int (*ContainsFunc)(TValue *self, TValue *item);
typedef TValue (*GetItemFunc)(TValue *self, size_t index);
typedef void (*SetItemFunc)(TValue *self, size_t index, TValue *value);
typedef TValue (*GetSubFunc)(TValue *self, TValue *key);
typedef void (*SetSubFunc)(TValue *self, TValue *key, TValue *value);

typedef struct _SeqMethods {
    /* sequence length */
    LenFunc len;
    /* sequence contains */
    ContainsFunc contains;
    /* sequence item getter */
    GetItemFunc get;
    /* sequence item setter */
    SetItemFunc set;
} SeqMethods;

typedef struct _MapMethods {
    /* mapping length */
    LenFunc len;
    /* mapping contains */
    ContainsFunc contains;
    /* mapping item getter */
    GetSubFunc get;
    /* mapping item setter */
    SetSubFunc set;
} MapMethods;

typedef enum {
    SLOT_HASH,
    SLOT_EQ,
    SLOT_NE,
    SLOT_LT,
    SLOT_LE,
    SLOT_GT,
    SLOT_GE,
    SLOT_STR,
    SLOT_CALL,
    SLOT_MAX
} SlotId;

typedef struct _IntfTable {
    char *name;
    int num_funcs;
    int num_parents;
    Object **methods;
    struct _IntfTable **parents;
} IntfTable;

Object *kl_get_intf_func(TValue *intf, int func_idx);

#define TP_FLAGS_CLASS  (1 << 0)
#define TP_FLAGS_TRAIT  (1 << 1)
#define TP_FLAGS_PUBLIC (1 << 2)
#define TP_FLAGS_READY  (1 << 3)

typedef struct _TypeObject {
    OBJECT_HEAD

    /* Interface tables */
    Vector itables;

    /* Type name */
    char *name;

    /* TP_FLAGS_XXX */
    int flags;
    /* number of base types */
    int nbases;
    /* parent traits */
    struct _TypeObject **bases;
    /* fields */
    Vector fields;
    /* methods */
    Vector methods;
    /* fields and methods */
    HashMap members;
    /* module */
    Object *module;

    /* methoddef */
    MethodDef *methdefs;
    /* memberdef */
    MemberDef *membdefs;

    /* allocate function */
    AllocFunc alloc;
    /* init function */
    InitFunc init;
    /* fini function */
    FiniFunc fini;

    /* mapping protocol methods */
    MapMethods *map;

    /* sequence protocol methods */
    SeqMethods *seq;

    /* for fast access in c extension */

    /* hash function(__hash__) */
    HashFunc hash;
    /* comparison function(__eq__, __lt__, etc.) */
    RichCmpFunc cmp;
    /* printable (__str__) */
    StrFunc str;
    /* call function(__call__) */
    CallFunc call;

    /* slots for special methods */
    Object *slots[SLOT_MAX];
} TypeObject;

/*---------------------------------------------------------------------------+
 |  Field Object                                                             |
 +---------------------------------------------------------------------------*/

typedef enum {
    FIELD_OFFSET, // C struct offset
    FIELD_INDEX   // Koala class field index
} FieldKind;

typedef struct _FieldObject {
    OBJECT_HEAD
    char *name;
    Object *owner;
    FieldKind kind;
    int type;
    int index;
} FieldObject;

extern TypeObject field_type;
Object *kl_new_field(char *name, int type, int offset, Object *owner);

static inline Object *kl_new_index_field(char *name, int type, int index, Object *owner)
{
    Object *obj = kl_new_field(name, type, index, owner);
    ((FieldObject *)obj)->kind = FIELD_INDEX;
    return obj;
}

extern TypeObject global_type;
Object *kl_new_global(char *name, int index, Object *m);

typedef struct _GlobalObject {
    OBJECT_HEAD
    char *name;
    Object *module;
    int index;
} GlobalObject;

/*---------------------------------------------------------------------------+
 |  CFunc&Code Object                                                        |
 +---------------------------------------------------------------------------*/

typedef struct _CFuncObject {
    OBJECT_HEAD
    Object *owner;
    NativeFunc func;
    int func_idx;
    char *name;
} CFuncObject;

typedef struct _CodeObject {
    OBJECT_HEAD
    Object *owner;
    int flags;
#define CODE_FLAG_PUB  (1 << 0)
#define CODE_FLAG_METH (1 << 1)
    int func_idx;
    CodeSpec cs;
} CodeObject;

extern TypeObject cfunc_type;
extern TypeObject code_type;

#define IS_CFUNC(ob) IS_TYPE((ob), &cfunc_type)
#define IS_CODE(ob)  IS_TYPE((ob), &code_type)

Object *kl_new_code(char *name, Object *owner);
Object *kl_new_cfunc(char *name, NativeFunc fn, Object *owner);

static inline void kl_set_func_idx(Object *obj, int func_idx)
{
    if (IS_CODE(obj)) {
        ((CodeObject *)obj)->func_idx = func_idx;
    } else {
        ASSERT(IS_CFUNC(obj));
        ((CFuncObject *)obj)->func_idx = func_idx;
    }
}

/*---------------------------------------------------------------------------+
 |  Bool related                                                             |
 +---------------------------------------------------------------------------*/

/* Rich comparison opcodes */
#define CMP_EQ 0
#define CMP_NE 1
#define CMP_LT 2
#define CMP_LE 3
#define CMP_GT 4
#define CMP_GE 5

#define BOOL_TRUE  bool_value(1)
#define BOOL_FALSE bool_value(0)

#define RETURN_TRUE  return BOOL_TRUE
#define RETURN_FALSE return BOOL_FALSE

/*
 * Macro for implementing rich comparisons
 *
 * C-comparison to Koala's rich comparison
 */

// clang-format off

#define RETURN_RICHCOMPARE(val, op) do {                     \
    switch (op) {                                            \
    case CMP_EQ: if ((val) == 0) RETURN_TRUE; RETURN_FALSE;  \
    case CMP_NE: if ((val) != 0) RETURN_TRUE; RETURN_FALSE;  \
    case CMP_LT: if ((val) < 0) RETURN_TRUE; RETURN_FALSE;   \
    case CMP_GT: if ((val) > 0) RETURN_TRUE; RETURN_FALSE;   \
    case CMP_LE: if ((val) <= 0) RETURN_TRUE; RETURN_FALSE;  \
    case CMP_GE: if ((val) >= 0) RETURN_TRUE; RETURN_FALSE;  \
    default:                                                 \
        UNREACHABLE();                                       \
    }                                                        \
} while (0)

// clang-format on

/*---------------------------------------------------------------------------+
 |  String related                                                           |
 +---------------------------------------------------------------------------*/

typedef struct _StringObject {
    OBJECT_HEAD
    size_t size;
    char *array;
} StringObject;

#define IS_STR(ob) IS_TYPE((ob), &str_type)

#define STR_BUF(ob) (((StringObject *)(ob))->array)
#define STR_LEN(ob) (((StringObject *)(ob))->size)

Object *kl_new_nstr(char *s, size_t len);
static inline Object *kl_new_str(char *s) { return kl_new_nstr(s, strlen(s)); }
Object *kl_new_fmt_str(char *fmt, ...);
void kl_free_str(Object *obj);

/*---------------------------------------------------------------------------+
 |  APIs of Object, TValue, TypeObject & ModuleObject                        |
 +---------------------------------------------------------------------------*/

void stbl_init(HashMap *map);
void stbl_add_obj(HashMap *map, char *name, Object *obj);
Object *stbl_find_obj(HashMap *map, char *name);

extern TypeObject any_type;
extern TypeObject type_type;
extern TypeObject none_type;
extern TypeObject bool_type;
extern TypeObject str_type;
extern TypeObject exc_type;
// extern TypeObject field_type;
// shared by all int/uint types
extern TypeObject int_type;
// shared by all float types
extern TypeObject float_type;

extern TypeObject Iterable_type;
extern TypeObject Iterator_type;
extern TypeObject Collection_type;
extern TypeObject Sequence_type;
extern TypeObject MutableSequence_type;

TypeObject *kl_typeof(TValue *val);
int kl_init_type(TypeObject *tp);
TypeObject *kl_new_type(char *name, int flags);

static inline Object *kl_to_str(TValue *val)
{
    TypeObject *tp = kl_typeof(val);
    if (tp->str) {
        TValue s = tp->str(val);
        return to_obj(&s);
    }

    return kl_new_fmt_str("<%s object at %p>", tp->name, val->obj);
}

Object *kl_type_find(TypeObject *tp, char *name);

/* Any object is callable, if it implements the call protocol. */
static inline TValue kl_do_call(TValue *callable, TValue *args, int nargs)
{
    TypeObject *tp = kl_typeof(callable);
    CallFunc call = tp->call;
    ASSERT(call != NULL);
    return call(callable, args, nargs);
}

static inline TValue kl_do_call_no_arg(TValue *callable) { return kl_do_call(callable, NULL, 0); }

static inline TValue kl_do_call_one_arg(TValue *callable, TValue *arg)
{
    return kl_do_call(callable, arg, 1);
}

static inline TValue kl_object_call(Object *callable, TValue *args, int nargs)
{
    TValue _call = obj_value(callable);
    return kl_do_call(&_call, args, nargs);
}

void kl_init_gm_stbl(void);
Object *kl_load_module(char *path);
Object *kl_get_module(char *path);
int kl_register_module(Object *m);
void kl_resolve_import(Object *m);
void kl_dump_module(Object *m);

/*---------------------------------------------------------------------------+
 |   Argument Helpers — extract typed arguments from args[]                  |
 +---------------------------------------------------------------------------*/

#define kl_arg_obj(index) \
    ({ \
        ASSERT(index >= 0 && index < nargs); \
        to_obj(args + index); \
    })

#define kl_arg_uint8(index) \
    ({ \
        ASSERT(index >= 0 && index < nargs); \
        to_uint8(args + index); \
    })

#define kl_arg_int64(index) \
    ({ \
        ASSERT(index >= 0 && index < nargs); \
        to_int64(args + index); \
    })

#define kl_arg_float(index) \
    ({ \
        ASSERT(index >= 0 && index < nargs); \
        to_float64(args + index); \
    })

#define kl_arg_bool(index) \
    ({ \
        ASSERT(index >= 0 && index < nargs); \
        to_bool(args + index); \
    })

#define kl_arg_str(index) \
    ({ \
        ASSERT(index >= 0 && index < nargs); \
        Object *o = to_obj(args + index); \
        ASSERT(IS_STR(o)); \
        STR_BUF(o); \
    })

static inline TValue kl_val_str(char *s)
{
    Object *so = kl_new_str(s);
    return obj_value(so);
}

static inline TValue kl_val_nstr(char *s, size_t len)
{
    Object *so = kl_new_nstr(s, len);
    return obj_value(so);
}

#define SELF_AS(tp_type) \
    ({ \
        Object *o = to_obj(self); \
        ASSERT(IS_TYPE(o, &tp_type)); \
        (void *)o; \
    })

/*---------------------------------------------------------------------------+
 |  Native Library API                                                       |
 +---------------------------------------------------------------------------*/

Object *kl_get_native(Object *m, char *name);

typedef struct _NativeLib {
    char *name;
    void *handle;
    HashMap symbols;
} NativeLib;

int kl_reg_func(NativeLib *lib, char *name, NativeFunc fn);
int kl_reg_meth(NativeLib *lib, char *cls, char *meth, NativeFunc fn);
int kl_reg_type(NativeLib *lib, TypeObject *tp);

/*---------------------------------------------------------------------------+
 |  Eval & Run                                                               |
 +---------------------------------------------------------------------------*/

TValue kl_eval_code(TValue *self, TValue *args, int nargs);
void kl_run_main(Object *m);
void kl_run_init(Object *m);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_OBJECT_H_ */

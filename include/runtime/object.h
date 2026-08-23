/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_OBJECT_H_
#define _KOALA_OBJECT_H_

#include "buffer.h"
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

#define OBJECT_HEAD \
    /* object's meta type object */ \
    struct _TypeObject *_type; \
    /* Size of the Koala-visible fields (dynamic size) */ \
    size_t _size;

typedef struct _Object {
    OBJECT_HEAD
} Object;

/* Assigns the type of a newly allocated object. */
#define INIT_OBJECT_HEAD(ob, type, size) \
    (ob)->_type = (type); \
    (ob)->_size = (size);

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
        int64_t ival; // integer/bool payload
        double fval;  // floating‑point payload
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
#define is_val(x) ((x)->tag < TAG_VAL_MAX)

// is_intf means it's an intf-table and value canbe any(primitive or object)
#define is_intf(x) ((x)->tag > TAG_OBJECT)

// is_ref means it's an object pointer
#define is_ref(x) ((x)->tag == TAG_OBJECT)

/*---------------------------------------------------------------------------+
 |   Value Construction                                                      |
 +---------------------------------------------------------------------------*/

/* clang-format off */
#define none_value          (TValue){ .tag = TAG_NONE,   .ival = 0 }
#define error_value         (TValue){ .tag = TAG_ERROR,  .ival = -1 }
#define bool_value(x)       (TValue){ .tag = TAG_BOOL,   .ival = (int)(x) }

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
#define obj_value(x)         (TValue){ .tag = TAG_OBJECT, .obj = (Object *)(x) }
#define intf_value(_itab, x) (TValue){ .itab = (_itab), .obj = (Object *)(x) }

/*---------------------------------------------------------------------------+
 |   Value Extraction                                                        |
 +---------------------------------------------------------------------------*/

#define to_bool(v)     ({ ASSERT(is_bool(v)); (v)->ival; })

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
// #define to_obj(v)      ({ ASSERT(is_ref(v)); (v)->obj; })

/* clang-format on */

/*---------------------------------------------------------------------------+
 |  Koala Instance Object Layout                                             |
 +---------------------------------------------------------------------------*/

typedef struct _InstObject {
    OBJECT_HEAD
    TValue fields[0];
} InstObject;

Object *kl_new_instance(struct _TypeObject *tp);
#define NR_FIELDS(obj) ((obj)->_size)

/*---------------------------------------------------------------------------+
 |  Type Object                                                              |
 +---------------------------------------------------------------------------*/

typedef TValue (*NativeFunc)(TValue *self, TValue *args, int nargs);

typedef struct _MethodDef {
    /* The name of func/method */
    char *name;
    /* The C function */
    NativeFunc cfunc;
} MethodDef;

typedef void (*GcMarkFunc)(Object *self);
typedef int (*InitFunc)(TValue *self, TValue *args, int nargs);
typedef void (*FiniFunc)(Object *self);
typedef TValue (*CallFunc)(TValue *self, TValue *args, int nargs);

typedef enum {
    /* hot slots -- dict/loop hot paths, all within the first cache line of TypeObject */

    /* comparison protocol */
    SLOT_EQ, // OP_NUM_EQ
    SLOT_NE, // OP_NUM_NE
    SLOT_LT, // OP_NUM_LT
    SLOT_LE, // OP_NUM_LE
    SLOT_GT, // OP_NUM_GT
    SLOT_GE, // OP_NUM_GE

    /* hashable protocol -- hit on every dict/set probe */
    SLOT_HASH,

    /* warm slots -- second cache line */

    /* sequence protocol */
    SLOT_LEN,      // OP_SEQ_LEN
    SLOT_GET_ITEM, // OP_SEQ_GET and OP_SEQ_GET_IMM
    SLOT_SET_ITEM, // OP_SEQ_SET and OP_SEQ_SET_IMM
    SLOT_CONTAINS, // shared by sequence and mapping protocols, the 'in' OP

    /* slice protocol */
    SLOT_GET_SLICE, /* __getslice__ */
    SLOT_SET_SLICE, /* __setslice__ */

    /* mapping subscript protocol */
    SLOT_GET_SUB, // __getsub__
    SLOT_SET_SUB, // __setsub__

    /* cold slots */

    /* printable protocol  */
    SLOT_STR, // __str__ -- print path

    /* arithmetic protocol */
    SLOT_ADD, // OP_NUM_ADD
    SLOT_SUB, // OP_NUM_SUB
    SLOT_MUL, // OP_NUM_MUL
    SLOT_DIV, // OP_NUM_DIV
    SLOT_MOD, // OP_NUM_MOD
    SLOT_NEG, // unary minus

    /* bitwise protocol */
    SLOT_SHL,     // OP_NUM_SHL
    SLOT_SHR,     // OP_NUM_SHR
    SLOT_BIT_AND, // OP_NUM_AND
    SLOT_BIT_OR,  // OP_NUM_OR
    SLOT_BIT_XOR, // OP_NUM_XOR
    SLOT_BIT_NOT, // bitwise NOT

    /* other slots */

    SLOT_MAX
} SlotId;

Object *kl_get_intf_func(TValue *intf, int func_idx);

#define TP_FLAGS_VALUE (1 << 0)
#define TP_FLAGS_CLASS (1 << 1)
#define TP_FLAGS_READY (1 << 2)

/**
 * Object memory layout:
 * +-----------------------------------+
 * | TypeObject *type                  |  <- Object Header (sizeof(BaseObject))
 * +-----------------------------------+
 * | size_t _size                       |  <- Size of the Koala-visible fields (dynamic size)
 * +-----------------------------------+
 * | TValue fields[num_fields]         |  <- Koala-visible fields (dynamic size)
 * +-----------------------------------+
 * | char priv_data[priv_size]         |  <- C-private opaque payload (dynamic size)
 * +-----------------------------------+
 *
 * NOTE: The 'priv_data' region is entirely invisible to Koala bytecode.
 *       It is managed exclusively by the C native runtime side.
 */

typedef struct _TypeObject {
    OBJECT_HEAD

    /* Interface tables */
    Vector itables;
    /* slots for special methods */
    Object *slots[SLOT_MAX];

    /* Type name */
    char *name;
    /* size of the C-private opaque payload appended to each object */
    int priv_size;
    /* TP_FLAGS_XXX */
    int flags;
    /* gc mark */
    GcMarkFunc gc_mark;

    /* init function */
    InitFunc init;
    /* fini function */
    FiniFunc fini;
    /* callable (__call__) */
    CallFunc call;

    /* fields */
    Vector fields;
    /* methods */
    Vector methods;
    /* fields and methods */
    HashMap members;
    /* module */
    Object *module;
    /* methoddefs */
    MethodDef *methdefs;
} TypeObject;

typedef struct _IntfTable {
    char *name;
    int num_funcs;
    int num_parents;
    Object **methods;
    struct _IntfTable **parents;
    TypeObject *tp;
} IntfTable;

#define tp_is_ref_type(tp) (!((tp)->flags & TP_FLAGS_VALUE))

// extract obj from an intf value; only valid when impl is a reference type
#define intf_to_obj(v) \
    ({ \
        ASSERT(is_intf(v)); \
        IntfTable *_itab = (IntfTable *)(v)->itab; \
        ASSERT(_itab && tp_is_ref_type(_itab->tp)); \
        (v)->obj; \
    })

static inline Object *to_obj(TValue *v)
{
    if (is_ref(v)) return v->obj;
    return intf_to_obj(v);
}

#define DEFINE_TYPE(_name, _flags, _priv_size, _methods, _gc_mark) \
    TypeObject _name##_type = { \
        ._type = &type_type, \
        .name = #_name, \
        .flags = (_flags), \
        .priv_size = (_priv_size), \
        .methdefs = (_methods), \
        .gc_mark = (_gc_mark), \
    }

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
#define IS_GLOBAL(ob) IS_TYPE((ob), &global_type)
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

TypeObject *kl_typeof(TValue *val);
void kl_init_type(TypeObject *tp);
TypeObject *kl_new_type(char *name, int flags);
int kl_tp_add_field(TypeObject *tp, char *name, Object *field);
int kl_tp_add_method(TypeObject *tp, char *name, Object *meth);
void kl_tp_install_slots(TypeObject *tp);

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

#define kl_arg_obj_as(index, tp_type) \
    ({ \
        ASSERT(index >= 0 && index < nargs); \
        Object *o = to_obj(args + index); \
        ASSERT(IS_TYPE(o, &tp_type)); \
        (void *)o; \
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

#define kl_arg_float64(index) \
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

#define kl_arg_strobj(index) \
    ({ \
        ASSERT(index >= 0 && index < nargs); \
        Object *o = to_obj(args + index); \
        ASSERT(IS_STR(o)); \
        (void *)o; \
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

// string from buffer and free the buffer
#define kl_val_str_from_buf(buf) \
    ({ \
        TValue v = kl_val_nstr(BUF_STR(buf), BUF_LEN(buf)); \
        FINI_BUF(buf); \
        v; \
    })

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
void kl_panic(char *msg);

/*---------------------------------------------------------------------------+
 |  Slot Call                                                                |
 +---------------------------------------------------------------------------*/

static inline TValue kl_slot_call_no_arg(TValue *self, int slotid)
{
    TypeObject *tp = kl_typeof(self);
    ASSERT(tp);
    Object *fn = tp->slots[slotid];
    ASSERT(fn);

    TValue ret;

    if (IS_CFUNC(fn)) {
        CFuncObject *cfn = (CFuncObject *)fn;
        ret = cfn->func(self, NULL, 0);
    } else {
        TValue val = obj_value(fn);
        ret = kl_eval_code(&val, self, 1);
    }

    return ret;
}

static inline TValue kl_slot_call_one_arg(TValue *self, TValue *arg, int slotid)
{
    TypeObject *tp = kl_typeof(self);
    ASSERT(tp);
    Object *fn = tp->slots[slotid];
    ASSERT(fn);

    TValue ret;

    if (IS_CFUNC(fn)) {
        CFuncObject *cfn = (CFuncObject *)fn;
        ret = cfn->func(self, arg, 1);
    } else {
        TValue val = obj_value(fn);
        TValue args[] = { *self, *arg };
        ret = kl_eval_code(&val, args, 2);
    }

    return ret;
}

static inline TValue kl_slot_call_two_args(TValue *self, TValue *arg0, TValue *arg1, int slotid)
{
    TypeObject *tp = kl_typeof(self);
    ASSERT(tp);
    Object *fn = tp->slots[slotid];
    ASSERT(fn);

    TValue ret;

    if (IS_CFUNC(fn)) {
        CFuncObject *cfn = (CFuncObject *)fn;
        TValue args[] = { *arg0, *arg1 };
        ret = cfn->func(self, args, 2);
    } else {
        TValue val = obj_value(fn);
        TValue args[] = { *self, *arg0, *arg1 };
        ret = kl_eval_code(&val, args, 3);
    }

    return ret;
}

static inline Object *kl_to_str(TValue *val)
{
    TValue s = kl_slot_call_no_arg(val, SLOT_STR);
    return to_obj(&s);
}

static inline unsigned int kl_hash(TValue *val)
{
    TValue ret = kl_slot_call_no_arg(val, SLOT_HASH);
    return (unsigned int)to_int64(&ret);
}

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_OBJECT_H_ */

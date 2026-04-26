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
        void *itbl;    // itable for traits
    };
    union {
        int64_t ival; // integer payload
        double fval;  // floating‑point payload
        int bval;     // boolean payload
        Object *obj;  // heap object pointer
    };
} TValue;

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

#define TAG_BFLOAT16 0b10000
#define TAG_FLOAT16  0b10001
#define TAG_FLOAT32  0b10010
#define TAG_FLOAT64  0b10011

// beyond this value, it's an object pointer
#define TAG_VAL_MAX 64
#define TAG_OBJECT  (TAG_VAL_MAX + 1)

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
#define is_float16(x)  ((x)->tag == TAG_FLOAT16)
#define is_float32(x)  ((x)->tag == TAG_FLOAT32)
#define is_float64(x)  ((x)->tag == TAG_FLOAT64)
#define is_bfloat16(x) ((x)->tag == TAG_BFLOAT16)

/* Category checks */
#define is_int(x)   (((x)->tag & 0b1100) == 0b1000)
#define is_uint(x)  (((x)->tag & 0b1100) == 0b1100)
#define is_float(x) (((x)->tag >= TAG_FLOAT16) && ((x)->tag <= TAG_FLOAT64))

/* Primitive vs reference */
#define is_val(x)  ((x)->tag < TAG_VAL_MAX)
#define is_obj(x)  ((x)->tag == TAG_OBJECT)
#define is_intf(x) ((x)->tag > TAG_OBJECT)
#define is_ref(x)  ((x)->tag >= TAG_OBJECT)

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
#define bfloat16_value(x)   (TValue){ .tag = TAG_BFLOAT16, .fval = (double)(x) }

/* Reference value */
#define obj_value(x)         (TValue){ .tag = TAG_OBJECT, .obj = (x) }
#define intf_value(_itab, x) (TValue){ .itab = (_itab), .obj = (x) }

#define to_bool(v)     ({ ASSERT(is_bool(v)); (v)->bval; })

/* Signed integers */
#define to_int8(v)     ({ ASSERT(is_int8(v)); (v)->ival; })
#define to_int16(v)    ({ ASSERT(is_int16(v)); (v)->ival; })
#define to_int32(v)    ({ ASSERT(is_int32(v)); (v)->ival; })
#define to_int64(v)    ({ ASSERT(is_int64(v)); (v)->ival; })

/* Unsigned integers */
#define to_uint8(v)    ({ ASSERT(is_uint8(v)); (v)->ival; })
#define to_uint16(v)   ({ ASSERT(is_uint16(v)); (v)->ival; })
#define to_uint32(v)   ({ ASSERT(is_uint32(v)); (v)->ival; })
#define to_uint64(v)   ({ ASSERT(is_uint64(v)); (v)->ival; })

/* Floating point (TEMP: stored as double; real impl should preserve bit pattern) */
#define to_float16(v)  ({ ASSERT(is_float16(v)); (v)->fval; })
#define to_float32(v)  ({ ASSERT(is_float32(v)); (v)->fval; })
#define to_float64(v)  ({ ASSERT(is_float64(v)); (v)->fval; })
#define to_bfloat16(v) ({ ASSERT(is_bfloat16(v)); (v)->fval; })

/* Reference */
#define to_obj(v)      ({ ASSERT(is_obj(v)); (v)->obj; })
#define to_intf(v)     ({ ASSERT(is_intf(v)); (v)->obj; })
#define to_ref(v)      ({ ASSERT(is_ref(v)); (v)->obj; })

/* clang-format on */

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

typedef Object *(*AllocFunc)(struct _TypeObject *tp);
typedef int (*InitFunc)(TValue *self, TValue *args, int nargs);
typedef void (*FiniFunc)(Object *self);

typedef unsigned int (*HashFunc)(TValue *self);
typedef TValue (*RichCmpFunc)(TValue *lhs, TValue *rhs, int op);
typedef TValue (*StrFunc)(TValue *self);
typedef TValue (*CallFunc)(TValue *self, TValue *args, int nargs);

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

#define TP_FLAGS_CLASS  (1 << 0)
#define TP_FLAGS_TRAIT  (1 << 1)
#define TP_FLAGS_PUBLIC (1 << 2)

typedef struct _TypeObject {
    OBJECT_HEAD

    /**
     * itable_entry: The magic anchor for interface dispatch and navigation.
     *
     * This is a "vptr" pointing to a specific entry point within a virtual
     * table block. It acts as the functional "View" for a given Trait.
     *
     * 1. Layout Structure:
     *    - Positive offsets [0, +N]: Method pointers for O(1) direct dispatch.
     *    - Negative offsets [-1, -M]: Navigation pointers (Entry Pointers) to
     *      parent itables for O(1) up-casting.
     *
     * 2. Optimization Strategy (The Koala Way):
     *    - PIP (Primary Inheritance Path): For single inheritance or the main
     *      branch of multi-inheritance, these traits share the SAME itable_entry
     *      physical address. Up-casting along the PIP is a zero-cost NO-OP
     *      (Value-equivalent IR).
     *    - SCM (Side-path): For traits with layout conflicts (discontinuous LRO),
     *      this points to a specialized "Patch View" where method pointers are
     *      re-ordered to satisfy the Trait's ABI.
     *
     * 3. Performance for Standard Libraries:
     *    Since most Koala stdlib traits follow single inheritance, they naturally
     *    fall into the PIP. This ensures that most trait operations incur
     *    ZERO overhead for pointer adjustment, behaving like static calls.
     */
    void **itable_entry;

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

    /* allocate function */
    AllocFunc alloc;
    /* init function */
    InitFunc init;
    /* fini function */
    FiniFunc fini;

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
 |  CFunc&Code related                                                       |
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
extern TypeObject Number_type;

extern TypeObject Iterable_type;
extern TypeObject Iterator_type;
extern TypeObject Collection_type;
extern TypeObject Sequence_type;
extern TypeObject MutableSequence_type;

TypeObject *kl_typeof(TValue *val);
int kl_init_type(TypeObject *tp);

/* Any object is callable, if it implements the call protocol. */
static inline TValue kl_do_call(TValue *callable, TValue *args, int nargs)
{
    TypeObject *tp = kl_typeof(callable);
    CallFunc call = tp->call;
    ASSERT(call != NULL);
    return call(callable, args, nargs);
}

static inline TValue kl_do_call_no_arg(TValue *callable)
{
    return kl_do_call(callable, NULL, 0);
}

static inline TValue kl_do_call_one_arg(TValue *callable, TValue *arg)
{
    return kl_do_call(callable, arg, 1);
}

void kl_init_gm_stbl(void);
int kl_load_module(char *path);
Object *kl_get_module(char *path);
int kl_register_module(Object *_m);
void kl_resolve_import(Object *_m);
void kl_dump_module(Object *_m);

TValue kl_eval_code(TValue *self, TValue *args, int nargs);
void kl_run_module(Object *_m);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_OBJECT_H_ */

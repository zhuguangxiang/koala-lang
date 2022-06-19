/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2018-2022 James <zhuguangxiang@gmail.com>
 */

#ifndef _KLM_H_
#define _KLM_H_

#include "common/list.h"
#include "common/vector.h"
#include "koalac/typedesc.h"
#include "opcode.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _KLMPackage KLMPackage;
typedef struct _KLMFunc KLMFunc;
typedef struct _KLMCodeBlock KLMCodeBlock;
typedef struct _KLMType KLMType;
typedef enum _KLMValueKind KLMValueKind;
typedef struct _KLMValue KLMValue;
typedef struct _KLMVar KLMVar;
typedef struct _KLMInst KLMInst;
typedef enum _KLMOperKind KLMOperKind;
typedef struct _KLMOper KLMOper;
typedef enum _KLMConstKind KLMConstKind;
typedef struct _KLMConst KLMConst;

/* package */
struct _KLMPackage {
    /* package name */
    char *name;
    /* variables */
    Vector variables;
    /* functions */
    Vector functions;
    /* types */
    Vector types;
    /* __init__ function */
    KLMFunc *init;
};

/* function */
struct _KLMFunc {
    /* func name */
    char *name;
    /* func proto */
    TypeDesc *ty;
    /* func type parameters */
    Vector *type_params;
    /* code block list */
    List cb_list;
    /* local variables */
    Vector locals;
    /* num of parameters */
    int num_params;
    /* num of local variables */
    int num_locals;
    /* num of code blocks */
    int num_cbs;
};

/* code block */
struct _KLMCodeBlock {
    char *label;
    /* linked in KLMFunc */
    List cb_node;
    /* ->KLMFunc */
    KLMFunc *func;
    /* instructions */
    Vector insts;
    /* num of instructions */
    int num_insts;
    /* branch block */
    int branch;
    /* return block */
    int ret;
};

/* type */
struct _KLMType {
    char *name;
    /* type parameters */
    Vector *type_params;
    /* base class or interface */
    Vector *bases;
    /* fields */
    Vector *fields;
    /* functions */
    Vector *functions;
};

enum _KLMValueKind {
    KLM_VALUE_NONE,
    KLM_VALUE_VAR,
    KLM_VALUE_CONST,
    KLM_VALUE_FUNC,
    KLM_VALUE_BLOCK,
    KLM_VALUE_MAX
};

#define KLM_VALUE_HEAD KLMValueKind kind;

/* value : var, const, func, block */
struct _KLMValue {
    KLM_VALUE_HEAD
};

/* global, local, temp variable */
struct _KLMVar {
    KLM_VALUE_HEAD
    char *name;
    TypeDesc *ty;
    int reg;
    int global;
};

/* operand kind */
enum _KLMOperKind {
    KLM_OPER_NONE,
    KLM_OPER_VAR,
    KLM_OPER_FUNC,
    KLM_OPER_BLOCK,
};

/* instruction operand */
struct _KLMOper {
    KLMOperKind kind;
    union {
        KLMValue *value;
        KLMFunc *func;
        KLMCodeBlock *block;
    };
};

/* instruction */
struct _KLMInst {
    KLMOpCode op;
    /* number of operands */
    int num_opers;
    /* operands */
    KLMOper operands[0];
};

/* constant kind */
enum _KLMConstKind {
    KLM_CONST_NONE,
    KLM_CONST_U8,
    KLM_CONST_U16,
    KLM_CONST_U32,
    KLM_CONST_U64,
    KLM_CONST_I8,
    KLM_CONST_I16,
    KLM_CONST_I32,
    KLM_CONST_I64,
    KLM_CONST_F32,
    KLM_CONST_F64,
    KLM_CONST_BOOL,
    KLM_CONST_CHAR,
    KLM_CONST_STR,
    KLM_CONST_MAX,
};

/* constant */
struct _KLMConst {
    KLM_VALUE_HEAD
    KLMConstKind const_kind;
    union {
        uint8_t u8val;
        int8_t i8val;
        uint16_t u16val;
        int16_t i16val;
        uint32_t u32val;
        int32_t i32val;
        uint64_t u64val;
        int64_t i64val;
        float f32val;
        double f64val;
        char *str;
    };
};

void klm_init_pkg(KLMPackage *pkg, char *name);
void klm_fini_pkg(KLMPackage *pkg);
void klm_write_llvm_bc(KLMPackage *pkg);
void klm_write_file(KLMPackage *pkg, char *path);
void klm_read_file(KLMPackage *pkg, char *filename);

KLMFunc *klmc_add_func(KLMPackage *pkg, char *name, TypeDesc *ty);
KLMValue *klm_add_global(KLMPackage *pkg, char *name, TypeDesc *ty);
KLMCodeBlock *klm_append_block(KLMFunc *func, char *name);
KLMValue *klm_add_local(KLMFunc *func, char *name, TypeDesc *ty);
KLMValue *klm_new_const();

void klm_build_copy(KLMCodeBlock *block, KLMValue *lhs, KLMValue *rhs);
void klm_build_return(KLMCodeBlock *block, KLMValue *ret);

KLMValue *klm_build_add(KLMCodeBlock *block, KLMValue *lhs, KLMValue *rhs);
KLMValue *klm_build_sub(KLMCodeBlock *block, KLMValue *lhs, KLMValue *rhs);
KLMValue *klm_build_mul(KLMCodeBlock *block, KLMValue *lhs, KLMValue *rhs);
KLMValue *klm_build_div(KLMCodeBlock *block, KLMValue *lhs, KLMValue *rhs);
KLMValue *klm_build_mod(KLMCodeBlock *block, KLMValue *lhs, KLMValue *rhs);

void klm_build_branch(KLMCodeBlock *block, KLMValue *cond, KLMCodeBlock *_if,
                      KLMCodeBlock *_else);
void klm_build_jump(KLMCodeBlock *block, KLMCodeBlock *target);

#ifdef __cplusplus
}
#endif

#endif /* _KLM_H_ */

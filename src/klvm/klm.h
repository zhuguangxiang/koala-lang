/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2018-2022 James <zhuguangxiang@gmail.com>
 */

#ifndef _KLM_H_
#define _KLM_H_

#include "common/hashmap.h"
#include "common/list.h"
#include "common/vector.h"
#include "opcode.h"
#include "typedesc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _KLMValueKind KLMValueKind;
typedef struct _KLMModule KLMModule;
typedef struct _KLMPackage KLMPackage;
typedef struct _KLMFunc KLMFunc;
typedef struct _KLMCodeBlock KLMCodeBlock;
typedef struct _KLMType KLMType;
typedef struct _KLMValue KLMValue;
typedef struct _KLMVar KLMVar;
typedef struct _KLMInst KLMInst;
typedef struct _KLMConst KLMConst;

enum _KLMValueKind {
    KLM_VALUE_NONE,
    KLM_VALUE_VAR,
    KLM_VALUE_FUNC,
    KLM_VALUE_STRUCT,
    KLM_VALUE_CLASS,
    KLM_VALUE_INTF,
    KLM_VALUE_ENUM,
    KLM_VALUE_CONST,
    KLM_VALUE_CODE,
    KLM_VALUE_PKG,
    KLM_VALUE_MAX
};

#define KLM_VALUE_HEAD  \
    HashMapEntry hnode; \
    char *name;         \
    KLMValueKind kind;  \
    TypeDesc *ty;

/* per shared object */
struct _KLMModule {
    char *name;
    /* imported packages(KLMPackage) */
    HashMap *ext_pkgs;
    /* to be compiled pkgs */
    Vector pkgs;
};

/* package */
struct _KLMPackage {
    KLM_VALUE_HEAD
    /* -> KLMModule */
    KLMModule *module;
    /* imported packages(KLMExtPkgNode) */
    HashMap *ext_pkgs;
    /* all symbols in current package */
    HashMap *stbl;
    /* variables */
    Vector variables;
    /* functions */
    Vector functions;
    /* types */
    Vector types;
    /* __init__ function */
    KLMFunc *__init__;
    /* compiled files(ParserState) */
    Vector pss;
};

struct _KLMExtPkgNode {
    HashMapEntry hnode;
    KLMPackage *pkg;
};

/* value : var, const, func, type */
struct _KLMValue {
    KLM_VALUE_HEAD
};

/* global, local, temp variable */
struct _KLMVar {
    KLM_VALUE_HEAD
    /* local index in func or global index in pkg */
    int vreg;
    /* global variable */
    int8_t global;
    /* no allocated, string, integer etc */
    int8_t konst;
    /* public or private */
    int8_t pub;
};

/* function */
struct _KLMFunc {
    KLM_VALUE_HEAD
    /* type parameters */
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
    /* public or private */
    int8_t pub;
};

/* type */
struct _KLMType {
    KLM_VALUE_HEAD
    /* type parameters */
    Vector *type_params;
    /* base class or interface */
    Vector *bases;
    /* fields */
    Vector *fields;
    /* functions */
    Vector *functions;
    /* public or private */
    int8_t pub;
};

/* constant: char is int32_t, bool is int8_t */
struct _KLMConst {
    KLM_VALUE_HEAD
    union {
        int8_t i8val;
        int16_t i16val;
        int32_t i32val;
        int64_t i64val;
        float f32val;
        double f64val;
        char *str;
    };
};

/* code block */
struct _KLMCodeBlock {
    KLM_VALUE_HEAD
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

/* instruction */
struct _KLMInst {
    KLMOpCode op;
    /* number of operands */
    int num_opers;
    /* operands */
    KLMValue *opers[0];
};

void klm_init_pkg(KLMPackage *pkg, char *name);
void klm_fini_pkg(KLMPackage *pkg);
void klm_write_llvm_bc(KLMPackage *pkg);
void klm_write_file(KLMPackage *pkg, char *path);
void klm_read_file(KLMPackage *pkg, char *filename);

KLMVar *klm_add_global(KLMPackage *pkg, char *name, TypeDesc *ty);
KLMFunc *klm_add_func(KLMPackage *pkg, char *name, TypeDesc *ty);
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

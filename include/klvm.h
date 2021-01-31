/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangXiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_IR_H_
#define _KOALA_IR_H_

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define KLVM_FLAGS_PUB    1
#define KLVM_FLAGS_CONST  2
#define KLVM_FLAGS_GLOBAL 4

typedef enum KLVMTypeKind {
    KLVM_TYPE_INT8 = 1,
    KLVM_TYPE_INT16,
    KLVM_TYPE_INT32,
    KLVM_TYPE_INT64,
    KLVM_TYPE_FLOAT32,
    KLVM_TYPE_FLOAT64,
    KLVM_TYPE_BOOL,
    KLVM_TYPE_CHAR,
    KLVM_TYPE_STRING,
    KLVM_TYPE_ANY,
    KLVM_TYPE_ARRAY,
    KLVM_TYPE_DICT,
    KLVM_TYPE_TUPLE,
    KLVM_TYPE_VALIST,
    KLVM_TYPE_KLASS,
    KLVM_TYPE_PROTO,
} KLVMTypeKind;

typedef struct KLVMType {
    KLVMTypeKind kind;
} KLVMType;

KLVMType *KLVMInt32Type(void);
KLVMType *KLVMBoolType(void);
int KLVMTypeCheck(KLVMType *ty1, KLVMType *ty2);
char *KLVMTypeString(KLVMType *type);

typedef enum KLVMValueKind {
    VALUE_CONST = 1,
    VALUE_VAR,
    VALUE_FUNC,
    VALUE_INST,
    VALUE_TYPE,
} KLVMValueKind;

// clang-format off
// Don't touch this!

#define KLVM_VALUE_HEAD \
    KLVMValueKind kind; KLVMType *type; const char *name; int tag;

#define INIT_VALUE_HEAD(val, _kind, _type, _name) \
    val->kind = _kind; val->type = _type; val->name = _name; val->tag = -1

// clang-format on

typedef struct KLVMValue {
    KLVM_VALUE_HEAD
} KLVMValue;

typedef struct KLVMModule {
    const char *name;
    HashMap map;
    Vector funcs;
    Vector vars;
    Vector consts;
    KLVMValue *fn;
} KLVMModule;

typedef struct KLVMConst {
    KLVM_VALUE_HEAD
    union {
        int8_t i8val;
        int16_t i16val;
        int32_t i32val;
        int64_t i64val;
        float f32val;
        double f64val;
        void *ptr;
    };
} KLVMConst;

typedef struct KLVMVar {
    KLVM_VALUE_HEAD
    int flags;
    int offset;
} KLVMVar;

typedef struct KLVMBasicBlock KLVMBasicBlock;

typedef struct KLVMBuilder {
    KLVMBasicBlock *bb;
    ListNode *rover;
} KLVMBuilder;

struct KLVMBasicBlock {
    ListNode bbnode;
    KLVMBuilder bldr;
    KLVMValue *fn;
    const char *label;
    int tag;
    int num_insts;
    List insts;
};

typedef struct KLVMFunc {
    KLVM_VALUE_HEAD
    int flags;
    int tag_index;
    int bb_tag_index;
    int num_params;
    Vector locals;
    KLVMBasicBlock *entry;
    int num_bbs;
    List bblist;
} KLVMFunc;

KLVMValue *KLVMGetParam(KLVMValue *fn, int index);

typedef enum KLVMBinaryOp {
    KLVM_BINARY_INVALID,
    KLVM_BINARY_ADD,
    KLVM_BINARY_SUB,
    KLVM_BINARY_MUL,
    KLVM_BINARY_DIV,
    KLVM_BINARY_MOD,
    KLVM_BINARY_POW,

    KLVM_BINARY_CMP_EQ,
    KLVM_BINARY_CMP_NE,
    KLVM_BINARY_CMP_GT,
    KLVM_BINARY_CMP_GE,
    KLVM_BINARY_CMP_LT,
    KLVM_BINARY_CMP_LE,

    KLVM_BINARY_AND,
    KLVM_BINARY_OR,

    KLVM_BINARY_BIT_AND,
    KLVM_BINARY_BIT_OR,
    KLVM_BINARY_BIT_XOR,
    KLVM_BINARY_BIT_SHL,
    KLVM_BINARY_BIT_SHR,
} KLVMBinaryOp;

typedef enum KLVMUnaryOp {
    KLVM_UNARY_INVALID,
    KLVM_UNARY_NEG,
    KLVM_UNARY_NOT,
    KLVM_UNARY_BIT_NOT,
} KLVMUnaryOp;

typedef enum KLVMInstKind {
    KLVM_INST_INVALID,
    KLVM_INST_COPY,
    KLVM_INST_BINARY,
    KLVM_INST_UNARY,
    KLVM_INST_CALL,
    KLVM_INST_RET,
    KLVM_INST_RET_VOID,
    KLVM_INST_JMP,
    KLVM_INST_JMP_COND,
    KLVM_INST_INDEX,
    KLVM_INST_NEW,
    KLVM_INST_FIELD,
} KLVMInstKind;

// clang-format off
// Don't touch this!
#define KLVM_INST_HEAD \
    KLVM_VALUE_HEAD ListNode node; KLVMInstKind op;

#define INIT_INST_HEAD(inst, type, _op, _name)      \
    INIT_VALUE_HEAD(inst, VALUE_INST, type, _name); \
    list_node_init(&inst->node);                    \
    inst->op = _op

// clang-format on

typedef struct KLVMInst {
    KLVM_INST_HEAD
} KLVMInst;

typedef struct KLVMCopyInst {
    KLVM_INST_HEAD
    KLVMValue *lhs;
    KLVMValue *rhs;
} KLVMCopyInst;

typedef struct KLVMBinaryInst {
    KLVM_INST_HEAD
    KLVMBinaryOp bop;
    KLVMValue *lhs;
    KLVMValue *rhs;
} KLVMBinaryInst;

typedef struct KLVMUnaryInst {
    KLVM_INST_HEAD
    KLVMBinaryOp uop;
    KLVMValue *val;
} KLVMUnaryInst;

typedef struct KLVMCallInst {
    KLVM_INST_HEAD
    KLVMValue *fn;
    Vector args;
} KLVMCallInst;

typedef struct KLVMJmpInst {
    KLVM_INST_HEAD
    KLVMBasicBlock *dest;
} KLVMJmpInst;

typedef struct KLVMCondJmpInst {
    KLVM_INST_HEAD
    KLVMValue *cond;
    KLVMBasicBlock *_then;
    KLVMBasicBlock *_else;
} KLVMCondJmpInst;

typedef struct KLVMRetInst {
    KLVM_INST_HEAD
    KLVMValue *ret;
} KLVMRetInst;

/* Create a new module */
KLVMModule *KLVMCreateModule(const char *name);

/* Destroy a module */
void KLVMDestroyModule(KLVMModule *m);

/* Dump a module */
void KLVMDumpModule(KLVMModule *m);

/* Add a variable to a module */
KLVMValue *KLVMAddVar(KLVMModule *m, KLVMType *ty, const char *name);

void KLVMSetVarName(KLVMValue *var, const char *name);
void KLVMSetVarPub(KLVMValue *var);
void KLVMSetVarConst(KLVMValue *var);

/* Add a function to a module */
KLVMValue *KLVMAddFunc(KLVMModule *m, KLVMType *ty, const char *name);

void KLVMSetFuncPub(KLVMValue *fn);

/* Get a function type */
KLVMType *KLVMProtoType(KLVMType *ret, KLVMType **param);
Vector *KLVMProtoTypeParams(KLVMType *ty);
KLVMValue *KLVMAddLocVar(KLVMValue *fn, KLVMType *ty, const char *name);
KLVMType *KLVMProtoTypeReturn(KLVMType *ty);

/* Append a basic block to the end of a function */
KLVMBasicBlock *KLVMFuncEntryBasicBlock(KLVMValue *fn);
KLVMBasicBlock *KLVMAppendBasicBlock(KLVMValue *fn, const char *name);

/* Add a basic block after 'bb' */
KLVMBasicBlock *KLVMAddBasicBlock(KLVMBasicBlock *bb, const char *name);

/* Add a basic block before 'bb' */
KLVMBasicBlock *KLVMAddBasicBlockBefore(KLVMBasicBlock *bb, const char *name);

/* Create an instruction builder */
KLVMBuilder *KLVMBasicBlockBuilder(KLVMBasicBlock *bb);

void KLVMSetBuilderPointAtEnd(KLVMBuilder *bldr);
void KLVMResetBuilderPoint(KLVMBuilder *bldr);
void KLVMSetBuilderPoint(KLVMBuilder *bldr, KLVMValue *inst);
void KLVMSetBuilderPointBefore(KLVMBuilder *bldr, KLVMValue *inst);

KLVMValue *KLVMBuildBinary(KLVMBuilder *bldr, KLVMBinaryOp op, KLVMValue *lhs,
    KLVMValue *rhs, const char *name);
#define KLVMBuildAdd(bldr, lhs, rhs, name) \
    KLVMBuildBinary(bldr, KLVM_BINARY_ADD, lhs, rhs, name)
#define KLVMBuildSub(bldr, lhs, rhs, name) \
    KLVMBuildBinary(bldr, KLVM_BINARY_SUB, lhs, rhs, name)
#define KLVMBuildCondLE(bldr, lhs, rhs, name) \
    KLVMBuildBinary(bldr, KLVM_BINARY_CMP_LE, lhs, rhs, name)

KLVMValue *KLVMBuildCall(
    KLVMBuilder *bldr, KLVMValue *fn, KLVMValue **args, const char *name);

void KLVMBuildJmp(KLVMBuilder *bldr, KLVMBasicBlock *dest);
void KLVMBuildCondJmp(KLVMBuilder *bldr, KLVMValue *cond, KLVMBasicBlock *_then,
    KLVMBasicBlock *_else);

void KLVMBuildRet(KLVMBuilder *bldr, KLVMValue *v);
void KLVMBuildRetVoid(KLVMBuilder *bldr);

KLVMValue *KLVMConstInt32(int32_t ival);

void KLVMBuildCopy(KLVMBuilder *bldr, KLVMValue *rhs, KLVMValue *lhs);

KLVMValue *KLVMModuleFunc(KLVMModule *m);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_IR_H_ */

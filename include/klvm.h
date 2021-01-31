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
int KLVMTypeCheck(KLVMType *ty1, KLVMType *ty2);
char *KLVMTypeString(KLVMType *type);

typedef enum KLVMValueKind {
    VALUE_CONST = 1,
    VALUE_VAR,
    VALUE_FUNC,
    VALUE_INST,
    VALUE_TYPE,
} KLVMValueKind;

#define KLVM_VALUE_HEAD \
    KLVMValueKind kind; \
    KLVMType *type;     \
    const char *name;

// clang-format off
// Don't touch this!
#define INIT_VALUE_HEAD(val, _kind, _type, _name) \
    val->kind = _kind; val->type = _type; val->name = _name

// clang-format on

typedef struct KLVMValue {
    KLVM_VALUE_HEAD
} KLVMValue;

void KLVMSetValueName(KLVMValue *val, const char *name);

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

typedef struct KLVMBasicBlock {
    ListNode bbnode;
    KLVMValue *fn;
    const char *name;
    int num_insts;
    List insts;
} KLVMBasicBlock;

typedef struct KLVMFunc {
    KLVM_VALUE_HEAD
    int flags;
    int num_params;
    Vector locals;
    int num_bbs;
    List bblist;
} KLVMFunc;

KLVMValue *KLVMGetParam(KLVMValue *fn, int index);

typedef struct KLVMBuilder {
    KLVMBasicBlock *bb;
    ListNode *rover;
} KLVMBuilder;

typedef enum KLVMInstKind {
    KLVM_INST_INVALID,
    KLVM_INST_COPY,
    KLVM_INST_ADD,
    KLVM_INST_SUB,
    KLVM_INST_CALL,
    KLVM_INST_RET,
    KLVM_INST_RET_VOID,
    KLVM_INST_JMP,
    KLVM_INST_JMP_EQ,
    KLVM_INST_JMP_NE,
    KLVM_INST_JMP_GT,
    KLVM_INST_JMP_GE,
    KLVM_INST_JMP_LT,
    KLVM_INST_JMP_LE,
    KLVM_INST_JMP_TRUE,
    KLVM_INST_JMP_FALSE,
    KLVM_INST_JMP_NIL,
    KLVM_INST_JMP_NOTNIL,
} KLVMInstKind;

#define KLVM_INST_HEAD             \
    KLVM_VALUE_HEAD ListNode node; \
    KLVMInstKind op;

#define INIT_INST_HEAD(inst, type, _op, _name)      \
    INIT_VALUE_HEAD(inst, VALUE_INST, type, _name); \
    list_node_init(&inst->node);                    \
    inst->op = _op

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
    KLVMValue *lhs;
    KLVMValue *rhs;
} KLVMBinaryInst;

typedef struct KLVMCallInst {
    KLVM_INST_HEAD
    KLVMValue *fn;
    Vector args;
} KLVMCallInst;

typedef struct KLVMCondJumpInst {
    KLVM_INST_HEAD
    KLVMValue *lhs;
    KLVMValue *rhs;
    KLVMBasicBlock *_then;
    KLVMBasicBlock *_else;
} KLVMCondJumpInst;

typedef struct KLVMJumpInst {
    KLVM_INST_HEAD
    KLVMBasicBlock *dest;
} KLVMJumpInst;

typedef struct KLVMRetInst {
    KLVM_INST_HEAD
    KLVMValue *ret;
} KLVMRetInst;

typedef struct KLVMModule {
    const char *name;
    HashMap map;
    Vector funcs;
    Vector vars;
    Vector consts;
    KLVMValue *fn;
} KLVMModule;

/* Create a new module */
KLVMModule *KLVMCreateModule(const char *name);

/* Destroy a module */
void KLVMDestroyModule(KLVMModule *m);

/* Dump a module */
void KLVMDumpModule(KLVMModule *m);

/* Add a variable to a module */
KLVMValue *KLVMAddVar(KLVMModule *m, KLVMType *ty, const char *name);

void KLVMSetVarPub(KLVMValue *var);
void KLVMSetVarConst(KLVMValue *var);

/* Add a function to a module */
KLVMValue *KLVMAddFunc(KLVMModule *m, KLVMType *ty, const char *name);

void KLVMSetFuncPub(KLVMValue *var);

/* Get a function type */
KLVMType *KLVMProtoType(KLVMType *ret, KLVMType **param);
Vector *KLVMProtoTypeParams(KLVMType *ty);
KLVMValue *KLVMAddLocVar(KLVMFunc *fn, KLVMType *ty, const char *name);
KLVMType *KLVMProtoTypeReturn(KLVMType *ty);

/* Append a basic block to the end of a function */
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

KLVMValue *KLVMBuildAdd(
    KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs, const char *name);
KLVMValue *KLVMBuildSub(
    KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs, const char *name);

KLVMValue *KLVMBuildCall(
    KLVMBuilder *bldr, KLVMValue *fn, KLVMValue **args, const char *name);

KLVMValue *KLVMBuildJump(KLVMBuilder *bldr, KLVMBasicBlock *dest);
KLVMValue *KLVMBuildCondJump(KLVMBuilder *bldr, KLVMInstKind op, KLVMValue *lhs,
    KLVMValue *rhs, KLVMBasicBlock *_then, KLVMBasicBlock *_else);
#define KLVMBuildCondJumpEQ(bldr, lhs, rhs, _then, _else) \
    KLVMBuildCondJump(bldr, KLVM_INST_JMP_EQ, lhs, rhs, _then, _else)
#define KLVMBuildCondJumpNE(bldr, lhs, rhs, _then, _else) \
    KLVMBuildCondJump(bldr, KLVM_INST_JMP_NE, lhs, rhs, _then, _else)
#define KLVMBuildCondJumpGT(bldr, lhs, rhs, _then, _else) \
    KLVMBuildCondJump(bldr, KLVM_INST_JMP_GT, lhs, rhs, _then, _else)
#define KLVMBuildCondJumpGE(bldr, lhs, rhs, _then, _else) \
    KLVMBuildCondJump(bldr, KLVM_INST_JMP_GE, lhs, rhs, _then, _else)
#define KLVMBuildCondJumpLT(bldr, lhs, rhs, _then, _else) \
    KLVMBuildCondJump(bldr, KLVM_INST_JMP_LT, lhs, rhs, _then, _else)
#define KLVMBuildCondJumpLE(bldr, lhs, rhs, _then, _else) \
    KLVMBuildCondJump(bldr, KLVM_INST_JMP_LE, lhs, rhs, _then, _else)
#define KLVMBuildCondJumpTrue(bldr, cond, _then, _else) \
    KLVMBuildCondJump(bldr, KLVM_INST_JMP_TRUE, cond, NULL, _then, _else)
#define KLVMBuildCondJumpFalse(bldr, cond, _then, _else) \
    KLVMBuildCondJump(bldr, KLVM_INST_JMP_FALSE, cond, NULL, _then, _else)
#define KLVMBuildCondJumpNil(bldr, cond, _then, _else) \
    KLVMBuildCondJump(bldr, KLVM_INST_JMP_NIL, cond, NULL, _then, _else)
#define KLVMBuildCondJumpNotNil(bldr, cond, _then, _else) \
    KLVMBuildCondJump(bldr, KLVM_INST_JMP_NOTNIL, cond, NULL, _then, _else)

KLVMValue *KLVMBuildRet(KLVMBuilder *bldr, KLVMValue *v);
KLVMValue *KLVMBuildRetVoid(KLVMBuilder *bldr);

KLVMValue *KLVMConstInt32(int32_t ival);

void KLVMBuildCopy(KLVMBuilder *bldr, KLVMValue *rhs, KLVMValue *lhs);

KLVMValue *KLVMModuleFunc(KLVMModule *m);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_IR_H_ */

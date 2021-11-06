/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KLVM_H_
#define _KLVM_H_

#include "util/bitvector.h"
#include "util/hashmap.h"
#include "util/list.h"
#include "util/typedesc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _KLVMModule KLVMModule;
typedef struct _KLVMVar KLVMVar;
typedef enum _KLVMValueKind KLVMValueKind;
typedef struct _KLVMValue KLVMValue;
typedef struct _KLVMConst KLVMConst;
typedef struct _KLVMValDef KLVMValDef;
typedef struct _KLVMInterval KLVMInterval;
typedef struct _KLVMRange KLVMRange;
typedef struct _KLVMFunc KLVMFunc;
typedef struct _KLVMBasicBlock KLVMBasicBlock;
typedef struct _KLVMEdge KLVMEdge;
typedef struct _KLVMUse KLVMUse;
typedef enum _KLVMOperKind KLVMOperKind;
typedef struct _KLVMOper KLVMOper;
typedef struct _KLVMInst KLVMInst;
typedef struct _KLVMBuilder KLVMBuilder;

struct _KLVMModule {
    /* module name */
    char *name;
    /* all symbols */
    HashMap stbl;
    /* variables */
    Vector variables;
    /* functions */
    Vector functions;
    /* types */
    Vector types;
    /* __init__ function */
    KLVMFunc *func;
};

/* Create a new module */
KLVMModule *klvm_create_module(char *name);

/* Destroy a module */
void klvm_destroy_module(KLVMModule *m);

/* Print a module */
void klvm_print_module(KLVMModule *m, FILE *fp);

/* clang-format off */
/* Dump a module */
#define klvm_dump_module(m) klvm_print_module(m, stdout); fflush(stdout)
/* clang-format on */

/* Add a variable to a module */
KLVMVar *klvm_add_var(KLVMModule *m, char *name, TypeDesc *type);

/* Add a function to a module */
KLVMFunc *klvm_add_func(KLVMModule *m, char *name, TypeDesc *type);

/* Get module's init function */
KLVMFunc *klvm_init_func(KLVMModule *m);

/* variable */
struct _KLVMVar {
    /* name */
    char *name;
    /* type */
    TypeDesc *type;
};

enum _KLVMValueKind {
    KLVM_VALUE_CONST = 1,
    KLVM_VALUE_REG,
};

#define KLVM_VALUE_HEAD \
    /* value kind */    \
    KLVMValueKind kind; \
    /* register type */ \
    TypeDesc *type;

struct _KLVMValue {
    KLVM_VALUE_HEAD
};

/* constant */
struct _KLVMConst {
    KLVM_VALUE_HEAD
    union {
        int32 ival;
        int64 i64val;
        float32 fval;
        float64 f64val;
        int32 cval;
        int8 bval;
        char *sval;
    };
};

/* value(reg) definition */
struct _KLVMValDef {
    KLVM_VALUE_HEAD
    /* label & tag */
    char *label;
    int tag;
    /* offset in IR */
    int pos;
    /* value numbering */
    int valnum;
    /* register */
    int reg;
    /* value interval */
    KLVMInterval *interval;
};

KLVMValue *klvm_const_int8(int8 v);
KLVMValue *klvm_const_int16(int16 v);
KLVMValue *klvm_const_int32(int32 v);
KLVMValue *klvm_const_int64(int64 v);
KLVMValue *klvm_const_float32(float32 v);
KLVMValue *klvm_const_float64(float64 v);
KLVMValue *klvm_const_bool(int8 v);
KLVMValue *klvm_const_char(int32 ch);
KLVMValue *klvm_const_str(char *s);

/* function */
struct _KLVMFunc {
    /* name */
    char *name;
    /* type */
    TypeDesc *type;
    /* tags of valdef */
    int def_tags;
    /* tags of basic block */
    int bb_tags;
    /* number of basic block */
    int num_bbs;
    /* basic block list */
    List bb_list;
    /* all edges */
    List edge_list;
    /* local varables */
    Vector locals;
    /* number of parameters */
    int num_params;
    /* start basic block */
    KLVMBasicBlock *sbb;
    /* end basic block */
    KLVMBasicBlock *ebb;
};

/* Get function prototype */
TypeDesc *klvm_get_func_type(KLVMFunc *fn);

/* Get function param by index */
KLVMValue *klvm_get_param(KLVMFunc *fn, int index);

/* Add a local variable */
KLVMValue *klvm_add_local(KLVMFunc *fn, TypeDesc *ty, char *name);

struct _KLVMBasicBlock {
    /* linked in KLVMFunction */
    List bb_link;
    /* -> KLVMFunction */
    KLVMFunc *func;
    /* tagged name */
    char *label;
    /* tagged number */
    int tag;

    /* computed by liveness analysis.*/
    struct {
        /* instructions sequence number in this block, [start, end) */
        int start, end;

        /* variables(registers) defined by this block */
        BitVector *defs;

        /* variables(registers) used by this block */
        BitVector *uses;

        /* variables(registers) that are live when entering this block */
        BitVector *liveins;

        /* variables(registers) that are live when exiting this block */
        BitVector *liveouts;
    };

    int num_insts;
    List inst_list;
    List in_edges;
    List out_edges;
};

/* Append a basic block to the end of a function */
KLVMBasicBlock *klvm_append_block(KLVMFunc *fn, char *label);

/* Add a basic block after 'bb' */
KLVMBasicBlock *klvm_add_block(KLVMBasicBlock *bb, char *label);

/* Add a basic block before 'bb' */
KLVMBasicBlock *klvm_add_block_before(KLVMBasicBlock *bb, char *label);

/* Delete a basic block */
void klvm_delete_block(KLVMBasicBlock *bb);

/* basic block iteration */
#define basic_block_foreach(bb, bb_list, closure) \
    list_foreach(bb, bb_link, bb_list, closure)

struct _KLVMEdge {
    /* linked in KLVMFunc */
    List link;
    /* src block */
    KLVMBasicBlock *src;
    /* dest block */
    KLVMBasicBlock *dst;
    /* add to dest block */
    List in_link;
    /* add to src block */
    List out_link;
};

/* Add an edge */
void klvm_link_age(KLVMBasicBlock *src, KLVMBasicBlock *dst);

/* edge-out iteration */
#define edge_out_foreach(edge, out_list, closure) \
    list_foreach(edge, out_link, out_list, closure)

/* edge-in iteration */
#define edge_in_foreach(edge, in_list, closure) \
    list_foreach(edge, in_link, in_list, closure)

#include "inst.h"
#include "interval.h"
#include "pass.h"

/* print instruction */
void klvm_print_inst(KLVMFunc *fn, KLVMInst *inst, FILE *fp);

#ifdef __cplusplus
}
#endif

#endif /* _KLVM_H_ */

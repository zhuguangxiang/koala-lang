/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KLVM_H_
#define _KLVM_H_

#include "util/bitset.h"
#include "util/hashmap.h"
#include "util/list.h"
#include "util/typedesc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _KLVMModule KLVMModule;
typedef enum _KLVMValueKind KLVMValueKind;
typedef struct _KLVMValue KLVMValue;
typedef struct _KLVMConst KLVMConst;
typedef struct _KLVMVar KLVMVar;
typedef struct _KLVMArgument KLVMArgument;
typedef struct _KLVMLocal KLVMLocal;
typedef struct _KLVMFunc KLVMFunc;
typedef struct _KLVMBasicBlock KLVMBasicBlock;
typedef struct _KLVMUse KLVMUse;
typedef enum _KLVMOperKind KLVMOperKind;
typedef struct _KLVMOper KLVMOper;
typedef struct _KLVMInst KLVMInst;
typedef struct _KLVMEdge KLVMEdge;
typedef struct _KLVMInterval KLVMInterval;
typedef struct _KLVMRange KLVMRange;
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
    KLVMFunc *init;
};

/* Create a new module */
KLVMModule *klvm_create_module(char *name);

/* Destroy a module */
void klvm_destroy_module(KLVMModule *m);

/* Add a variable to a module */
KLVMValue *klvm_add_var(KLVMModule *m, char *name, TypeDesc *type);

/* Add a function to a module */
KLVMFunc *klvm_add_func(KLVMModule *m, char *name, TypeDesc *type);

/* Get module's init function */
KLVMFunc *klvm_init_func(KLVMModule *m);

enum _KLVMValueKind {
    KLVM_VALUE_NONE,
    KLVM_VALUE_CONST,
    KLVM_VALUE_VAR,
    KLVM_VALUE_FUNC,
    KLVM_VALUE_BLOCK,
    KLVM_VALUE_ARG,
    KLVM_VALUE_LOCAL,
    KLVM_VALUE_INST,
};

/* clang-format off */
#define KLVM_VALUE_HEAD KLVMValueKind kind; List use_list; char *name; \
    TypeDesc *type; int vreg; int tag;
/* clang-format on */

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

KLVMValue *klvm_const_int8(int8 v);
KLVMValue *klvm_const_int16(int16 v);
KLVMValue *klvm_const_int32(int32 v);
KLVMValue *klvm_const_int64(int64 v);
KLVMValue *klvm_const_float32(float32 v);
KLVMValue *klvm_const_float64(float64 v);
KLVMValue *klvm_const_bool(int8 v);
KLVMValue *klvm_const_char(int32 ch);
KLVMValue *klvm_const_str(char *s);

/* variable */
struct _KLVMVar {
    KLVM_VALUE_HEAD
};

/* argument variable */
struct _KLVMArgument {
    KLVM_VALUE_HEAD
};

/* local variable */
struct _KLVMLocal {
    KLVM_VALUE_HEAD
    /* link in bb */
    List bb_link;
    /* ->bb */
    KLVMBasicBlock *bb;
};

/* local iteration */
#define local_foreach(local, bb, closure) \
    list_foreach(local, bb_link, &(bb)->local_list, closure)

/* function */
struct _KLVMFunc {
    KLVM_VALUE_HEAD
    /* value tag */
    int val_tag;
    /* basic block tag */
    int bb_tag;
    /* number of instructions */
    int num_insts;
    /* number of registers */
    int vregs;
    /* basic block list */
    List bb_list;
    /* all edges */
    List edge_list;
    /* arguments */
    Vector args;
    /* start basic block */
    KLVMBasicBlock *sbb;
    /* end basic block */
    KLVMBasicBlock *ebb;
};

/* Get function prototype */
TypeDesc *klvm_get_functype(KLVMFunc *fn);

/* Get function param by index */
KLVMValue *klvm_get_param(KLVMFunc *fn, int index);

/* Set value name */
void klvm_set_name(KLVMValue *val, char *name);

/*
Start: target of a branch(label) or the instruction after a branch.
End: a branch/return or the instrunction before the target of a branch.
 */
struct _KLVMBasicBlock {
    KLVM_VALUE_HEAD
    /* linked in KLVMFunc */
    List link;
    /* ->KLVMFunc(parent) */
    KLVMFunc *func;

    /* block has branch flag */
    bool has_branch;
    /* block has return flag */
    bool has_ret;

    /* number of locals */
    int num_locals;
    /* number of instructions */
    int num_insts;
    /* number of in-edges */
    int num_inedges;
    /* number of out-edges */
    int num_outedges;

    /* locals */
    List local_list;
    /* instructions */
    List inst_list;
    /* in edges(predecessors) */
    List in_edges;
    /* out edges(successors) */
    List out_edges;

    /* computed by liveness analysis */
    struct {
        /* instructions sequence number in this block, [start, end) */
        int start, end;

        /* variables(registers) defined by this block */
        BitSet *def_set;

        /* variables(registers) used by this block */
        BitSet *use_set;

        /* variables(registers) that are live when entering this block */
        BitSet *live_in_set;

        /* variables(registers) that are live when exiting this block */
        BitSet *live_out_set;
    };
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
#define basic_block_foreach(bb, fn, closure) \
    list_foreach(bb, link, &(fn)->bb_list, closure)

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
#define edge_out_foreach(edge, bb, closure) \
    list_foreach(edge, out_link, &(bb)->out_edges, closure)

/* edge-in iteration */
#define edge_in_foreach(edge, bb, closure) \
    list_foreach(edge, in_link, &(bb)->in_edges, closure)

#define edge_out_empty(bb) list_empty(&(bb)->out_edges)
#define edge_out_first(bb) list_first(&(bb)->out_edges, KLVMEdge, out_link)
#define edge_out_last(bb)  list_last(&(bb)->out_edges, KLVMEdge, out_link)

#define edge_in_empty(bb) list_empty(&(bb)->in_edges)
#define edge_in_first(bb) list_first(&(bb)->in_edges, KLVMEdge, in_link)
#define edge_in_last(bb)  list_last(&(bb)->in_edges, KLVMEdge, in_link)

#include "inst.h"
// #include "interval.h"
#include "pass.h"
#include "printer.h"

/* compute instruction linear position */
void klvm_compute_inst_positions(KLVMFunc *fn);

/* liveness analysis */
void klvm_analyze_liveness(KLVMFunc *fn);

#ifdef __cplusplus
}
#endif

#endif /* _KLVM_H_ */

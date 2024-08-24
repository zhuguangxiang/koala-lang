/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_IR_H_
#define _KOALA_IR_H_

#include "hashmap.h"
#include "list.h"
#include "opcode.h"
#include "typedesc.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _KlrValueKind {
    KLR_VALUE_NONE,
    KLR_VALUE_CONST,
    KLR_VALUE_GLOBAL,
    KLR_VALUE_FUNC,
    KLR_VALUE_BLOCK,
    KLR_VALUE_PARAM,
    KLR_VALUE_LOCAL,
    KLR_VALUE_INSN,
    KLR_VALUE_MAX,
} KlrValueKind;

/* clang-format off */
#define KLR_VALUE_HEAD      \
    KlrValueKind kind;      \
    /* value type */        \
    TypeDesc *desc;         \
    /* list of all Uses */  \
    List use_list;          \
    /* printable name */    \
    char *name;             \
    /* printable tag */     \
    int tag;
/* clang-format on */

typedef struct _KlrValue {
    KLR_VALUE_HEAD
} KlrValue;

#define INIT_KLR_VALUE(val, _kind, _desc, _name) \
    (val)->kind = (_kind); \
    (val)->desc = (_desc); \
    init_list(&(val)->use_list); \
    (val)->name = _name ? _name : ""; \
    (val)->tag = -1;

typedef union _KlrConstValue {
    int64_t ival;
    double fval;
    int bval;
    char *sval;
} KlrConstValue;

/* literal constant */
typedef struct _KlrConst {
    KLR_VALUE_HEAD
    int which;
#define CONST_INT  1
#define CONST_FLT  2
#define CONST_BOOL 3
#define CONST_STR  4
    int len;
    union {
        int64_t ival;
        double fval;
        int bval;
        char *sval;
    };
} KlrConst;

/* global variable */
typedef struct _KlrGlobal {
    KLR_VALUE_HEAD
} KlrGlobal;

/* local register variable */
typedef struct _KlrLocal {
    KLR_VALUE_HEAD
    /* link in bb */
    List bb_link;
    /* ->bb */
    struct _KlrBasicBlock *bb;
    /* rename counter(phi used) */
    int counter;
} KlrLocal;

/* function parameter */
typedef struct _KlrParam {
    KLR_VALUE_HEAD
} KlrParam;

/* function */
typedef struct _KlrFunc {
    KLR_VALUE_HEAD

    /* basic block tag */
    int bb_tag;
    /* variable order */
    int order;

    /* basic block list */
    List bb_list;
    /* all edges */
    List edge_list;

    /* params(value, use only) */
    Vector params;
    /* locals(load/store) */
    Vector locals;

    /* start basic block */
    struct _KlrBasicBlock *sbb;
    /* end basic block */
    struct _KlrBasicBlock *ebb;

    /* module pointer */
    struct _KlrModule *module;
} KlrFunc;

/* basic block */
typedef struct _KlrBasicBlock {
    KLR_VALUE_HEAD

    /* linked in KlrFunc */
    List link;
    /* ->KlrFunc(parent) */
    KlrFunc *func;

    /* locals defined in this block */
    List local_list;

    /* block has branch flag */
    int has_branch;
    /* block has return flag */
    int has_ret;

    /* number of instructions */
    int num_insns;

    /* instructions */
    List insn_list;

    /* number of in-edges */
    int num_inedges;
    /* number of out-edges */
    int num_outedges;

    /* in edges(predecessors) */
    List in_edges;
    /* out edges(successors) */
    List out_edges;

} KlrBasicBlock;

/* edge between basic blocks */
typedef struct _KlrEdge {
    /* linked in KlrFunc */
    List link;

    /* src block */
    KlrBasicBlock *src;
    /* add to src block */
    List out_link;

    /* dest block */
    KlrBasicBlock *dst;
    /* add to dest block */
    List in_link;
} KlrEdge;

/* module(translation basic unit) */
typedef struct _KlrModule {
    /* module name */
    char *name;

    /* global variables */
    Vector globals;

    /* functions */
    Vector functions;

    /* __init__ function */
    KlrFunc *init;
} KlrModule;

/* def-use */
typedef struct _KlrUse {
    /* def-value(use_list) */
    KlrValue *ref;
    /* link in use_list */
    List use_link;

    /* self instruction */
    struct _KlrInsn *insn;
    /* self operand */
    struct _KlrOper *oper;
} KlrUse;

/* phi operand */
typedef struct _KlrPhiParam {
    KlrUse use;
    KlrBasicBlock *bb;
    List bb_link;
} KlrPhiParam;

typedef enum _KlrOperKind {
    KLR_OPER_NONE,
    KLR_OPER_CONST,
    KLR_OPER_GLOBAL,
    KLR_OPER_FUNC,
    KLR_OPER_BLOCK,
    KLR_OPER_PARAM,
    KLR_OPER_LOCAL,
    KLR_OPER_INSN,
    KLR_OPER_PHI,
} KlrOperKind;

/* operand */
typedef struct _KlrOper {
    KlrOperKind kind;
    union {
        KlrUse use;
        KlrPhiParam phi;
    };
} KlrOper;

#define KLR_INSN_FLAGS_LOOP 1

/* instruction */
typedef struct _KlrInsn {
    KLR_VALUE_HEAD

    /* opcode */
    OpCode code;

    /* linear position */
    int pos;

    /* instruction flags */
    int flags;

    /* filled phi parameter index */
    int filled;

    /* link in bb */
    List bb_link;
    /* ->bb */
    KlrBasicBlock *bb;

    /* phi variable */
    KlrValue *phi;

    /* constant result of constant folding */
    KlrValue *result;

    /* number of operands */
    int num_opers;
    /* operands */
    KlrOper opers[0];
} KlrInsn;

/* instruction builder */
typedef struct _KlrBuilder {
    KlrBasicBlock *bb;
    List *it;
} KlrBuilder;

/* APIs */

/* <1> literal constants */
KlrValue *klr_const_int(TypeDesc *ty, int64_t val);

static inline KlrValue *klr_const_int8(int8_t val)
{
    return klr_const_int(desc_int8(), val);
}

static inline KlrValue *klr_const_int16(int16_t val)
{
    return klr_const_int(desc_int16(), val);
}

static inline KlrValue *klr_const_int32(int32_t val)
{
    return klr_const_int(desc_int32(), val);
}

static inline KlrValue *klr_const_int64(int64_t val)
{
    return klr_const_int(desc_int64(), val);
}

static inline KlrValue *klr_const_float32(float val)
{
    return klr_const_int(desc_float32(), val);
}

static inline KlrValue *klr_const_float64(double val)
{
    return klr_const_int(desc_float64(), val);
}

KlrValue *klr_const_bool(int val);
KlrValue *klr_const_string(char *s, int len);

/* <2> module */

KlrModule *klr_create_module(char *name);
void klr_destroy_module(KlrModule *m);

static inline void klr_set_name(KlrValue *val, char *name)
{
    val->name = name ? name : "";
}

KlrValue *klr_add_func(KlrModule *m, TypeDesc *ret, TypeDesc **params, char *name);
KlrValue *klr_get_param(KlrValue *fn, int index);
KlrValue *klr_add_global(KlrModule *m, TypeDesc *ty, char *name);
KlrValue *klr_add_local(KlrBuilder *bldr, TypeDesc *ty, char *name);

#define local_foreach(local, func) vector_foreach(local, &(func)->locals)

/* <3> basic block */

/* append a basic block to the end of a function */
KlrBasicBlock *klr_append_block(KlrValue *fn_val, char *name);

/* add a basic block after 'bb' */
KlrBasicBlock *klr_add_block(KlrBasicBlock *bb, char *name);

/* add a basic block before 'bb' */
KlrBasicBlock *klr_add_block_before(KlrBasicBlock *bb, char *name);

/* delete a basic block */
void klr_delete_block(KlrBasicBlock *bb);

/* add an edge */
void klr_link_edge(KlrBasicBlock *src, KlrBasicBlock *dst);
/* remove an edge */
void klr_remove_edge(KlrEdge *edge);

/* edge-out iteration */
#define edge_out_foreach(edge, bb) list_foreach(edge, out_link, &(bb)->out_edges)

/* edge-in iteration */
#define edge_in_foreach(edge, bb) list_foreach(edge, in_link, &(bb)->in_edges)

#define edge_in_foreach_reverse(edge, bb) \
    list_foreach_reverse(edge, in_link, &(bb)->in_edges)

#define edge_out_empty(bb) list_empty(&(bb)->out_edges)
#define edge_out_first(bb) list_first(&(bb)->out_edges, KlrEdge, out_link)
#define edge_out_last(bb)  list_last(&(bb)->out_edges, KlrEdge, out_link)

#define edge_in_empty(bb) list_empty(&(bb)->in_edges)
#define edge_in_first(bb) list_first(&(bb)->in_edges, KlrEdge, in_link)
#define edge_in_last(bb)  list_last(&(bb)->in_edges, KlrEdge, in_link)

/* basic block iteration */
#define basic_block_foreach(bb, fn) list_foreach(bb, link, &(fn)->bb_list)
#define basic_block_foreach_safe(bb, nxt, fn) \
    list_foreach_safe(bb, nxt, link, &(fn)->bb_list)

static inline int klr_get_nr_preds(KlrBasicBlock *bb)
{
    int nr_preds = 0;
    KlrEdge *edge;
    edge_in_foreach(edge, bb) {
        nr_preds++;
    }
    return nr_preds;
}

/* predecessor is empty? */
#define bb_pred_empty(bb) edge_in_empty(bb)

/* predecessor iteration */
#define bb_pred_foreach(pred, bb) \
    list_foreach_expr(e_, KlrEdge, in_link, &(bb)->in_edges, pred = e_->src)

static inline int klr_get_nr_succ(KlrBasicBlock *bb)
{
    int nr_succ = 0;
    KlrEdge *edge;
    edge_out_foreach(edge, bb) {
        nr_succ++;
    }
    return nr_succ;
}

/* successor is empty? */
#define bb_succ_empty(bb) edge_out_empty(bb)

/* successor iteration */
#define bb_succ_foreach(succ, bb) \
    list_foreach_expr(e_, KlrEdge, out_link, &(bb)->out_edges, succ = e_->dst)

/* get basic block label */
#define bb_label(bb) (bb)->name[0] ? (bb)->name : (bb)->tags;

/* <4> instructions */

void klr_delete_insn(KlrInsn *insn);

/* IR: %0 int = load_local %foo */
KlrValue *klr_build_load(KlrBuilder *bldr, KlrValue *var);

/* IR: store_local %var, %val */
void klr_build_store(KlrBuilder *bldr, KlrValue *var, KlrValue *val);

KlrValue *klr_build_binary(KlrBuilder *bldr, KlrValue *lhs, KlrValue *rhs, OpCode op,
                           char *name);

/* IR: %2 int = add %0, %1 */
static inline KlrValue *klr_build_add(KlrBuilder *bldr, KlrValue *lhs, KlrValue *rhs,
                                      char *name)
{
    return klr_build_binary(bldr, lhs, rhs, OP_BINARY_ADD, name);
}

/* IR: %2 int = sub %0, %1 */
static inline KlrValue *klr_build_sub(KlrBuilder *bldr, KlrValue *lhs, KlrValue *rhs,
                                      char *name)
{
    return klr_build_binary(bldr, lhs, rhs, OP_BINARY_SUB, name);
}

/* IR: %2 int = cmp %0, %1 */
KlrValue *klr_build_cmp(KlrBuilder *bldr, KlrValue *lhs, KlrValue *rhs, OpCode code,
                        char *name);

#define klr_build_cmpeq(bldr, lhs, rhs, name) \
    klr_build_cmp(bldr, lhs, rhs, OP_BINARY_CMP_EQ, name)

#define klr_build_cmpne(bldr, lhs, rhs, name) \
    klr_build_cmp(bldr, lhs, rhs, OP_BINARY_CMP_NE, name)

#define klr_build_cmplt(bldr, lhs, rhs, name) \
    klr_build_cmp(bldr, lhs, rhs, OP_BINARY_CMP_LT, name)

#define klr_build_cmpgt(bldr, lhs, rhs, name) \
    klr_build_cmp(bldr, lhs, rhs, OP_BINARY_CMP_GT, name)

#define klr_build_cmple(bldr, lhs, rhs, name) \
    klr_build_cmp(bldr, lhs, rhs, OP_BINARY_CMP_LE, name)

#define klr_build_cmpge(bldr, lhs, rhs, name) \
    klr_build_cmp(bldr, lhs, rhs, OP_BINARY_CMP_GE, name)

/* IR: br %0, %bb1, %bb2 */
void klr_build_jmp_cond(KlrBuilder *bldr, KlrValue *cond, KlrBasicBlock *_then,
                        KlrBasicBlock *_else);

/* IR: br %bb */
void klr_build_jmp(KlrBuilder *bldr, KlrBasicBlock *target);

/* IR: %0 int = call %func, %argument-list */
KlrValue *klr_build_call(KlrBuilder *bldr, KlrFunc *fn, KlrValue **args, int nargs,
                         char *name);

/* IR: ret %var */
void klr_build_ret(KlrBuilder *bldr, KlrValue *ret);

/* instruction iteration */
#define insn_foreach(insn, bb) list_foreach(insn, bb_link, &(bb)->insn_list)
#define insn_foreach_safe(insn, next, bb) \
    list_foreach_safe(insn, next, bb_link, &(bb)->insn_list)
#define insn_first(bb) list_first(&(bb)->insn_list, KlrInsn, bb_link)
#define insn_last(bb)  list_last(&(bb)->insn_list, KlrInsn, bb_link)

/* def-use iteration */
#define use_foreach(use, val) list_foreach(use, use_link, &(val)->use_list)
#define use_foreach_safe(use, next, val) \
    list_foreach_safe(use, next, use_link, &(val)->use_list)
#define use_first(val) list_first(&(val)->use_list, KlrUse, use_link)
#define use_last(val)  list_last(&(val)->use_list, KlrUse, use_link)

/* operand iteration */
#define operand_foreach(oper, insn) \
    for (int i__ = 0; (i__ < (insn)->num_opers) && (oper = &(insn)->opers[i__], 1); i__++)

// clang-format off

#define insn_operand(insn, i) ({ \
    ASSERT((i) >= 0 && (i) < (insn)->num_opers); \
    (insn)->opers + i; \
})

#define insn_operand_value(insn, i) ({ \
    KlrOper *oper = insn_operand(insn, i); \
    oper->use.ref; \
})

// clang-format on

/* invalidate i-th operand and delete this value if it's not used */
void invalidate_insn_operand(KlrInsn *insn, int i);

/* update operand */
void update_insn_operand(KlrInsn *insn, int i, KlrValue *val);

/* check value is used or not */
#define klr_value_used(val) (!list_empty(&(val)->use_list))

/* <5> printer */

/* print an instruction */
void klr_print_insn(KlrInsn *insn, FILE *fp);

/* print a function */
void klr_print_func(KlrFunc *func, FILE *fp);

/* print a CFG of a function */
void klr_print_cfg(KlrFunc *func, FILE *fp);

/* print a module */
void klr_print_module(KlrModule *m, FILE *fp);

/* dump a module to stdout */
#define klr_dump_module(m) \
    klr_print_module(m, stdout); \
    fflush(stdout);

/* <6> instruction builder */

/* clang-format off */

/* set builder at head */
#define klr_builder_head(bldr, _bb) do { \
    (bldr)->bb = (_bb); \
    (bldr)->it = &(_bb)->insn_list; \
} while (0)

/* set builder at and */
#define klr_builder_end(bldr, _bb) do { \
    (bldr)->bb = (_bb); \
    (bldr)->it = (_bb)->insn_list.prev; \
} while (0)

/* set builder at 'insn' */
#define klr_builder_at(bldr, insn) do { \
    (bldr)->bb = (insn)->bb; \
    (bldr)->it = &(insn)->bb_link; \
} while (0)

/* set builder before 'insn' */
#define klr_builder_before(bldr, insn) do { \
    (bldr)->bb = (insn)->bb; \
    (bldr)->it = (insn)->bb_link.prev; \
} while (0)

/* clang-format on */

/* <7> pass */

typedef void (*KlrPassFunc)(KlrFunc *fn, void *arg);

typedef struct _KlrPassGroup {
    List passes;
} KlrPassGroup;

/* define a pass group */
#define KLR_PASS_GROUP(name) KlrPassGroup name = { .passes = LIST_INIT(name.passes) };

/* finalize a pass group */
void klr_fini_pass_group(KlrPassGroup *grp);

/* register one pass */
void klr_add_pass(KlrPassGroup *grp, char *name, KlrPassFunc fn, void *arg);

/* execute pass group */
void klr_run_pass_group(KlrPassGroup *grp, KlrFunc *fn);

/* ir context */
typedef struct _KlrContext {
    HashMap passes;
    Vector modules;
} KlrContext;

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_IR_H_ */

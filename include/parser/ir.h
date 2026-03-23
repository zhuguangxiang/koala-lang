/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_IR_H_
#define _KOALA_IR_H_

#include "hashmap.h"
#include "list.h"
#include "opcode.h"
#include "typespec.h"
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
    KLR_VALUE_INSN,
    KLR_VALUE_KLASS,
    KLR_VALUE_FIELD,
    KLR_VALUE_EXT_FUNC,
    KLR_VALUE_EXT_GLOBAL,
    KLR_VALUE_MAX,
} KlrValueKind;

// 'print_name' is final printed name.
// The 'tag' and 'name' are used to generate 'print_name'.

/* clang-format off */
#define KLR_VALUE_HEAD      \
    KlrValueKind kind;      \
    /* value type */        \
    TypeSpec *ts;           \
    /* list of all Uses */  \
    List use_list;          \
    /* list of all defs */  \
    List def_list;          \
    /* use count */         \
    int use_count;          \
    /* def count */         \
    int def_count;          \
    /* virtual register */  \
    int vreg;               \
    /* tag */               \
    int tag;                \
    /* name */              \
    char *name;             \
    /* print name */        \
    char print_name[64];
/* clang-format on */

typedef struct _KlrValue {
    KLR_VALUE_HEAD
} KlrValue;

#define INIT_KLR_VALUE(val, _kind, _ts, _name) \
    (val)->kind = (_kind); \
    (val)->ts = (_ts); \
    init_list(&(val)->use_list); \
    init_list(&(val)->def_list); \
    (val)->use_count = 0; \
    (val)->def_count = 0; \
    (val)->vreg = -1; \
    (val)->tag = -1; \
    (val)->name = _name ? _name : "";

/* literal constant */
typedef struct _KlrConst {
    KLR_VALUE_HEAD
    HashMapEntry hnode;
    int index;
    int which;
#define CONST_INT   1
#define CONST_UINT  2
#define CONST_FLT   3
#define CONST_BOOL  4
#define CONST_STR   5
#define CONST_LIST  6
#define CONST_TUPLE 7
#define CONST_NONE  8
    int len;
    union {
        uint64_t ival;
        double fval;
        int bval;
        char *sval;
        struct {
            KlrValue **items;
        } list;
    };
} KlrConst;

/* global/field variable */
typedef struct _KlrVar {
    KLR_VALUE_HEAD
    int mutable;
    KlrConst *kval;
} KlrGlobal, KlrField;

/* function parameter */
typedef struct _KlrParam {
    KLR_VALUE_HEAD
} KlrParam;

struct _KlrKlass;

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

    /* start basic block */
    struct _KlrBasicBlock *sbb;
    /* end basic block */
    struct _KlrBasicBlock *ebb;

    /* module pointer */
    struct _KlrModule *module;

    /* klass pointer */
    struct _KlrKlass *klass;

    /* number of virtual registers used */
    int num_vregs;

    /* LIR (Lowered IR) generation pipeline:
     *
     *   1. VReg assignment:
     *        Assign a unique virtual register to every SSA value.
     *
     *   2. Lowering:
     *        Instruction selection, pattern fusion, instruction splitting,
     *        and conversion from SSA IR to linear LIR.
     *
     *   3. Register allocation:
     *        Map vregs to physical registers (LSRA / graph coloring / simple
     * RA), inserting spills and reloads when necessary.
     *
     *   4. Code emission:
     *        Patch jump offsets, apply WIDE expansion, and encode final VM
     * bytecode.
     */
    Vector lir;
} KlrFunc;

/* basic block */
typedef struct _KlrBasicBlock {
    KLR_VALUE_HEAD

    /* linked in KlrFunc */
    List link;

    /* ->KlrFunc(parent) */
    KlrFunc *func;

    /* number of instructions */
    int num_insns;

    /* instructions */
    List insn_list;

    /* block visited flag */
    int visited;

    /* number of in-edges */
    int num_inedges;
    /* number of out-edges */
    int num_outedges;

    /* in edges(predecessors) */
    List in_edges;
    /* out edges(successors) */
    List out_edges;

    /* optimization/register allocation related */

    /* local variable constant map */
    HashMap local_var_map;

    /**
     * The position of this block in the Reverse Post-Order (RPO) sequence.
     * Used for back-edge detection: if (target->index <= current->index),
     * it's a loop back-edge.
     */
    int index;

    /**
     * The linear position (pos) of the FIRST instruction in this block.
     * Used as the starting anchor for live interval backward scanning.
     */
    int first_pos;

    /**
     * The linear position (pos) of the LAST instruction in this block.
     * Crucial for determining if a value is live-out of this block.
     */
    int last_pos;

    /**
     * Flag: True if this block is the entry point (header) of a loop.
     * Identified when a predecessor is a back-edge (index >= header->index).
     */
    int is_loop_header;

    /**
     * Flag: True if this block ends with a jump to an earlier block in RPO.
     * This marks the "latch" or "exit" point of a loop body.
     */
    int has_back_edge;

    /* backend-private pointer */
    void *mach;
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
    /* external symbols */
    Vector ext_syms;
    /* __init__ function */
    KlrFunc *init;
    /* klasses */
    Vector klasses;
    /* constant map */
    HashMap consts;
    /* constant next available index */
    int const_next;
} KlrModule;

typedef struct _KlrKlass {
    KLR_VALUE_HEAD
    KlrModule *module;
    Vector fields;
    Vector methods;
} KlrKlass;

#define KLR_EXT_SYM_HEAD \
    KLR_VALUE_HEAD \
    /* module pointer */ \
    KlrModule *module; \
    /* owner pkg path */ \
    char *path;

/* external symbol */
typedef struct _KlrExtSym {
    KLR_EXT_SYM_HEAD
} KlrExtSym;

/* external function */
typedef struct _KlrExtFunc {
    KLR_EXT_SYM_HEAD
    /* proto */
    TypeSpec *proto;
} KlrExtFunc;

typedef struct _KlrExtGlobal {
    KLR_EXT_SYM_HEAD
} KlrExtGlobal;

/* def-use */
typedef struct _KlrUse {
    /* def-value(use_list) */
    KlrValue *ref;
    /* link in use_list */
    List use_link;

    /* back point to insn */
    struct _KlrInsn *insn;
    /* back point to oper */
    struct _KlrOper *oper;

    /* true if this is a write(Def), false if it is a read(Use) */
    int is_def;
} KlrUse;

/* phi operand */
typedef struct _KlrPhiParam {
    KlrBasicBlock *bb;
    List bb_link;
} KlrPhiParam;

/* operand */
typedef struct _KlrOper {
    KlrUse use;
    KlrPhiParam *phi;
} KlrOper;

typedef enum {
    RAW_OPER_NONE,
    RAW_OPER_REG,
    RAW_OPER_INDEX,
    RAW_OPER_IMM,
    RAW_OPER_CONST,
    RAW_OPER_BLOCK,
    RAW_OPER_GLOBAL,
    RAW_OPER_FUNC,
} KlrRawOperKind;

/* flatten operand */
typedef struct _KlrRawOper {
    KlrRawOperKind kind;

    union {
        int index;
        int imm;
        KlrConst *kval;
        KlrBasicBlock *bb;
        int global_index;
        int func_index;
    };
} KlrRawOper;

#define KLR_INSN_FLAGS_CONST 1
#define KLR_INSN_FLAGS_DEAD  2

/* instruction */
typedef struct _KlrInsn {
    KLR_VALUE_HEAD

    /* opcode */
    OpCode code;

    /**
     * Linear position (coordinate) in the RPO sequence.
     * Serves as the time-axis for Live Interval analysis [start, end].
     */
    int pos;

    /* instruction flags */
    int flags;

    /* number of arguments for OP_CALL */
    int num_args;

    /* filled phi parameter index */
    int filled;

    /* link in bb */
    List bb_link;
    /* ->bb */
    KlrBasicBlock *bb;

    /* phi variable */
    KlrValue *phi;

    /* Raw operands produced by isel and consumed by mach/emit. */
    KlrRawOper raw_opers[3];

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

/* Set operand to use the instruction's own result vreg. */
static inline void set_raw_oper_reg(KlrInsn *insn, int slot)
{
    KlrRawOper *op = &insn->raw_opers[slot];
    op->kind = RAW_OPER_REG;
    /* RAW_OPER_REG carries no payload. */
}

/* Set operand to reference an input operand by index. */
static inline void set_raw_oper_index(KlrInsn *insn, int slot, int index)
{
    KlrRawOper *op = &insn->raw_opers[slot];
    op->kind = RAW_OPER_INDEX;
    op->index = index; /* Index into insn->operands[]. */
}

/* Set operand to an empty slot. */
static inline void set_raw_oper_none(KlrInsn *insn, int slot)
{
    KlrRawOper *op = &insn->raw_opers[slot];
    op->kind = RAW_OPER_NONE;
}

/* Set operand to an immediate value. */
static inline void set_raw_oper_imm(KlrInsn *insn, int slot, int imm)
{
    KlrRawOper *op = &insn->raw_opers[slot];
    op->kind = RAW_OPER_IMM;
    op->imm = imm; /* Immediate value; encoding is handled in emit. */
}

/* Set operand to a constant pool entry. */
static inline void set_raw_oper_const(KlrInsn *insn, int slot, KlrConst *kval)
{
    KlrRawOper *op = &insn->raw_opers[slot];
    op->kind = RAW_OPER_CONST;
    op->kval =
        kval; /* Constant pool entry; index assigned during linearization. */
}

/* Set operand to a basic block target. */
static inline void set_raw_oper_block(KlrInsn *insn, int slot,
                                      KlrBasicBlock *bb)
{
    KlrRawOper *op = &insn->raw_opers[slot];
    op->kind = RAW_OPER_BLOCK;
    op->bb = bb; /* Basic block target; mapped to label during linearization. */
}

/* <1> literal constants */
KlrValue *klr_const_int(uint64_t val, TypeSpec *ts, KlrModule *m);
KlrValue *klr_const_uint(uint64_t val, TypeSpec *ts, KlrModule *m);
KlrValue *klr_const_float(double val, TypeSpec *ts, KlrModule *m);
KlrValue *klr_const_bool(int val, KlrModule *m);
KlrValue *klr_const_str(char *s, int len, KlrModule *m);
KlrValue *klr_const_list(KlrValue **items, int size, TypeSpec *ts,
                         KlrModule *m);
KlrValue *klr_const_tuple(KlrValue **items, int size, TypeSpec *ts,
                          KlrModule *m);
KlrValue *klr_const_none(KlrModule *m);

static inline int klr_is_const(KlrValue *val)
{
    if (val->kind == KLR_VALUE_CONST) return 1;
    return 0;
}

static inline int klr_is_insn(KlrValue *val)
{
    if (val->kind == KLR_VALUE_INSN) return 1;
    return 0;
}

static inline int klr_is_local(KlrValue *val)
{
    if (!klr_is_insn(val)) return 0;

    KlrInsn *insn = (KlrInsn *)val;
    return insn->code == OP_IR_LOCAL;
}

static inline int klr_is_param(KlrValue *val)
{
    if (val->kind == KLR_VALUE_PARAM) return 1;
    return 0;
}

static inline int klr_is_block(KlrValue *val)
{
    if (val->kind == KLR_VALUE_BLOCK) return 1;
    return 0;
}

static inline int klr_is_func(KlrValue *val)
{
    if (val->kind == KLR_VALUE_FUNC) return 1;
    return 0;
}

int klr_is_immutable(KlrValue *val);

static inline int insn_is_dead(KlrInsn *insn)
{
    if (insn->flags & KLR_INSN_FLAGS_DEAD) return 1;
    return 0;
}

static inline int klr_is_global(KlrValue *val)
{
    if (val->kind == KLR_VALUE_GLOBAL) return 1;
    return 0;
}

/* <2> module */

KlrModule *klr_create_module(char *name);
void klr_destroy_module(KlrModule *m);

KlrValue *klr_add_func(KlrModule *m, TypeSpec *ret, char *name);
KlrValue *klr_func_get_param(KlrValue *val, int index);
KlrValue *klr_func_add_param(KlrValue *val, TypeSpec *ts, char *name);

#define param_foreach(param, func) vector_foreach(param, &(func)->params)

KlrValue *klr_add_global(KlrModule *m, TypeSpec *ts, char *name, int mut);
KlrValue *klr_add_klass(KlrModule *m, TypeSpec *ts, char *name);
KlrValue *klr_klass_add_field(KlrValue *klass, char *name, TypeSpec *ts);
KlrValue *klr_klass_add_method(KlrValue *klass, char *name, TypeSpec *ret,
                               TypeSpec **params);

// ir doesn't check external symbol's type
KlrValue *klr_add_ext_func(KlrModule *m, TypeSpec *proto, char *path,
                           char *name);
KlrValue *klr_add_ext_global(KlrModule *m, TypeSpec *ts, char *path,
                             char *name);

#define local_foreach(local, func) vector_foreach_ptr(local, &(func)->locals)

/* <3> basic block */

/* append a basic block to the end of a function */
KlrBasicBlock *klr_append_block(KlrValue *fn_val, char *name);

/* add a basic block after 'bb' */
KlrBasicBlock *klr_add_block(KlrBasicBlock *bb, char *name);

/* add a basic block before 'bb' */
KlrBasicBlock *klr_add_block_before(KlrBasicBlock *bb, char *name);

/* erase a basic block */
void klr_erase_block(KlrBasicBlock *bb);

/* merge src' into 'dst',
if 'dst' has only one successor of 'src' and 'src' has only one predecessor of
'dst' The caller must check the condition and 'src' is not removed.
*/
void Klr_merge_block(KlrBasicBlock *dst, KlrBasicBlock *src);

/* update local variable */
int klr_update_local_var(KlrBasicBlock *bb, KlrInsn *local, KlrValue *val);

/* clear local variable */
int klr_clear_local_var(KlrBasicBlock *bb, KlrInsn *local);

/* get local variable */
KlrValue *klr_get_local_var(KlrBasicBlock *bb, KlrInsn *local);

/* clear all local variables */
int klr_clear_local_var_map(KlrBasicBlock *bb);

/* check block has terminator or not */
int block_has_terminator(KlrBasicBlock *bb);

/* add an edge */
void klr_link_edge(KlrBasicBlock *src, KlrBasicBlock *dst);

/* remove an edge */
void klr_remove_edge(KlrEdge *edge);

/* remove all out edges of a basic block */
void klr_remove_all_out_edges(KlrBasicBlock *bb);

/* edge-out iteration */
#define edge_out_foreach(edge, bb) \
    list_foreach(edge, out_link, &(bb)->out_edges)

/* edge-out safe iteration */
#define edge_out_foreach_safe(edge, nxt, bb) \
    list_foreach_safe(edge, nxt, out_link, &(bb)->out_edges)

#define edge_out_foreach_reverse(edge, bb) \
    list_foreach_reverse(edge, out_link, &(bb)->out_edges)

/* edge-in iteration */
#define edge_in_foreach(edge, bb) list_foreach(edge, in_link, &(bb)->in_edges)

/* edge-in safe iteration */
#define edge_in_foreach_safe(edge, nxt, bb) \
    list_foreach_safe(edge, nxt, in_link, &(bb)->in_edges)

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

#define bb_foreach_reverse(bb, fn) \
    list_foreach_reverse(bb, link, &(fn)->bb_list)

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

/* <4> instructions */

void klr_append_insn(KlrBuilder *bldr, KlrInsn *insn);

void klr_erase_insn(KlrInsn *insn);

/* check an ir needs allocate register or not */
int ir_has_value(KlrInsn *insn);

/* IR: %0 = local int [immutable] */
KlrValue *klr_build_local(KlrBuilder *bldr, TypeSpec *ts, char *name);

/* IR: %0 = local float [mutable] */
KlrValue *klr_build_local_var(KlrBuilder *bldr, TypeSpec *ts, char *name);

/* IR: %0 = get_global %global */
KlrValue *klr_build_get_global(KlrBuilder *bldr, KlrValue *global);

/* IR: set_global %global, %src */
void klr_build_set_global(KlrBuilder *bldr, KlrValue *global, KlrValue *val);

/* IR: move %dst, %src */
void klr_build_move(KlrBuilder *bldr, KlrValue *var, KlrValue *val);

KlrValue *klr_build_binary(KlrBuilder *bldr, KlrValue *lhs, KlrValue *rhs,
                           OpCode op, char *name, const char *op_name);

/* IR: %2 int = add %0, %1 */
static inline KlrValue *klr_build_add(KlrBuilder *bldr, KlrValue *lhs,
                                      KlrValue *rhs, char *name)
{
    return klr_build_binary(bldr, lhs, rhs, OP_BINARY_ADD, name, "add");
}

/* IR: %2 int = sub %0, %1 */
static inline KlrValue *klr_build_sub(KlrBuilder *bldr, KlrValue *lhs,
                                      KlrValue *rhs, char *name)
{
    return klr_build_binary(bldr, lhs, rhs, OP_BINARY_SUB, name, "sub");
}

/* IR: %2 int = cmp %0, %1 */
KlrValue *klr_build_cmp(KlrBuilder *bldr, KlrValue *lhs, KlrValue *rhs,
                        OpCode code, char *name);

#define klr_build_cmpeq(bldr, lhs, rhs, name) \
    klr_build_cmp(bldr, lhs, rhs, OP_BINARY_CMPEQ, name)

#define klr_build_cmpne(bldr, lhs, rhs, name) \
    klr_build_cmp(bldr, lhs, rhs, OP_BINARY_CMPNE, name)

#define klr_build_cmplt(bldr, lhs, rhs, name) \
    klr_build_cmp(bldr, lhs, rhs, OP_BINARY_CMPLT, name)

#define klr_build_cmpgt(bldr, lhs, rhs, name) \
    klr_build_cmp(bldr, lhs, rhs, OP_BINARY_CMPGT, name)

#define klr_build_cmple(bldr, lhs, rhs, name) \
    klr_build_cmp(bldr, lhs, rhs, OP_BINARY_CMPLE, name)

#define klr_build_cmpge(bldr, lhs, rhs, name) \
    klr_build_cmp(bldr, lhs, rhs, OP_BINARY_CMPGE, name)

/* IR: br %0, %bb1, %bb2 */
void klr_build_jmp_cond(KlrBuilder *bldr, KlrValue *cond, KlrBasicBlock *_then,
                        KlrBasicBlock *_else);

/* IR: br %bb */
void klr_build_jmp(KlrBuilder *bldr, KlrBasicBlock *target);

/* IR: %0 int = call %func, %argument-list */
KlrValue *klr_build_call(KlrBuilder *bldr, KlrValue *fn, KlrValue **args,
                         int nargs, char *name);

/* IR: ret %var */
void klr_build_ret(KlrBuilder *bldr, KlrValue *ret);

/* IR: ret */
void klr_build_ret_void(KlrBuilder *bldr);

/* IR: push %var */
KlrInsn *klr_build_push(KlrBuilder *bldr, KlrValue *val);

/* IR: push_int_imm %var */
KlrValue *klr_build_push_int_imm(KlrBuilder *bldr, KlrValue *val);

/* IR: push_bool %var */
KlrValue *klr_build_push_bool(KlrBuilder *bldr, KlrValue *val);

/* IR: push_const cp-offset */
KlrValue *klr_build_push_const(KlrBuilder *bldr, KlrValue *val);

/* add a return instruction at the end of a basic block if it doesn't have one
 */
void klr_add_last_return(KlrBasicBlock *bb);

/* IR: %0 = const.int */
KlrInsn *klr_build_const_int(KlrBuilder *bldr, KlrValue *var, KlrValue *val);

/* IR: %0 = const.load %var */
KlrInsn *klr_build_const_load(KlrBuilder *bldr, KlrValue *var, KlrValue *val);

/* instruction iteration */
#define insn_foreach(insn, bb) list_foreach(insn, bb_link, &(bb)->insn_list)

#define insn_foreach_safe(insn, next, bb) \
    list_foreach_safe(insn, next, bb_link, &(bb)->insn_list)

#define insn_foreach_reverse(insn, bb) \
    list_foreach_reverse(insn, bb_link, &(bb)->insn_list)

#define insn_foreach_reverse_safe(insn, next, bb) \
    list_foreach_reverse_safe(insn, next, bb_link, &(bb)->insn_list)

#define insn_first(bb) list_first(&(bb)->insn_list, KlrInsn, bb_link)
#define insn_last(bb)  list_last(&(bb)->insn_list, KlrInsn, bb_link)

/* def-use iteration */
#define use_foreach(use, val) list_foreach(use, use_link, &(val)->use_list)

#define use_foreach_safe(use, next, val) \
    list_foreach_safe(use, next, use_link, &(val)->use_list)

#define use_first(val) list_first(&(val)->use_list, KlrUse, use_link)
#define use_last(val)  list_last(&(val)->use_list, KlrUse, use_link)

/* operand & use */

// clang-format off

#define oper_value(oper) ((oper)->use.ref)

#define insn_operand(insn, i) ({ \
    ASSERT((i) >= 0 && (i) < (insn)->num_opers); \
    (insn)->opers + i; \
})

#define insn_oper_value(insn, i) ({ \
    KlrOper *oper = insn_operand(insn, i); \
    oper_value(oper); \
})

/* insn->oper.use iteration */
#define insn_oper_use_foreach(use, insn) \
    for (int i__ = 0; (i__ < (insn)->num_opers) && (use = &(insn)->opers[i__].use, 1); i__++)

#define _insn_oper_value_foreach(val, insn, start) \
    for (int i__ = (start); (i__ < (insn)->num_opers) && (val = insn_oper_value(insn, i__), 1); i__++)

/* insn->oper.use.ref iteration */
#define insn_oper_value_foreach(val, insn) _insn_oper_value_foreach(val, insn, 0)

// clang-format on

/* replace all uses of 'def' value with 'val' value */
void replace_all_uses_with(KlrValue *val, KlrValue *def);

/* set/clear operand */
void set_operand_at(KlrInsn *insn, int i, KlrValue *val);
void set_operand(KlrOper *oper, KlrInsn *insn, KlrValue *val);
void clear_operand(KlrOper *oper);
void clear_operand_at(KlrInsn *insn, int i);

/* check value is used or not */
#define klr_is_used(val) (!list_empty(&(val)->use_list))

/* <5> printer */

/* get value printed name */
char *klr_value_name(KlrValue *val);

/* get basic block printed name */
char *klr_block_name(KlrBasicBlock *bb);

/* print a value's name */
static inline void klr_print_value_name(KlrValue *val, FILE *fp)
{
    fprintf(fp, "%s", klr_value_name(val));
}

/* update tags and then print */
void update_tags(KlrFunc *fn);

/* print an instruction */
void klr_print_insn(KlrInsn *insn, FILE *fp);

/* print a function */
void klr_print_func(KlrFunc *func, FILE *fp);

/* print a module */
void klr_print_module(KlrModule *m, FILE *fp);

/* dump a module to stdout */
#define klr_dump_module(m) \
    klr_print_module(m, stdout); \
    fflush(stdout);

// clang-format off
#ifndef NOLOG
#define log_insn(insn) do { \
    klr_print_insn(insn, stderr); \
    putc('\n', stderr); \
} while (0)
#else
#define log_insn(insn) ((void)0)
#endif
// clang-format on

void klr_insn_remap(KlrFunc *func);
void klr_alloc_registers(KlrFunc *func);
void klr_simple_alloc_registers(KlrFunc *func);

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

/* Reverse Post Order */
void klr_build_rpo(KlrFunc *fn);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_IR_H_ */

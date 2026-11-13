/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_GVN_H_
#define _KOALA_GVN_H_

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Global value numbering on a minimal, IR independent SSA form.
 *
 * Two algorithms are provided, both from
 *
 *   Preston Briggs, Keith D. Cooper, L. Taylor Simpson,
 *   "Value Numbering",
 *   Software Practice & Experience, 27(6):701-724, June 1997.
 *   https://www.cs.tufts.edu/comp/150FP/archive/keith-cooper/value-numbering.pdf
 *
 *   - gvn_dvnt: dominator-based value numbering technique (DVNT), see
 *     gvn_dvnt.c. Pessimistic, a single walk over the dominator tree.
 *   - gvn_rpo: the optimistic reverse post-order (RPO) algorithm of
 *     L. Taylor Simpson, "Value-Driven Redundancy Elimination",
 *     PhD thesis, Rice University, 1996, see gvn_rpo.c. Iterates to a fixed
 *     point and also finds congruences carried around loops.
 *
 * The function is given in SSA form:
 *   - blocks are `0 .. num_blocks - 1`, edges are `(src, dst)` pairs;
 *   - instructions are `0 .. num_insns - 1`, an instruction's id is its value;
 *   - the instructions of block `b` are contiguous, in program order,
 *     `[block_start[b], block_start[b + 1])`, with all PHIs first;
 *   - the k-th argument of a PHI in block `b` flows in along the k-th edge
 *     (in `edges` order) whose destination is `b`;
 *   - every non-PHI argument dominates its use, every PHI argument dominates
 *     the end of its predecessor.
 *
 * The result is `replace[i]`: `i` itself if it is kept, otherwise a kept
 * instruction with the same value that dominates `i`. Rewriting every use of
 * `i` with `replace[i]` makes `i` dead. Instructions in blocks unreachable from
 * entry are always kept, and edges from unreachable blocks are ignored.
 */

typedef enum _GvnOp {
    /* constant, `imm` is its value */
    GVN_CONST,
    /* function parameter, `imm` is its index; same index, same value */
    GVN_PARAM,
    /* unknown value (call, load, ...), never congruent to anything else */
    GVN_OPAQUE,
    /* SSA phi */
    GVN_PHI,
    /* binary operators on int64, wrapping */
    GVN_ADD,
    GVN_SUB,
    GVN_MUL,
    GVN_AND,
    GVN_OR,
    GVN_XOR,
    /* comparisons, 1 or 0 */
    GVN_EQ,
    GVN_LT,
} GvnOp;

typedef struct _GvnInsn {
    GvnOp op;
    /* CONST value, PARAM index */
    int64_t imm;
    /* 2 for binary operators, the number of in-edges for PHI */
    int num_args;
    /* argument instruction ids */
    int *args;
} GvnInsn;

typedef struct _GvnFunc {
    int num_blocks;
    int entry;
    int num_edges;
    /* (src, dst) pairs */
    const int *edges;
    int num_insns;
    const GvnInsn *insns;
    /* num_blocks + 1 entries */
    const int *block_start;
} GvnFunc;

/* Dominator-based value numbering. Returns the number of replaced insns. */
int gvn_dvnt(const GvnFunc *fn, int *replace);

/* Optimistic RPO value numbering. Returns the number of replaced insns. */
int gvn_rpo(const GvnFunc *fn, int *replace);

static inline int gvn_is_binary(GvnOp op) { return op >= GVN_ADD; }

static inline int gvn_is_commutative(GvnOp op)
{
    return op == GVN_ADD || op == GVN_MUL || op == GVN_AND || op == GVN_OR || op == GVN_XOR ||
           op == GVN_EQ;
}

/* Evaluate a binary operator, shared by constant folding and interpreters. */
static inline int64_t gvn_eval(GvnOp op, int64_t a, int64_t b)
{
    uint64_t x = (uint64_t)a;
    uint64_t y = (uint64_t)b;
    switch (op) {
        case GVN_ADD:
            return (int64_t)(x + y);
        case GVN_SUB:
            return (int64_t)(x - y);
        case GVN_MUL:
            return (int64_t)(x * y);
        case GVN_AND:
            return (int64_t)(x & y);
        case GVN_OR:
            return (int64_t)(x | y);
        case GVN_XOR:
            return (int64_t)(x ^ y);
        case GVN_EQ:
            return a == b;
        case GVN_LT:
            return a < b;
        default:
            ASSERT(0);
            return 0;
    }
}

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_GVN_H_ */

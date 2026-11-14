/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_INLINE_COST_H_
#define _KOALA_INLINE_COST_H_

#include "ir.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RECURSIVE_INLINE_MAX_DEPTH  8
#define RECURSIVE_INLINE_MAX_CALLS  2
#define RECURSIVE_INLINE_MAX_SITES  ((1 << (RECURSIVE_INLINE_MAX_DEPTH + 1)) - 2)
#define RECURSIVE_INLINE_PROB_SCALE 10000

typedef struct _RecursiveInlineCost {
    long long size;
    long long size_per_copy;
    int blocks;
    int blocks_per_copy;
} RecursiveInlineCost;

void recursive_inline_cost_init(RecursiveInlineCost *cost, int insns, int returns, int params,
                                int blocks);
int recursive_inline_cost_allow_level(RecursiveInlineCost *cost, int depth, int calls);
void recursive_inline_call_probabilities(KlrFunc *fn, KlrInsn **calls, int count,
                                         int *probabilities);
int recursive_inline_probability_product(int parent, int callee);
int recursive_inline_probability_allowed(int probability);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_INLINE_COST_H_ */

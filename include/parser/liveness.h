/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_LIVENESS_H_
#define _KOALA_LIVENESS_H_

#include "lsra.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Build the live ranges of the params and insns of ctx->func into ctx->intervals, in the order
 * params then insns by pos. Each value gets an id in its tag (ctx->vals, ctx->nids) and
 * ctx->imap maps the id to its interval. The insn positions must be assigned.
 */
void klr_build_live_ranges(KlrLSRAContext *ctx);

/* the interval of a value, or NULL if it has none */
KlrInterval *klr_value_interval(KlrLSRAContext *ctx, KlrValue *val);

/* add the ranges of `src` to `dst` */
void klr_interval_union(KlrInterval *dst, KlrInterval *src);

/* add [start, end) to `intv` */
void klr_interval_add(KlrInterval *intv, int start, int end);

/* true if the two intervals are live at a common position */
int klr_interval_intersects(KlrInterval *a, KlrInterval *b);

void klr_interval_fini(KlrInterval *intv);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_LIVENESS_H_ */

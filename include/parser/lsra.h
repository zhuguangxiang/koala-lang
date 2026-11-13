/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_LSRA_H_
#define _KOALA_LSRA_H_

#include "bitset.h"
#include "ir.h"
#include "pass.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_REGS 256

/* single range, the span from first definition to last use */
/* a half-open live range [start, end) */
typedef struct _KlrRange {
    int start;
    int end;
} KlrRange;

typedef struct _KlrInterval {
    /* The associated IR value (Insn/Param) */
    KlrValue *val;

    /* start of the first range */
    int start;
    /* end of the last range */
    int end;

    /* sorted, disjoint and non-adjacent live ranges; the gaps are holes where the register
     * holds nothing and may be given to another interval */
    KlrRange *ranges;
    int nranges;
    int cap;
    /* index of the range the scan is at */
    int cursor;

    /* the allocated flag used to control free register */
    int allocated;

    /* take the register of this value if it is free, so a move or cast between them is dropped */
    KlrValue *hint;
    /* share the register of this local, whose value is dead over the interval */
    KlrValue *alias;
} KlrInterval;

/* LSRA Context for liveness analysis and register mapping */
typedef struct _KlrLSRAContext {
    // the function being processed
    KlrFunc *func;
    // all intervals for insns and params in function
    Vector intervals;
    // bitset to track free/used registers, 1 means free, 0 means used
    BitSet bitset;
    /* last postion */
    int last_pos;
    /* fixedslot insns */
    Vector fixed;
    /* the values that may get a register, indexed by the id kept in their tag */
    KlrValue **vals;
    int nids;
    /* id -> index of its interval, -1 if none */
    int *imap;
} KlrLSRAContext;

void kl_do_lsra(KlrModule *m);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_LSRA_H_ */

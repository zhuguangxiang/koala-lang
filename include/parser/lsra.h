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
typedef struct _KlrInterval {
    /* The associated IR value (Insn/Param) */
    KlrValue *val;

    /* First definition position (pos) */
    int start;
    /* Last use position (pos) */
    int end;

    /* the allocated flag used to control free register */
    int allocated;
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
} KlrLSRAContext;

void kl_do_lsra(KlrModule *m);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_LSRA_H_ */

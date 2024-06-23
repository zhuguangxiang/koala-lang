/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "eval.h"
#include "mm.h"
#include "run.h"

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------DATA-----------------------------------*/

/* max stack size */
#define MAX_STACK_SIZE (64 * 1024)

/* max call depth, stop for this limit */
#define MAX_CALL_DEPTH 10000

/*-------------------------------------API-----------------------------------*/

/* forward declaration */
int _eval_frame(KoalaState *ks, CallFrame *cf);

static CallFrame *_new_frame(KoalaState *ks)
{
    CallFrame *cf;
    if (ks->free_cf_list) {
        cf = ks->free_cf_list;
        ks->free_cf_list = cf->cf_back;
    } else {
        cf = mm_alloc_obj_fast(cf);
    }

    cf->cf_local_stack = ks->stack_ptr;
    // ks->stack_ptr += (nlocals + stack_size);
    // ASSERT(ks->stack_ptr < ks->base_stack_ptr + ks->ks_stack_size);

    return cf;
}

static void _free_frame(KoalaState *ks, CallFrame *cf) {}

static void _pop_frame(KoalaState *ks, CallFrame *cf)
{
    /* shrink stack */
    ks->stack_ptr -= (cf->cf_stack_size + cf->cf_local_size);
    ASSERT(ks->stack_ptr >= ks->base_stack_ptr);

    /* save to cached free list */
    cf->cf_back = ks->free_cf_list;
    ks->free_cf_list = cf;
}

KoalaState *ks_new(void)
{
    KoalaState *ks = mm_alloc_obj_fast(ks);
    lldq_node_init(&ks->link);
    ks->ts = __ts();

    ks->base_stack_ptr = mm_alloc_fast(MAX_STACK_SIZE * sizeof(Value));
    ks->stack_ptr = ks->base_stack_ptr;
    ks->stack_size = MAX_STACK_SIZE;

    return ks;
}

void ks_free(KoalaState *ks)
{
    ASSERT(!ks->cf);
    CallFrame *cf = ks->free_cf_list;
    CallFrame *next;
    while (cf) {
        next = cf->cf_back;
        _free_frame(ks, cf);
    }
    mm_free(ks->base_stack_ptr);
    mm_free(ks);
}

int _eval_frame(KoalaState *ks, CallFrame *cf)
{
    int retval = 0;
    int op;

    /* push frame */
    cf->cf_back = ks->cf;
    cf->cf_ks = ks;
    ks->cf = cf;
    ++ks->depth;

    ASSERT(ks->depth <= MAX_CALL_DEPTH);

main_loop:
    for (;;) {
    dispatch:
        // NEXT_OP();
    dispatch_opcode:
        switch (op) {
            case 1:
                /* code */
                break;

            default:
                break;
        } /* switch */

        /* This should never be reached here! */
        UNREACHABLE();

    error:

        // ASSERT(_err_occurred(ks));

        /* log traceback info */

        /* finish the loop as we have an error. */
        break;
    } /* main_loop */

    ASSERT(0);
    // ASSERT(_err_occurred(ks));

    /* pop frame */
    ks->cf = cf->cf_back;
    --ks->depth;
    return retval;
}

#ifdef __cplusplus
}
#endif

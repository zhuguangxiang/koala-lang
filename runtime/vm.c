/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "vm.h"
#include "opcode.h"

#ifdef __cplusplus
extern "C" {
#endif

/* clang-format off */

#define NEXT_OP()   ({ *pc++; })
#define NEXT_REG()  ({ *pc++; })
#define NEXT_I8()   ({ int8 v = *(int8 *)pc; pc += 1; v; })
#define NEXT_I16()  ({ int16 v = *(int16 *)pc; pc += 2; v; })
#define NEXT_I32()  ({ int32 v = *(int32 *)pc; pc += 4; v; })

#define REG_SET_I32(reg, val) *(cf->base + reg) = (KlValue){val, int32_vtbl}
#define REG_GET_I32(reg) *(int32 *)(cf->base + reg)

#define PUSH(reg) *++ks->top = *(cf->base + reg);
#define POP(reg) *(cf->base + reg) = *ks->top--;

/* clang-format on */

#if 0
void kl_eval_frame(KlState *ks, KlFrame *cf)
{
    uint8 op;
    uint8 ra, rb, rc;
    KlCode *code = (KlCode *)cf->func->ptr;
    uint8 *pc = code->codes + cf->pc;

    /* main loop */
    for (;;) {
        uint8 op = NEXT_OP();
        switch (op) {
            case OP_JMP_I32_CMPGTK: {
                ra = NEXT_REG();
                int32 va = REG_GET_I32(ra);
                uint8 k = (uint8)NEXT_I8();
                int16 offset = NEXT_I16();
                if (va > k) pc += offset;
                break;
            }
            case OP_RET_VALUE: {
                ra = NEXT_REG();
                if (ra != 0) {
                    assert(0);
                }
                int32 va = REG_GET_I32(ra);
                // printf("%d\n", va);
                ks->top = cf->base;
                KlFrame *back = cf->back;
                ks->cf = back;
                --ks->depth;
                return;
            }
            case OP_I32_SUBK: {
                ra = NEXT_REG();
                rb = NEXT_REG();
                int32 vb = REG_GET_I32(rb);
                uint8 k = (uint8)NEXT_I8();
                REG_SET_I32(ra, vb - k);
                break;
            }
            case OP_I32_ADD: {
                ra = NEXT_REG();
                rb = NEXT_REG();
                rc = NEXT_REG();
                int32 vb = REG_GET_I32(rb);
                int32 vc = REG_GET_I32(rc);
                REG_SET_I32(ra, vb + vc);
                break;
            }
            case OP_PUSH: {
                ra = NEXT_REG();
                PUSH(ra);
                break;
            }
            case OP_POP: {
                ra = NEXT_REG();
                POP(ra);
                break;
            }
            case OP_CALL: {
                int16 offset = NEXT_I16();
                uint8 argc = (uint8)NEXT_I8();
                kl_eval_func(ks, cf->func);
                break;
            }
            default:
                assert(0);
                break;
        }
    }
}

#else

void kl_eval_frame(KlState *ks, KlFrame *cf)
{
    uint8 op;
    uint8 ra, rb, rc;
    KlCode *code = (KlCode *)cf->func->ptr;
    uint8 *pc = code->codes + cf->pc;

#include "disptab.h"

    /* main loop */
    for (;;) {
        vm_dispatch(NEXT_OP())
        {
            vm_case(OP_JMP_I32_CMPGTK)
            {
                ra = NEXT_REG();
                int32 va = REG_GET_I32(ra);
                uint8 k = (uint8)NEXT_I8();
                int16 offset = NEXT_I16();
                if (va > k) pc += offset;
                vm_break;
            }
            vm_case(OP_RET_VALUE)
            {
                ra = NEXT_REG();
                // if (ra != 0) {
                //   assert(0);
                //}
                // int32 va = REG_GET_I32(ra);
                // printf("%d\n", va);
                ks->top = cf->base;
                KlFrame *back = cf->back;
                ks->cf = back;
                --ks->depth;
                return;
            }
            vm_case(OP_I32_SUBK)
            {
                ra = NEXT_REG();
                rb = NEXT_REG();
                int32 vb = REG_GET_I32(rb);
                uint8 k = (uint8)NEXT_I8();
                REG_SET_I32(ra, vb - k);
                vm_break;
            }
            vm_case(OP_I32_ADD)
            {
                ra = NEXT_REG();
                rb = NEXT_REG();
                rc = NEXT_REG();
                int32 vb = REG_GET_I32(rb);
                int32 vc = REG_GET_I32(rc);
                REG_SET_I32(ra, vb + vc);
                vm_break;
            }
            vm_case(OP_PUSH)
            {
                ra = NEXT_REG();
                PUSH(ra);
                vm_break;
            }
            vm_case(OP_POP)
            {
                ra = NEXT_REG();
                POP(ra);
                vm_break;
            }
            vm_case(OP_CALL)
            {
                int16 offset = NEXT_I16();
                uint8 argc = (uint8)NEXT_I8();
                kl_eval_func(ks, cf->func);
                vm_break;
            }
            assert(0);
        }
    }
}

#endif

void kl_eval_func(KlState *ks, KlFunc *func)
{
    KlFrame *cf;
    if (ks->cf->prev) {
        cf = ks->cf->prev;
    } else {
        cf = malloc(sizeof(*cf));
    }
    cf->func = func;
    cf->base = ks->cf->top + 1;
    cf->top = cf->base + 2;
    // assert(ks->top >= ks->cf->top);
    ks->top = cf->top;
    cf->back = ks->cf;
    ks->cf->prev = cf;
    ks->cf = cf;
    ++ks->depth;
    kl_eval_frame(ks, cf);
}

#ifdef __cplusplus
}
#endif

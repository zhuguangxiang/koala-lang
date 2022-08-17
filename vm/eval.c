/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2022-2032 James <zhuguangxiang@gmail.com>
 */

#include "eval.h"
#include "opcode.h"

#ifdef __cplusplus
extern "C" {
#endif

KlFunc *psudo_get_method(int index);

// clang-format off

#define NEXT_OP()   ({ *pc++; })
#define NEXT_I8()   ({ int8_t v = *(int8_t *)pc; pc += 1; v; })
#define NEXT_I16()  ({ int16_t v = *(int16_t *)pc; pc += 2; v; })
#define NEXT_I32()  ({ int32_t v = *(int32_t *)pc; pc += 4; v; })

#define PUSH_I32(val) ({ \
    int32_t v = (int32_t)(val); \
    *ks->top = (StkVal)v; \
    ++ks->top; \
})

#define POP_I32() ({ \
    --ks->top; \
    int32_t v = (int32_t)*ks->top; \
    v; \
})

#define PUSH_SIZE(from, size) ({ \
    memmove(ks->top, (from), (size) * sizeof(StkVal)); \
    ks->top += (size); \
})

#define POP_SIZE(to, size) ({ \
    ks->top -= (size); \
    memmove((to), ks->top, (size) * sizeof(StkVal)); \
})

// clang-format on

void kl_evaluate(KlState *ks, KlFrame *cf)
{
    KlFunc *func = cf->func;
    uint8_t op;
    uint8_t *pc = func->codes;

    /* main loop */
    for (;;) {
        op = NEXT_OP();
        switch (op) {
            case OP_HALT: {
                printf("Koala VM is halted.\n");
                exit(-1);
                break;
            }
            case OP_I32_CONST: {
                PUSH_I32(NEXT_I32());
                break;
            }
            case OP_I32_ADD: {
                int32_t v1 = POP_I32();
                int32_t v2 = POP_I32();
                int64_t sum = (int64_t)v1 + (int64_t)v2;
                if (sum > INT32_MAX || sum < INT32_MIN) {
                    printf("error: overflow\n");
                    exit(-1);
                } else {
                    PUSH_I32(sum);
                }
                break;
            }
            case OP_LOCAL_GET: {
                uint8_t index = (uint8_t)NEXT_I8();
                KlLocal *local = func->locals + index;
                StkVal *from = cf->base + local->offset;
                PUSH_SIZE(from, local->size);
                break;
            }
            case OP_LOCAL_SET: {
                uint8_t index = (uint8_t)NEXT_I8();
                KlLocal *local = func->locals + index;
                StkVal *to = cf->base + local->offset;
                POP_SIZE(to, local->size);
                break;
            }
            case OP_CALL: {
                KlFunc *f = psudo_get_method(NEXT_I16());
                kl_do_call(ks, f);
                break;
            }
            case OP_CALL_INDIRECT: {
                break;
            }
            case OP_RETURN: {
                return;
            }
            case OP_RETURN_VALUE: {
                POP_SIZE(cf->base, func->ret_size);
                return;
            }
            default: {
                assert(0);
                break;
            }
        }
    }
}

void kl_do_call(KlState *ks, KlFunc *func)
{
    /* reuse call frame, if available */
    KlFrame *cf = ks->cf->prev;
    if (cf == NULL) {
        cf = malloc(sizeof(KlFrame));
    }

    /* intialize this call frame */
    cf->func = func;
    cf->base = ks->top - func->param_size;
    cf->top = cf->base + func->stack_size;
    cf->prev = NULL;

    /* add to state */
    cf->back = ks->cf;
    ks->cf->prev = cf;

    ks->top = cf->top;
    ks->cf = cf;
    ++ks->depth;

    /* exec this call frame */
    kl_evaluate(ks, cf);

    ks->top = cf->base + func->ret_size;
    ks->cf = cf->back;
    --ks->depth;
    printf("res=%ld\n", ks->top[-1]);
}

#ifdef __cplusplus
}
#endif

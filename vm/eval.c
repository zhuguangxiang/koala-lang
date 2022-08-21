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

#define NEXT_OP()   *pc++
#define NEXT_I8()   ({ int8_t v = *(int8_t *)pc; pc++; v; })
#define NEXT_I16()  ({ int16_t v = *(int16_t *)pc; pc += 2; v; })
#define NEXT_U8()   ({ uint8_t v = *(uint8_t *)pc; pc++; v; })
#define NEXT_U16()  ({ uint16_t v = *(uint16_t *)pc; pc += 2; v; })

#define PUSH_I32(val) do { \
    *ks->top = (uint32_t)(val); \
    ++ks->top; \
} while (0)

#define POP_I32() ({ --ks->top; (int32_t)*ks->top; })

#define PUSH_I8(val) do { \
    *ks->top = (int8_t)(val); \
    ++ks->top; \
} while (0)

#define POP_I8() ({ --ks->top; *(int8_t *)(ks->top); })

#define PUSH_LABEL(addr) do { \
    *((uintptr_t *)(ks->top)) = (uintptr_t)(addr); \
    ks->top += 2; \
} while (0)

#define PUSH_REF(addr) do { \
    *((uintptr_t *)(ks->top)) = *(uintptr_t *)(addr); \
    ks->top += 2; \
} while (0)

#define POP_REF() ({ ks->top -= 2; *(uintptr_t *)(ks->top); })

#define PUSH_SIZED(addr, size) do { \
    memcpy(ks->top, (addr), (size) * sizeof(StkVal)); \
    ks->top += (size); \
} while (0)

#define STACK_GET_I32(i) (int32_t)*(ks->top + (i))

#define STACK_SET_I32(i, v) do { \
    *(ks->top + (i)) = (int32_t)(v); \
} while (0)

#define STACK_TOP_INC(i) do { \
    ks->top += (i); \
} while (0)

#define STACK_TOP_DEC(i) do { \
    ks->top -= (i); \
} while (0)

// clang-format on

void show_locals(KlFrame *cf)
{
    KlLocal *local;
    KlFunc *func = cf->func;
    for (int i = 0; i < func->num_locals; i++) {
        local = func->locals + i;
        if (local->size == 1) {
            printf("[%04d]: %d\n", i, cf->base[local->offset]);
        } else {
            assert(0);
        }
    }
}

#if 1
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
            case OP_I8_PUSH: {
                PUSH_I8(NEXT_I8());
                break;
            }
            /*
            case OP_I16_PUSH: {
                PUSH_I16(NEXT_I16());
                break;
            }
            */
            case OP_I32_CONST_M1: {
                PUSH_I32(-1);
                break;
            }
            case OP_I32_CONST_0: {
                PUSH_I32(0);
                break;
            }
            case OP_I32_CONST_1: {
                PUSH_I32(1);
                break;
            }
            case OP_I32_CONST_2: {
                PUSH_I32(2);
                break;
            }
            case OP_I32_CONST_3: {
                PUSH_I32(3);
                break;
            }
            case OP_I32_CONST_4: {
                PUSH_I32(4);
                break;
            }
            case OP_I32_CONST_5: {
                PUSH_I32(5);
                break;
            }
            case OP_I32_ADD: {
                int32_t v1 = STACK_GET_I32(-1);
                int32_t v2 = STACK_GET_I32(-2);
                STACK_TOP_DEC(1);
                int32_t sum;
                if (__builtin_sadd_overflow(v2, v1, &sum)) {
                    printf("error: overflow\n");
                    exit(-1);
                } else {
                    STACK_SET_I32(-1, sum);
                }
                break;
            }
            case OP_I32_SUB: {
                int32_t v1 = STACK_GET_I32(-1);
                int32_t v2 = STACK_GET_I32(-2);
                STACK_TOP_DEC(1);
                int32_t sub;
                if (__builtin_ssub_overflow(v2, v1, &sub)) {
                    printf("error: overflow\n");
                    exit(-1);
                } else {
                    STACK_SET_I32(-1, sub);
                }
                break;
            }
            case OP_FIELD_I8_GET: {
                uint16_t offset = NEXT_U16();
                int8_t *addr = (int8_t *)POP_REF();
                int8_t v = *(addr + offset);
                PUSH_I8(v);
                break;
            }
            case OP_FIELD_I8_SET: {
                uint16_t offset = NEXT_U16();
                int8_t *addr = (int8_t *)POP_REF();
                int8_t v = POP_I8();
                *(addr + offset) = v;
                break;
            }
            case OP_LOCAL_I32_GET: {
                uint16_t offset = NEXT_U16();
                PUSH_I32(cf->base[offset]);
                break;
            }
            case OP_LOCAL_I32_SET: {
                uint16_t offset = NEXT_U16();
                cf->base[offset] = POP_I32();
                break;
            }
            case OP_LOCAL_LABEL_GET: {
                uint16_t offset = NEXT_U16();
                PUSH_LABEL(cf->base + offset);
                break;
            }
            case OP_LOCAL_SIZED_GET: {
                uint16_t offset = NEXT_U16();
                uint16_t size = NEXT_U16();
                PUSH_SIZED(cf->base + offset, size);
                break;
            }
            case OP_LOCAL_REF_GET: {
                uint16_t offset = NEXT_U16();
                PUSH_REF(cf->base + offset);
                break;
            }
            case OP_JMP_INT32_GT: {
                int16_t offset = NEXT_I16();
                int32_t rhs = STACK_GET_I32(-1);
                int32_t lhs = STACK_GET_I32(-2);
                STACK_TOP_DEC(2);
                if (lhs > rhs) {
                    pc += offset;
                } else {
                    // do nothing
                }
                break;
            }
            case OP_CALL: {
                KlFunc *f = psudo_get_method(NEXT_I16());
                kl_do_call(ks, f);
                break;
            }
            case OP_RETURN: {
                // show_locals(cf);
                return;
            }
            case OP_I32_RETURN: {
                // show_locals(cf);
                --ks->top;
                cf->base[0] = ks->top[0];
                return;
            }
            default: {
                assert(0);
                break;
            }
        }
    }
}

#else

void kl_evaluate(KlState *ks, KlFrame *cf)
{
    KlFunc *func = cf->func;
    uint8_t op;
    uint8_t *pc = func->codes;

#include "disptab.h"

    /* main loop */
    for (;;) {
        // op = NEXT_OP();
        vm_dispatch(NEXT_OP())
        {
            vm_case(OP_HALT)
            {
                printf("Koala VM is halted.\n");
                exit(-1);
                vm_break;
            }
            vm_case(OP_I32_CONST)
            {
                PUSH_I32(NEXT_I16());
                vm_break;
            }
            vm_case(OP_I32_ADD)
            {
                int32_t v1 = POP_I32();
                int32_t v2 = POP_I32();
                int64_t sum = (int64_t)v1 + (int64_t)v2;
                if (sum > INT32_MAX || sum < INT32_MIN) {
                    printf("error: overflow\n");
                    exit(-1);
                } else {
                    PUSH_I32(sum);
                }
                vm_break;
            }
            vm_case(OP_I32_SUB)
            {
                int32_t v1 = POP_I32();
                int32_t v2 = POP_I32();
                int64_t result = (int64_t)v1 - (int64_t)v2;
                PUSH_I32(result);
                // if (result > INT32_MAX || result < INT32_MIN) {
                //     printf("error: overflow\n");
                //     exit(-1);
                // } else {
                //     PUSH_I32(result);
                // }
                vm_break;
            }
            vm_case(OP_LOCAL_GET)
            {
                // uint8_t index = (uint8_t)NEXT_I8();
                //  KlLocal *local = func->locals + index;
                //  StkVal *from = cf->base + local->offset;
                //  PUSH_SIZE(from, local->size);

                *ks->top = cf->base[0];
                ks->top++;

                vm_break;
            }
            vm_case(OP_LOCAL_SET)
            {
                uint8_t index = (uint8_t)NEXT_I8();
                KlLocal *local = func->locals + index;
                StkVal *to = cf->base + local->offset;
                POP_SIZE(to, local->size);
                // ks->top--;
                // *to = *ks->top;

                vm_break;
            }
            vm_case(OP_JMP_INT32_CMPGT)
            {
                int32_t lhs = POP_I32();
                int32_t rhs = POP_I32();
                int16_t offset = NEXT_I16();
                if (lhs > rhs) {
                    pc += offset;
                } else {
                    // do nothing
                }
                vm_break;
            }
            vm_case(OP_CALL)
            {
                KlFunc *f = psudo_get_method(NEXT_I16());
                // kl_do_call(ks, f);
                KlFrame *cf2 = ks->cf->prev;
                if (cf2 == NULL) {
                    cf2 = malloc(sizeof(KlFrame));
                } else {
                    // printf("abc:%d\n", frame_index);
                }

                /* intialize this call frame */
                cf2->func = f;
                cf2->base = ks->top - f->param_size;
                cf2->top = cf2->base + f->stack_size;
                // cf2->prev = NULL;

                /* add to state */
                cf2->back = ks->cf;
                ks->cf->prev = cf2;

                ks->top = cf2->top;
                ks->cf = cf2;
                //++ks->depth;

                /* exec this call frame */
                kl_evaluate(ks, cf2);
                vm_break;
            }
            vm_case(OP_CALL_INDIRECT)
            {
                vm_break;
            }
            vm_case(OP_RETURN)
            {
                ks->top = cf->base + cf->func->ret_size;
                ks->cf = cf->back;
                return;
            }
            vm_case(OP_RETURN_VALUE)
            {
                // POP_SIZE(cf->base, func->ret_size);
                ks->top--;
                *cf->base = *ks->top;
                ks->top = cf->base + cf->func->ret_size;
                ks->cf = cf->back;
                return;
            }
            vm_case(OP_I32_CONST_0)
            {
                PUSH_I32(0);
                vm_break;
            }
            vm_case(OP_I32_CONST_1)
            {
                PUSH_I32(1);
                vm_break;
            }
            vm_case(OP_I32_CONST_2)
            {
                PUSH_I32(2);
                vm_break;
            }
            assert(0);
        }
    }
}
#endif

void kl_do_call(KlState *ks, KlFunc *func)
{
    /* reuse call frame, if available */
    KlFrame *cf = ks->cf->prev;
    if (cf == NULL) {
        cf = calloc(1, sizeof(KlFrame));
    }

    /* intialize this call frame */
    cf->func = func;
    cf->base = ks->top - func->param_size;
    cf->top = cf->base + func->stack_size;
    // cf->prev = NULL;

    /* add to state */
    cf->back = ks->cf;
    ks->cf->prev = cf;

    ks->top = cf->top;
    ks->cf = cf;
    //++ks->depth;

    /* exec this call frame */
    kl_evaluate(ks, cf);

    ks->top = cf->base + func->ret_size;
    ks->cf = cf->back;
    //--ks->depth;
    // printf("res=%ld\n", ks->top[-1]);
}

void kl_fini_state(KlState *ks)
{
    KlFrame *cf = &ks->base_cf;
    KlFrame *prev = cf->prev;
    while (prev != NULL) {
        KlFrame *cur = prev;
        prev = prev->prev;
        free(cur);
    }
}

#ifdef __cplusplus
}
#endif

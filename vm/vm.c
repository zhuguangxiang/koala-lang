/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "vm.h"
#include "opcode.h"
#include "util/mm.h"

#ifdef __cplusplus
extern "C" {
#endif

// clang-format off

#define NEXT_OP()   ({ *pc++; })
#define NEXT_REG()  ({ *pc++; })
#define NEXT_I8()   ({ int8 v = *(int8 *)pc; pc += 1; v; })
#define NEXT_I16()  ({ int16 v = *(int16 *)pc; pc += 2; v; })
#define NEXT_I32()  ({ int32 v = *(int32 *)pc; pc += 4; v; })

#define SET_STK_I32(reg, val) *(int32 *)(ci->base + reg) = (val)
#define GET_STK_I32(reg) *(int32 *)(ci->base + reg)

#define MOVE(ra, rb) ci->base[ra] = ci->base[rb]
#define PUSH(ra) *++ks->top = ci->base[ra]
#define SAVE_RET(ra) ci->base[ra] = *(ci->top + 1)

#define GET_RET_I32() *(int32 *)(ci->top + 1)

#define STK_NIL(ra) ci->base[ra] = (StkVal)nil

// clang-format on

#if 1
void koala_execute(KoalaState *ks, CallInfo *ci)
{
    uint8 op;
    uint8 ra, rb, rc;
    uint8 *pc = ci->savedpc;

    /* main loop */
    for (;;) {
        op = NEXT_OP();
        switch (op) {
            case OP_MOVE: {
                ra = NEXT_REG();
                rb = NEXT_REG();
                MOVE(ra, rb);
                break;
            }
            case OP_NIL: {
                ra = NEXT_REG();
                STK_NIL(ra);
                break;
            }
            case OP_I8K: {
                ra = NEXT_REG();
                int8 v = NEXT_I8();
                SET_STK_I32(ra, v);
                break;
            }
            case OP_I32_ADD: {
                ra = NEXT_REG();
                rb = NEXT_REG();
                rc = NEXT_REG();
                int32 v1 = GET_STK_I32(rb);
                int32 v2 = GET_STK_I32(rc);
                SET_STK_I32(ra, v1 + v2);
                break;
            }
            case OP_I32_ADDK: {
                ra = NEXT_REG();
                rb = NEXT_REG();
                int32 v1 = GET_STK_I32(rb);
                uint8 v2 = (uint8)NEXT_I8();
                SET_STK_I32(ra, v1 + v2);
                break;
            }
            case OP_I32_SUBK: {
                ra = NEXT_REG();
                rb = NEXT_REG();
                int32 v1 = GET_STK_I32(rb);
                uint8 v2 = (uint8)NEXT_I8();
                SET_STK_I32(ra, v1 - v2);
                break;
            }
            case OP_I32_CMPK: {
                ra = NEXT_REG();
                rb = NEXT_REG();
                int32 v1 = GET_STK_I32(rb);
                uint8 v2 = (uint8)NEXT_I8();
                int32 res = v1 > v2 ? 1 : (v1 < v2 ? -1 : 0);
                SET_STK_I32(ra, res);
                break;
            }
            case OP_I32_JMP_CMPKGT: {
                ra = NEXT_REG();
                int32 v1 = GET_STK_I32(ra);
                uint8 v2 = (uint8)NEXT_I8();
                int16 offset = NEXT_I16();
                int32 res = v1 > v2 ? 1 : (v1 < v2 ? -1 : 0);
                if (res > 0) pc += offset;
                break;
            }
            case OP_JGT: {
                ra = NEXT_REG();
                int16 offset = NEXT_I16();
                int32 v1 = GET_STK_I32(ra);
                if (v1 > 0) pc += offset;
                break;
            }
            case OP_RET: {
                CallInfo *_ci = ci->prev;
                ks->ci = _ci;
                ks->top = ci->base - 1;
                --ks->nci;
                return;
            }
            case OP_PUSH: {
                ra = NEXT_REG();
                PUSH(ra);
                break;
            }
            case OP_SAVE_RET: {
                ra = NEXT_REG();
                SAVE_RET(ra);
                break;
            }
            case OP_I32_ADD_RET: {
                ra = NEXT_REG();
                rb = NEXT_REG();
                int32 v1 = GET_STK_I32(rb);
                int32 v2 = GET_RET_I32();
                SET_STK_I32(ra, v1 + v2);
                break;
            }
            case OP_PUSH_I32_SUBK: {
                ra = NEXT_REG();
                int32 v1 = GET_STK_I32(ra);
                uint8 v2 = (uint8)NEXT_I8();
                v1 = v1 - v2;
                *++ks->top = v1;
                break;
            }
            case OP_CALL: {
                CallInfo *_ci = ci->next;
                if (!_ci) {
                    // printf("new callinfo\n");
                    _ci = mm_alloc_obj(_ci);
                } else {
                    // printf("use exist callinfo\n");
                }
                _ci->base = ci->top + 1;
                _ci->top = _ci->base + 2;
                ks->top = _ci->top;
                _ci->prev = ci;
                ci->next = _ci;
                ks->ci = _ci;
                ++ks->nci;
                int8 num = NEXT_I8();
                int16 offset = NEXT_I16();
                _ci->relinfo = ci->relinfo;
                _ci->code = (uint8 *)ci->relinfo;
                _ci->savedpc = _ci->code;
                ci->savedpc = pc;
                koala_execute(ks, _ci);
                break;
            }
            default: {
                assert(0);
                break;
            }
        }
    }
}
#endif

#if 0

void koala_execute(KoalaState *ks, CallInfo *ci)
{
    uint8 ra, rb, rc;
    uint8 *pc = ci->savedpc;

    static void *dispatch_table[] = {
        &&L$OP_I32_JMP_CMPKGT, &&L$OP_RET,           &&L$OP_SAVE_RET,
        &&L$OP_I32_ADD_RET,    &&L$OP_PUSH_I32_SUBK, &&L$OP_CALL,
    };

#define DISPATCH() goto *dispatch_table[*pc++]

    /* main loop */
    DISPATCH();
    for (;;) {
    L$OP_I32_JMP_CMPKGT : {
        ra = NEXT_REG();
        int32 v1 = GET_STK_I32(ra);
        uint8 v2 = (uint8)NEXT_I8();
        int16 offset = NEXT_I16();
        int32 res = v1 > v2 ? 1 : (v1 < v2 ? -1 : 0);
        if (res > 0) pc += offset;
        DISPATCH();
    }
    L$OP_RET : {
        CallInfo *_ci = ci->prev;
        ks->ci = _ci;
        ks->top = ci->base - 1;
        --ks->nci;
        return;
    }
    L$OP_SAVE_RET : {
        ra = NEXT_REG();
        SAVE_RET(ra);
        DISPATCH();
    }
    L$OP_I32_ADD_RET : {
        ra = NEXT_REG();
        rb = NEXT_REG();
        int32 v1 = GET_STK_I32(rb);
        int32 v2 = GET_RET_I32();
        SET_STK_I32(ra, v1 + v2);
        DISPATCH();
    }
    L$OP_PUSH_I32_SUBK : {
        ra = NEXT_REG();
        int32 v1 = GET_STK_I32(ra);
        uint8 v2 = (uint8)NEXT_I8();
        v1 = v1 - v2;
        *++ks->top = v1;
        DISPATCH();
    }
    L$OP_CALL : {
        CallInfo *_ci = ci->next;
        if (!_ci) {
            // printf("new callinfo\n");
            _ci = mm_alloc_obj(_ci);
        } else {
            // printf("use exist callinfo\n");
        }
        _ci->base = ci->top + 1;
        _ci->top = _ci->base + 2;
        ks->top = _ci->top;
        _ci->prev = ci;
        ci->next = _ci;
        ks->ci = _ci;
        ++ks->nci;
        int8 num = NEXT_I8();
        int16 offset = NEXT_I16();
        _ci->relinfo = ci->relinfo;
        _ci->code = (uint8 *)ci->relinfo;
        _ci->savedpc = _ci->code;
        ci->savedpc = pc;
        koala_execute(ks, _ci);
        DISPATCH();
    }
        assert(0);
    }
}

#endif

#ifdef __cplusplus
}
#endif

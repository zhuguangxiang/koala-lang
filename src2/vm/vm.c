/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangxiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "vm.h"
#include "cf_ffi.h"
#include "codeobject.h"
#include "opcode.h"

#ifdef __cplusplus
extern "C" {
#endif

static void __new_callinfo(KoalaState *ks)
{
    if (ks->nci >= MAX_CALL_DEPTH) {
        printf("error: stack overflow(too many call frame)\n");
        abort();
    }

    CallInfo *back = ks->ci;
    TValueRef *func = back->top + 1;
    Object *meth = func->_v.obj;
    int nloc = method_get_nloc(meth);

    CallInfo *ci = MemAlloc(sizeof(CallInfo));
    ci->back = back;
    ci->func = func;
    ci->top = func + nloc;

    ks->top = ci->top;
    ks->ci = ci;
    ks->nci++;
}

static void __free_callinfo(KoalaState *ks)
{
    CallInfo *ci = ks->ci;
    CallInfo *back = ci->back;

    ks->ci = back;
    ks->top = back->top;
    ks->nci--;
    MemFree(ci);
}

#define NEXT_OP() (assert(pc < codesize), codes[pc++])

#define TOP()   *(top - 1)
#define POP()   *(top--)
#define PUSH(v) (*++top = (v))

#define NEXT_BYTE() (assert(pc < codesize), codes[pc++])

#define CHECK()                                \
    if (top < base || top >= ks->stack_last) { \
        printf("error: stack overlfow\n");     \
    }

#define SET_LOCAL(i, v) *(ci->func + 1 + i) = (v)
#define GET_LOCAL(i)    *(ci->func + 1 + i)

void eval(KoalaState *ks)
{
    CallInfo *ci = ks->ci;

    /* calculate stack */
    TValueRef *base = ci->top;
    TValueRef *top = ks->top;

    /* opcode array */
    MethodObject *meth = (MethodObject *)ci->func->_v.obj;
    CodeObject *cobj = meth->ptr;
    uint8_t *codes = cobj->codes;
    int codesize = cobj->size;

    /* code pc */
    int pc = ci->saved_pc;

    /* current opcode */
    uint8_t op;

    /* temperary variables */
    TValueRef x, y;
    int iarg;

    while (1) {
        op = NEXT_OP();
        CHECK();
        switch (op) {
            case OP_POP_TOP: {
                x = POP();
                break;
            }
            case OP_DUP: {
                x = TOP();
                PUSH(x);
                break;
            }
            case OP_SWAP: {
                x = POP();
                y = POP();
                PUSH(x);
                PUSH(y);
                break;
            }
            case OP_CONST_BYTE: {
                int8_t bval = NEXT_BYTE();
                setbyteval(&x, bval);
                PUSH(x);
                break;
            }
            case OP_LOAD: {
                abort();
                break;
            }
            case OP_LOAD_0: {
                x = GET_LOCAL(0);
                PUSH(x);
                break;
            }
            case OP_LOAD_1: {
                x = GET_LOCAL(1);
                PUSH(x);
                break;
            }
            case OP_STORE: {
                iarg = NEXT_BYTE();
                x = POP();
                SET_LOCAL(iarg, x);
                break;
            }
            case OP_ADD: {
                x = POP();
                y = POP();
                if (x._t.tag == 1 && y._t.tag == 1) {
                    if (x._t.kind == TYPE_BYTE && y._t.kind == TYPE_BYTE) {
                        x._v.bval += y._v.bval;
                        PUSH(x);
                    }
                } else {
                }
                break;
            }
            case OP_SUB: {
                break;
            }
            case OP_CALL: {
                break;
            }
            case OP_RETURN_VALUE: {
                x = POP();
                __free_callinfo(ks);
                *++ks->top = x;
                return;
            }
            default: {
                printf("error: unrecognized opcode\n");
                abort();
            }
        }
    }
}

DLLEXPORT KoalaState *kl_new_state(void)
{
    KoalaState *ks;

    /* create new koalastate */
    ks = MemAlloc(sizeof(*ks));

    /* initialize stack  */
    ks->stack = MemAlloc(MAX_STACK_SIZE * sizeof(TValueRef));
    ks->stacksize = MAX_STACK_SIZE;
    ks->stack_last = ks->stack + ks->stacksize;
    ks->top = ks->stack;

    /* initialize ci */
    CallInfo *ci = &ks->base_ci;
    ci->back = null;
    ci->func = ks->top;
    /* `func` entry for this `ci` */
    setnilval(ks->top);
    ci->top = ks->top;

    ks->ci = ci;
    ks->nci = 1;

    return ks;
}

static void to_value(TValueRef *x, TypeDesc *desc, intptr_t val)
{
    switch (desc->kind) {
        case TYPE_BYTE:
            setbyteval(x, (int8_t)val);
            break;
        case TYPE_INT:
            setintval(x, (int64_t)val);
            break;
        default:
            abort();
            break;
    }
}

void kl_do_call(KoalaState *ks)
{
    CallInfo *ci = ks->ci;
    TValueRef *func = ci->top + 1;
    MethodObject *meth = (MethodObject *)func->_v.obj;
    if (meth->kind == CFUNC_KIND) {
        ProtoDesc *proto = (ProtoDesc *)meth->desc;
        cfunc_t *cf = meth->ptr;
        TValueRef *args = func + 1;
        int narg = ks->top - ci->top;
        intptr_t ret;
        kl_call_cfunc(cf, args, narg, &ret);
        ks->top -= narg;
        if (proto->rtype) {
            TValueRef val;
            to_value(&val, proto->rtype, ret);
            *++ks->top = val;
        }
    } else {
        assert(meth->kind == KFUNC_KIND);
        __new_callinfo(ks);
        eval(ks);
    }
}

#ifdef __cplusplus
}
#endif

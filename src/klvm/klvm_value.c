/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2020-2021 James <zhuguangxiang@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "klvm/klvm_value.h"

#ifdef __cplusplus
extern "C" {
#endif
/*--------------------------------------------------------------------------*\
|* Literal constant                                                         *|
\*--------------------------------------------------------------------------*/

KLVMValue *KLVMConstInt32(int32_t v)
{
    KLVMConstRef k = MemAllocWithPtr(k);
    INIT_VALUE_HEAD(k, KLVM_VALUE_CONST, KLVMTypeInt32(), "");
    k->i32val = v;
    return (KLVMValueRef)k;
}

/*--------------------------------------------------------------------------*\
|* Basic Block                                                              *|
\*--------------------------------------------------------------------------*/

static KLVMBlockRef __new_block(KLVMValueRef fn, int dummy, char *label)
{
    KLVMBlockRef bb = MemAllocWithPtr(bb);
    InitList(&bb->bb_link);
    InitList(&bb->insns);
    InitList(&bb->in_edges);
    InitList(&bb->out_edges);
    bb->fn = fn;
    bb->label = label;
    bb->dummy = dummy;
    bb->tag = -1;
    return bb;
}

KLVMBlockRef KLVMAppendBlock(KLVMValueRef _fn, char *label)
{
    KLVMBlockRef bb = __new_block(_fn, 0, label);
    KLVMFuncRef fn = (KLVMFuncRef)_fn;

    if (ListEmpty(&fn->bb_list)) {
        /* first block, add an edge <start, bb> */
        KLVMLinkEdge(fn->sbb, bb);
    }

    ListPushBack(&fn->bb_list, &bb->bb_link);
    fn->num_bbs++;
    return bb;
}

KLVMBlockRef KLVMAddBlock(KLVMBlockRef _bb, char *label)
{
    KLVMFuncRef fn = (KLVMFuncRef)_bb->fn;
    KLVMBlockRef bb = __new_block((KLVMValueRef)fn, 0, label);
    ListAdd(&_bb->bb_link, &bb->bb_link);
    fn->num_bbs++;
    return bb;
}

KLVMBlockRef KLVMAddBlockBefore(KLVMBlockRef _bb, char *label)
{
    KLVMFuncRef fn = (KLVMFuncRef)_bb->fn;
    KLVMBlockRef bb = __new_block((KLVMValueRef)fn, 0, label);
    ListAddBefore(&_bb->bb_link, &bb->bb_link);
    fn->num_bbs++;
    return bb;
}

void KLVMDeleteBlock(KLVMBlockRef bb)
{
    /* FIXME */
    ListRemove(&bb->bb_link);
    MemFree(bb);
}

/*--------------------------------------------------------------------------*\
|* Block Edge                                                               *|
\*--------------------------------------------------------------------------*/

void KLVMLinkEdge(KLVMBlockRef src, KLVMBlockRef dst)
{
    KLVMEdgeRef edge = MemAllocWithPtr(edge);
    edge->src = src;
    edge->dst = dst;
    InitList(&edge->link);
    InitList(&edge->in_link);
    InitList(&edge->out_link);
    KLVMFuncRef fn = (KLVMFuncRef)src->fn;
    ListPushBack(&fn->edge_list, &edge->link);
    ListPushBack(&src->out_edges, &edge->out_link);
    ListPushBack(&dst->in_edges, &edge->in_link);
}

/*--------------------------------------------------------------------------*\
|* Instruction Builder                                                      *|
\*--------------------------------------------------------------------------*/

void KLVMSetBuilderAtEnd(KLVMBuilderRef bldr, KLVMBlockRef bb)
{
    bldr->bb = bb;
    bldr->it = &bb->insns;
}

void KLVMSetBuilderAtHead(KLVMBuilderRef bldr, KLVMBlockRef bb)
{
    bldr->bb = bb;
    bldr->it = bb->insns.prev;
}

/*--------------------------------------------------------------------------*\
|* Function                                                                 *|
\*--------------------------------------------------------------------------*/

KLVMTypeRef KLVMGetFunctionType(KLVMValueRef fn)
{
    return NULL;
}

KLVMValueRef KLVMGetParam(KLVMValueRef _fn, int index)
{
    KLVMFuncRef fn = (KLVMFuncRef)_fn;
    int num_params = fn->num_params;
    if (index < 0 || index >= num_params) {
        printf("error: index %d out of range(0 ..< %d)\n", index, num_params);
        abort();
    }

    KLVMValueRef param = NULL;
    VectorGet(&fn->locals, index, &param);
    return param;
}

static KLVMVarRef __new_var(KLVMTypeRef ty, char *name)
{
    if (!name) {
        printf("error: variable must have name\n");
        abort();
    }

    KLVMVarRef var = MemAllocWithPtr(var);
    INIT_VALUE_HEAD(var, KLVM_VALUE_VAR, ty, name);
    return var;
}

KLVMValueRef KLVMAddLocal(KLVMValueRef fn, char *name, KLVMTypeRef ty)
{
    KLVMVarRef var = __new_var(ty, name);
    var->local = 1;
    VectorPushBack(&((KLVMFuncRef)fn)->locals, &var);
    return (KLVMValueRef)var;
}

/*--------------------------------------------------------------------------*\
|* Module: values container                                                 *|
\*--------------------------------------------------------------------------*/

KLVMModuleRef KLVMCreateModule(char *name)
{
    KLVMModuleRef m = MemAllocWithPtr(m);
    m->name = name;
    VectorInitPtr(&m->items);
    return m;
}

void KLVMDestroyModule(KLVMModuleRef m)
{
    /* FIXME */
    MemFree(m);
}

static KLVMFuncRef __new_func(KLVMTypeRef ty, char *name)
{
    if (!name) {
        printf("error: function must have name\n");
        abort();
    }

    KLVMFuncRef fn = MemAllocWithPtr(fn);
    INIT_VALUE_HEAD(fn, KLVM_VALUE_FUNC, ty, name);

    InitList(&fn->bb_list);
    InitList(&fn->edge_list);
    VectorInitPtr(&fn->locals);

    /* initial 'start' and 'end' dummy block */
    fn->sbb = __new_block((KLVMValueRef)fn, 1, "start");
    fn->ebb = __new_block((KLVMValueRef)fn, 1, "end");

    // add parameters
    VectorRef pty = KLVMProtoParams(ty);
    fn->num_params = VectorSize(pty);
    KLVMTypeRef *item;
    VectorForEach(item, pty, { KLVMAddLocal((KLVMValueRef)fn, "", *item); });

    return fn;
}

KLVMValueRef KLVMGetInitFunction(KLVMModuleRef m)
{
    KLVMValueRef fn = m->fn;
    if (!fn) {
        KLVMTypeRef vvty = KLVMTypeProto(NULL, NULL, 0);
        fn = (KLVMValueRef)__new_func(vvty, "__init__");
        VectorPushBack(&m->items, &fn);
        m->fn = fn;
    }
    return fn;
}

KLVMValueRef KLVMAddVariable(KLVMModuleRef m, char *name, KLVMTypeRef ty)
{
    KLVMVarRef var = __new_var(ty, name);
    VectorPushBack(&m->items, &var);
    return (KLVMValueRef)var;
}

KLVMValueRef KLVMAddFunction(KLVMModuleRef m, char *name, KLVMTypeRef ty)
{
    KLVMFuncRef fn = __new_func(ty, name);
    VectorPushBack(&m->items, &fn);
    return (KLVMValueRef)fn;
}

/*--------------------------------------------------------------------------*\
|* Pass                                                                     *|
\*--------------------------------------------------------------------------*/

typedef struct _KLVMPassInfo {
    List link;
    KLVMPassFunc callback;
    void *arg;
} KLVMPassInfo, *KLVMPassInfoRef;

void KLVMInitPassGroup(KLVMPassGroupRef grp)
{
    InitList(&grp->passes);
}

void KLVMFiniPassGroup(KLVMPassGroupRef grp)
{
    KLVMPassInfoRef pi, nxt;
    ListForEachSafe(pi, nxt, link, &grp->passes, {
        ListRemove(&pi->link);
        MemFree(pi);
    });
}

void KLVMRegisterPass(KLVMPassGroupRef grp, KLVMPassRef pass)
{
    KLVMPassInfoRef pi = MemAllocWithPtr(pi);
    InitList(&pi->link);
    pi->callback = pass->callback;
    pi->arg = pass->arg;
    ListPushBack(&grp->passes, &pi->link);
}

static void _run_pass(KLVMPassGroupRef grp, KLVMFuncRef fn)
{
    KLVMPassInfoRef pi;
    ListForEach(pi, link, &grp->passes, { pi->callback(fn, pi->arg); });
}

void KLVMRunPassGroup(KLVMPassGroupRef grp, KLVMModuleRef m)
{
    KLVMFuncRef fn;
    KLVMValueRef *item;
    VectorForEach(item, &m->items, {
        if ((*item)->kind != KLVM_VALUE_FUNC) continue;
        fn = *(KLVMFuncRef *)item;
        _run_pass(grp, fn);
    });
}

#ifdef __cplusplus
}
#endif

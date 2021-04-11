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
|* Module                                                                   *|
\*--------------------------------------------------------------------------*/

KLVMModuleRef KLVMCreateModule(char *name)
{
    KLVMModuleRef m = MemAllocWithPtr(m);
    m->name = name;
    VectorInitPtr(&m->items);
    return m;
}

static KLVMFuncRef _NewFunc(char *name, KLVMTypeRef ty)
{
    if (!name) {
        printf("error: function must have name\n");
        abort();
    }

    KLVMFuncRef fn = MemAlloc(sizeof(*fn));
    INIT_KLVM_VALUE_HEAD(fn, KLVM_VALUE_FUNC, ty, name);
    VectorInit(&fn->locals, sizeof(void *));
    InitList(&fn->bb_list);

    // add parameters
    VectorRef pty = KLVMProtoParams(ty);
    fn->num_params = VectorSize(pty);
    KLVMTypeRef *item;
    VectorForEach(item, pty, { KLVMAddLocal((KLVMValueRef)fn, "", *item); });

    return fn;
}

KLVMValueRef KLVMGetInitFunc(KLVMModuleRef m)
{
    KLVMValueRef fn = m->fn;
    if (!fn) {
        fn = (KLVMValueRef)_NewFunc("__init__", NULL);
        m->fn = fn;
    }
    return fn;
}

void KLVMDestroyModule(KLVMModuleRef m)
{
    MemFree(m);
}

/*--------------------------------------------------------------------------*\
|* Literal constant                                                         *|
\*--------------------------------------------------------------------------*/

KLVMValue *KLVMConstInt32(int32_t v)
{
    KLVMConstRef k = MemAlloc(sizeof(*k));
    INIT_KLVM_VALUE_HEAD(k, KLVM_VALUE_CONST, KLVMTypeInt32(), "");
    k->i32val = v;
    return (KLVMValueRef)k;
}

/*--------------------------------------------------------------------------*\
|* Variable                                                                 *|
\*--------------------------------------------------------------------------*/

static inline KLVMVarRef _NewVar(KLVMTypeRef ty, char *name)
{
    if (!name) {
        printf("error: variable must have name\n");
        abort();
    }

    KLVMVarRef var = MemAlloc(sizeof(*var));
    INIT_KLVM_VALUE_HEAD(var, KLVM_VALUE_VAR, ty, name);
    return var;
}

KLVMValueRef KLVMAddVar(KLVMModuleRef m, char *name, KLVMTypeRef ty)
{
    KLVMVarRef var = _NewVar(ty, name);
    VectorPushBack(&m->items, &var);
    return (KLVMValueRef)var;
}

KLVMValueRef KLVMAddFunc(KLVMModuleRef m, char *name, KLVMTypeRef ty)
{
    KLVMFuncRef fn = _NewFunc(name, ty);
    // add to module
    VectorPushBack(&m->items, &fn);
    return (KLVMValueRef)fn;
}

/*--------------------------------------------------------------------------*\
|* Basic Block                                                              *|
\*--------------------------------------------------------------------------*/

static KLVMBlockRef _AppendBlock(KLVMValueRef fn, char *label)
{
    KLVMBlockRef bb = MemAlloc(sizeof(*bb));
    InitList(&bb->bb_link);
    bb->fn = fn;
    InitList(&bb->insts);
    KLVMFunc *_fn = (KLVMFunc *)fn;
    ListPushBack(&_fn->bb_list, &bb->bb_link);
    _fn->num_bbs++;
    bb->label = label;
    bb->tag = -1;
    return bb;
}

KLVMBlockRef KLVMAppendBlock(KLVMValueRef fn, char *label)
{
    return _AppendBlock(fn, label);
}

KLVMBlockRef KLVMAddBlock(KLVMBlockRef bb, char *label)
{
    return NULL;
}

KLVMBlockRef KLVMAddBlockBefore(KLVMBlockRef bb, char *label)
{
    return NULL;
}

/*--------------------------------------------------------------------------*\
|* Instruction Visitor                                                      *|
\*--------------------------------------------------------------------------*/

/* Set visitor at end */
void KLVMSetVisitorAtEnd(KLVMVisitorRef vst)
{
}

/* Set visitor at head */
void KLVMSetVisitorAtHead(KLVMVisitorRef vst)
{
}

/* Set visitor at 'inst' */
void KLVMSetVisitor(KLVMVisitorRef vst, KLVMValueRef inst)
{
}

/* Set visitor before 'inst' */
void KLVMSetVisitorBefore(KLVMVisitorRef vst, KLVMValueRef inst)
{
}

/*--------------------------------------------------------------------------*\
|* Function                                                                 *|
\*--------------------------------------------------------------------------*/

/* Get function param by index */
KLVMValue *KLVMGetParam(KLVMValue *fn, int index)
{
    KLVMFunc *_fn = (KLVMFunc *)fn;
    int num_params = _fn->num_params;
    if (index < 0 || index >= num_params) {
        printf("error: index %d out of range(0 ..< %d)\n", index, num_params);
        abort();
    }

    KLVMValue *para = NULL;
    VectorGet(&_fn->locals, index, &para);
    return para;
}

KLVMValueRef KLVMAddLocal(KLVMValueRef fn, char *name, KLVMTypeRef ty)
{
    KLVMVarRef var = _NewVar(ty, name);
    var->local = 1;
    VectorPushBack(&((KLVMFunc *)fn)->locals, &var);
    return (KLVMValueRef)var;
}

#ifdef __cplusplus
}
#endif

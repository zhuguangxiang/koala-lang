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

#include "klvm/klvm.h"

static void test_fib(KLVMModuleRef m)
{
    KLVMTypeRef rtype = KLVMTypeInt32();
    KLVMTypeRef params[] = { KLVMTypeInt32() };
    KLVMTypeRef proto = KLVMTypeProto(rtype, params, 1);
    KLVMValueRef fn = KLVMAddFunction(m, "fib", proto);
    KLVMValueRef n = KLVMGetParam(fn, 0);
    KLVMSetName(n, "n");

    KLVMBlockRef entry = KLVMAppendBlock(fn, "entry");
    KLVMBlockRef _then = KLVMAppendBlock(fn, "then");
    KLVMBlockRef _else = KLVMAppendBlock(fn, "else");

    KLVMBuilder bldr;
    KLVMSetBuilderAtEnd(&bldr, entry);

    KLVMValueRef cond = KLVMBuildCmple(&bldr, n, KLVMConstInt32(1), "");
    KLVMBuildCondJmp(&bldr, cond, _then, _else);

    KLVMSetBuilderAtEnd(&bldr, _then);
    KLVMBuildRet(&bldr, n);

    KLVMSetBuilderAtEnd(&bldr, _else);
    KLVMValueRef args1[] = {
        KLVMBuildSub(&bldr, n, KLVMConstInt32(1), ""),
    };
    KLVMValueRef r1 = KLVMBuildCall(&bldr, fn, args1, 1, "");
    KLVMValueRef args2[] = {
        KLVMBuildSub(&bldr, n, KLVMConstInt32(2), "x"),
    };
    KLVMValueRef r2 = KLVMBuildCall(&bldr, fn, args2, 1, "");
    KLVMBuildRet(&bldr, KLVMBuildAdd(&bldr, r1, r2, ""));
}

/*
static void test_func(KLVMModule *m)
{
    KLVMType *rtype = KLVMInt32Type();
    KLVMType *params[] = {
        KLVMInt32Type(),
        KLVMInt32Type(),
        NULL,
    };
    KLVMType *proto = KLVMProtoType(rtype, params);
    KLVMValue *fn = KLVMAddFunc(m, proto, "add");
    KLVMValue *v1 = KLVMGetParam(fn, 0);
    KLVMValue *v2 = KLVMGetParam(fn, 1);
    KLVMSetVarName(v1, "v1");
    KLVMSetVarName(v2, "v2");

    KLVMBasicBlock *entry = KLVMFuncEntryBasicBlock(fn);
    KLVMBuilder *bldr = KLVMBasicBlockBuilder(entry);
    KLVMValue *t1 = KLVMBuildAdd(bldr, v1, v2, "");
    KLVMValue *ret = KLVMAddLocVar(fn, KLVMInt32Type(), "res");
    KLVMBuildCopy(bldr, t1, ret);
    KLVMBuildRet(bldr, ret);
    KLVMBuildRet(bldr, KLVMConstInt32(-100));

    KLVMBasicBlock *bb2 = KLVMAppendBasicBlock(fn, "test_bb");
    bldr = KLVMBasicBlockBuilder(bb2);
    KLVMValue *t2 = KLVMBuildSub(bldr, v1, KLVMConstInt32(20), "");
    KLVMBuildRet(bldr, t2);

    KLVMBuildJmp(bldr, bb2);
}

static void test_var(KLVMModule *m)
{
    KLVMBasicBlock *entry = KLVMFuncEntryBasicBlock(KLVMModuleFunc(m));
    KLVMBuilder *bldr = KLVMBasicBlockBuilder(entry);

    KLVMValue *foo = KLVMAddVar(m, KLVMInt32Type(), "foo");
    KLVMBuildCopy(bldr, KLVMConstInt32(100), foo);

    KLVMValue *bar = KLVMAddVar(m, KLVMInt32Type(), "bar");
    KLVMValue *k200 = KLVMConstInt32(200);
    KLVMValue *v = KLVMBuildAdd(bldr, foo, k200, "");
    KLVMBuildCopy(bldr, v, bar);

    KLVMValue *baz = KLVMAddVar(m, KLVMInt32Type(), "baz");
    KLVMBuildCopy(bldr, KLVMBuildSub(bldr, foo, bar, ""), baz);
}
*/

int main(int argc, char *argv[])
{
    KLVMModuleRef m = KLVMCreateModule("test");
    test_fib(m);
    KLVMDumpModule(m);
    KLVMDestroyModule(m);
    return 0;
}

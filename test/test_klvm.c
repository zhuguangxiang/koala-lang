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

static void test_func(KLVMModuleRef m)
{
    KLVMTypeRef rtype = KLVMTypeInt32();
    KLVMTypeRef params[] = {
        KLVMTypeInt32(),
        KLVMTypeInt32(),
    };
    KLVMTypeRef proto = KLVMTypeProto(rtype, params, 2);
    KLVMValueRef fn = KLVMAddFunction(m, "add", proto);
    KLVMValueRef v1 = KLVMGetParam(fn, 0);
    KLVMValueRef v2 = KLVMGetParam(fn, 1);
    KLVMSetName(v1, "v1");
    KLVMSetName(v2, "v2");

    KLVMBuilder bldr;

    KLVMBlockRef entry = KLVMAppendBlock(fn, "entry");
    KLVMSetBuilderAtEnd(&bldr, entry);
    KLVMValueRef t1 = KLVMBuildAdd(&bldr, v1, v2, "");
    KLVMValueRef ret = KLVMAddLocal(fn, "res", KLVMTypeInt32());
    KLVMBuildCopy(&bldr, ret, t1);
    KLVMBuildRet(&bldr, ret);
    // KLVMBuildRet(&bldr, KLVMConstInt32(-100));

    KLVMBlockRef bb2 = KLVMAppendBlock(fn, "test_bb");
    KLVMSetBuilderAtEnd(&bldr, bb2);
    KLVMValueRef t2 = KLVMBuildSub(&bldr, v1, KLVMConstInt32(20), "");
    KLVMBuildRet(&bldr, t2);

    KLVMBuildJmp(&bldr, bb2);
}

static void test_var(KLVMModuleRef m)
{
    KLVMValueRef fn = KLVMGetInitFunction(m);
    KLVMBlockRef entry = KLVMAppendBlock(fn, "entry");

    KLVMBuilder bldr;

    KLVMSetBuilderAtEnd(&bldr, entry);
    KLVMValueRef foo = KLVMAddVariable(m, "foo", KLVMTypeInt32());
    KLVMBuildCopy(&bldr, foo, KLVMConstInt32(100));

    KLVMValueRef bar = KLVMAddVariable(m, "bar", KLVMTypeInt32());
    KLVMValueRef k200 = KLVMConstInt32(200);
    KLVMValueRef v = KLVMBuildAdd(&bldr, foo, k200, "");
    KLVMBuildCopy(&bldr, bar, v);

    KLVMValueRef baz = KLVMAddVariable(m, "baz", KLVMTypeInt32());
    KLVMBuildCopy(&bldr, baz, KLVMBuildSub(&bldr, foo, bar, ""));
}

int main(int argc, char *argv[])
{
    KLVMModuleRef m = KLVMCreateModule("test");
    test_fib(m);
    test_func(m);
    test_var(m);
    KLVMDumpModule(m);

    KLVMPassGroup grp;
    KLVMInitPassGroup(&grp);
    KLVMAddUnReachBlockPass(&grp);
    KLVMAddDotPass(&grp);
    KLVMRunPassGroup(&grp, m);

    KLVMDestroyModule(m);
    return 0;
}

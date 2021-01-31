/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangXiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "klvm.h"

/*
gcc -g test/test_klvm.c src/ir/klvm_module.c src/ir/klvm_type.c src/mm.c
src/vector.c -I./include
*/

static void test_fib(KLVMModule *m)
{
    KLVMType *rtype = KLVMInt32Type();
    KLVMType *params[] = {
        KLVMInt32Type(),
        NULL,
    };
    KLVMType *proto = KLVMProtoType(rtype, params);
    KLVMValue *fn = KLVMAddFunc(m, proto, "fib");
    KLVMValue *n = KLVMGetParam(fn, 0);
    KLVMSetValueName(n, "n");

    KLVMBasicBlock *entry = KLVMAppendBasicBlock(fn, "entry");
    KLVMBuilder *builder = KLVMBasicBlockBuilder(entry);
    KLVMBasicBlock *_then = KLVMAppendBasicBlock(fn, "_then");
    KLVMBasicBlock *_else = KLVMAppendBasicBlock(fn, "_else");
    KLVMValue *k1 = KLVMConstInt32(1);
    KLVMValue *cond1 = KLVMBuildCondJumpLE(builder, n, k1, _then, _else);

    builder = KLVMBasicBlockBuilder(_then);
    KLVMBuildRet(builder, n);

    builder = KLVMBasicBlockBuilder(_else);
    KLVMValue *args1[] = {
        KLVMBuildSub(builder, n, KLVMConstInt32(1), "sub1"),
        NULL,
    };
    KLVMValue *r1 = KLVMBuildCall(builder, fn, args1, "r1");
    KLVMValue *args2[] = {
        KLVMBuildSub(builder, n, KLVMConstInt32(2), "sub2"),
        NULL,
    };
    KLVMValue *r2 = KLVMBuildCall(builder, fn, args2, "r2");
    KLVMBuildRet(builder, KLVMBuildAdd(builder, r1, r2, "ret"));
}

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
    KLVMSetValueName(v1, "v1");
    KLVMSetValueName(v2, "v2");

    KLVMBasicBlock *entry = KLVMAppendBasicBlock(fn, "entry");
    KLVMBuilder *builder = KLVMBasicBlockBuilder(entry);
    KLVMValue *t1 = KLVMBuildAdd(builder, v1, v2, "t1");
    KLVMValue *ret = KLVMAddVar(m, KLVMInt32Type(), "ret");
    KLVMBuildCopy(builder, t1, ret);
    KLVMBuildRet(builder, ret);
    KLVMBuildRet(builder, KLVMConstInt32(-100));

    KLVMBasicBlock *bb2 = KLVMAppendBasicBlock(fn, "L2");
    builder = KLVMBasicBlockBuilder(bb2);
    KLVMValue *t2 = KLVMBuildSub(builder, v1, KLVMConstInt32(20), "t2");
    KLVMBuildRet(builder, t2);
}

static void test_var(KLVMModule *m)
{
    KLVMBasicBlock *entry = KLVMAppendBasicBlock(KLVMModuleFunc(m), "entry");
    KLVMBuilder *builder = KLVMBasicBlockBuilder(entry);

    KLVMValue *foo = KLVMAddVar(m, KLVMInt32Type(), "foo");
    KLVMBuildCopy(builder, KLVMConstInt32(100), foo);

    KLVMValue *bar = KLVMAddVar(m, KLVMInt32Type(), "bar");
    KLVMValue *k200 = KLVMConstInt32(200);
    KLVMValue *v = KLVMBuildAdd(builder, foo, k200, "v1");
    KLVMBuildCopy(builder, v, bar);

    KLVMValue *baz = KLVMAddVar(m, KLVMInt32Type(), "baz");
    v = KLVMBuildSub(builder, foo, bar, "v2");
    KLVMBuildCopy(builder, v, baz);
}

int main(int argc, char *argv[])
{
    KLVMModule *m;
    m = KLVMCreateModule("test");
    test_var(m);
    test_func(m);
    test_fib(m);
    KLVMDumpModule(m);
    KLVMDestroyModule(m);
    return 0;
}

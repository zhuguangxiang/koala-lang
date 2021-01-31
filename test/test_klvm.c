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
    KLVMSetVarName(n, "n");

    KLVMBasicBlock *entry = KLVMFuncEntryBasicBlock(fn);
    KLVMBuilder *bldr = KLVMBasicBlockBuilder(entry);
    KLVMBasicBlock *_then = KLVMAppendBasicBlock(fn);
    KLVMBasicBlock *_else = KLVMAppendBasicBlock(fn);
    KLVMValue *cond = KLVMBuildCondLE(bldr, n, KLVMConstInt32(1));
    KLVMBuildCondJmp(bldr, cond, _then, _else);

    bldr = KLVMBasicBlockBuilder(_then);
    KLVMBuildRet(bldr, n);

    bldr = KLVMBasicBlockBuilder(_else);
    KLVMValue *args1[] = {
        KLVMBuildSub(bldr, n, KLVMConstInt32(1)),
        NULL,
    };
    KLVMValue *r1 = KLVMBuildCall(bldr, fn, args1);
    KLVMValue *args2[] = {
        KLVMBuildSub(bldr, n, KLVMConstInt32(2)),
        NULL,
    };
    KLVMValue *r2 = KLVMBuildCall(bldr, fn, args2);
    KLVMBuildRet(bldr, KLVMBuildAdd(bldr, r1, r2));
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
    KLVMSetVarName(v1, "v1");
    KLVMSetVarName(v2, "v2");

    KLVMBasicBlock *entry = KLVMFuncEntryBasicBlock(fn);
    KLVMBuilder *bldr = KLVMBasicBlockBuilder(entry);
    KLVMValue *t1 = KLVMBuildAdd(bldr, v1, v2);
    KLVMValue *ret = KLVMAddLocVar(fn, KLVMInt32Type(), "res");
    KLVMBuildCopy(bldr, t1, ret);
    KLVMBuildRet(bldr, ret);
    KLVMBuildRet(bldr, KLVMConstInt32(-100));

    KLVMBasicBlock *bb2 = KLVMAppendBasicBlock(fn);
    bldr = KLVMBasicBlockBuilder(bb2);
    KLVMValue *t2 = KLVMBuildSub(bldr, v1, KLVMConstInt32(20));
    KLVMBuildRet(bldr, t2);
}

static void test_var(KLVMModule *m)
{
    KLVMBasicBlock *entry = KLVMFuncEntryBasicBlock(KLVMModuleFunc(m));
    KLVMBuilder *bldr = KLVMBasicBlockBuilder(entry);

    KLVMValue *foo = KLVMAddVar(m, KLVMInt32Type(), "foo");
    KLVMBuildCopy(bldr, KLVMConstInt32(100), foo);

    KLVMValue *bar = KLVMAddVar(m, KLVMInt32Type(), "bar");
    KLVMValue *k200 = KLVMConstInt32(200);
    KLVMValue *v = KLVMBuildAdd(bldr, foo, k200);
    KLVMBuildCopy(bldr, v, bar);

    KLVMValue *baz = KLVMAddVar(m, KLVMInt32Type(), "baz");
    KLVMBuildCopy(bldr, KLVMBuildSub(bldr, foo, bar), baz);
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

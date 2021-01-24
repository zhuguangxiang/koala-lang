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

static void test_func(KLVMModule *m)
{
    KLVMType *ret = KLVMInt32Type();
    KLVMType *params[] = {
        KLVMInt32Type(),
        KLVMInt32Type(),
        NULL,
    };
    KLVMType *proto = KLVMProtoType(ret, params);
    KLVMValue *fn = KLVMAddFunc(m, proto, "add");
    KLVMValue *v1 = KLVMGetParam(fn, 0);
    KLVMValue *v2 = KLVMGetParam(fn, 1);
    KLVMSetValueName(v1, "v1");
    KLVMSetValueName(v2, "v2");

    KLVMBasicBlock *entry = KLVMAppendBasicBlock(fn, "entry");
    KLVMBuilder *builder = KLVMBasicBlockBuilder(entry);
    KLVMValue *res = KLVMBuildAdd(builder, v1, v2, "res");
    KLVMBuildRet(builder, res);
    KLVMBuildRet(builder, KLVMConstInt32(-100));
}

static void test_var(KLVMModule *m)
{
    KLVMBasicBlock *entry = KLVMAppendBasicBlock(m->fn, "entry");
    KLVMBuilder *builder = KLVMBasicBlockBuilder(entry);

    KLVMValue *foo = KLVMAddVar(m, KLVMInt32Type(), "foo");
    KLVMBuildStore(builder, KLVMConstInt32(100), foo);

    KLVMValue *bar = KLVMAddVar(m, KLVMInt32Type(), "bar");
    KLVMValue *k200 = KLVMConstInt32(200);
    KLVMValue *v = KLVMBuildAdd(builder, foo, k200, "v1");
    KLVMBuildStore(builder, v, bar);

    KLVMValue *baz = KLVMAddVar(m, KLVMInt32Type(), "baz");
    v = KLVMBuildSub(builder, foo, bar, "v2");
    KLVMBuildStore(builder, v, baz);
}

int main(int argc, char *argv[])
{
    KLVMModule *m;
    m = KLVMCreateModule("test");
    test_var(m);
    test_func(m);
    KLVMDumpModule(m);
    KLVMDestroyModule(m);
    return 0;
}

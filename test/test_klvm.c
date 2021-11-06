/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "klvm/klvm.h"

static void test_fib(KLVMModule *m)
{
    TypeDesc *rtype = desc_from_int32();
    TypeDesc *params[] = { desc_from_int32() };
    TypeDesc *proto = desc_from_proto2(rtype, params, 1);
    KLVMFunc *fn = klvm_add_func(m, "fib", proto);
    // KLVMValue *n = klvm_get_param(fn, 0);
    // klvm_set_name(n, "n");

    KLVMBasicBlock *entry = klvm_append_block(fn, "entry");
    KLVMBasicBlock *_then = klvm_append_block(fn, "then");
    KLVMBasicBlock *_else = klvm_append_block(fn, "else");

    KLVMBuilder bldr;

    klvm_set_builder_tail(&bldr, entry);
    /*
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
    */
    klvm_build_ret(&bldr, klvm_const_int32(100));
}

/*
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

    KLVMBasicBlockRef entry = KLVMAppendBlock(fn, "entry");
    KLVMSetBuilderAtEnd(&bldr, entry);
    KLVMValueRef t1 = KLVMBuildAdd(&bldr, v1, v2, "");
    KLVMValueRef ret = KLVMAddLocal(fn, "res", KLVMTypeInt32());
    KLVMBuildCopy(&bldr, ret, t1);
    KLVMBuildRet(&bldr, ret);
    // KLVMBuildRet(&bldr, KLVMConstInt32(-100));

    KLVMBasicBlockRef bb2 = KLVMAppendBlock(fn, "test_bb");
    KLVMSetBuilderAtEnd(&bldr, bb2);
    KLVMValueRef t2 = KLVMBuildSub(&bldr, v1, KLVMConstInt32(20), "");
    KLVMBuildRet(&bldr, t2);

    KLVMBuildJmp(&bldr, bb2);
}

static void test_var(KLVMModule *m)
{
    KLVMFunc *fn = klvm_init_func(m);
    KLVMBasicBlock *entry = klvm_append_block(fn, "entry");

    KLVMBuilder bldr;

    klvm_set_builder_tail(&bldr, entry);
    KLVMVar *foo = klvm_add_var(m, "foo", desc_from_int32());
    KLVMBuildCopy(&bldr, foo, KLVMConstInt32(100));

    KLVMValueRef bar = KLVMAddVariable(m, "bar", KLVMTypeInt32());
    KLVMValueRef k200 = KLVMConstInt32(200);
    KLVMValueRef v = KLVMBuildAdd(&bldr, foo, k200, "");
    KLVMBuildCopy(&bldr, bar, v);

    KLVMValueRef baz = KLVMAddVariable(m, "baz", KLVMTypeInt32());
    KLVMBuildCopy(&bldr, baz, KLVMBuildSub(&bldr, foo, bar, ""));
}
*/

static void test_func2(KLVMModule *m)
{
    KLVMFunc *fn = klvm_init_func(m);
    KLVMValue *v1 = klvm_add_local(fn, desc_from_int32(), "a");
    KLVMValue *v2 = klvm_add_local(fn, desc_from_int32(), "b");
    KLVMValue *v3 = klvm_add_local(fn, desc_from_int32(), "c");

    KLVMBasicBlock *entry = klvm_append_block(fn, "entry");
    KLVMBuilder bldr;
    klvm_set_builder_tail(&bldr, entry);

    klvm_build_copy(&bldr, v1, klvm_const_int32(1));
    klvm_build_copy(&bldr, v2, klvm_const_int32(2));
    KLVMValue *v4 = klvm_build_add(&bldr, v1, v2, null);
    klvm_build_copy(&bldr, v3, v4);
    KLVMValue *v5 = klvm_build_sub(&bldr, v1, v2, null);
    klvm_build_copy(&bldr, v3, v5);
    klvm_build_ret(&bldr, v3);
}

int main(int argc, char *argv[])
{
    init_desc();

    KLVMModule *m = klvm_create_module("test");
    test_fib(m);
    // test_func(m);
    // test_var(m);
    test_func2(m);
    klvm_dump_module(m);

    List list = LIST_INIT(list);
    klvm_add_unreachblock_pass(&list);
    klvm_add_dot_pass(&list);
    klvm_run_passes(&list, m);
    klvm_fini_passes(&list);

    klvm_destroy_module(m);
    fini_desc();
    return 0;
}

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
    KLVMValue *n = klvm_get_param(fn, 0);
    klvm_set_name(n, "n");

    KLVMBasicBlock *entry = klvm_append_block(fn, "entry");
    KLVMBasicBlock *_then = klvm_append_block(fn, "then");
    KLVMBasicBlock *_else = klvm_append_block(fn, "else");

    KLVMBuilder bldr;

    klvm_builder_end(&bldr, entry);
    KLVMValue *cond = klvm_build_cmple(&bldr, n, klvm_const_int32(1), null);
    klvm_build_condjmp(&bldr, cond, _then, _else);

    klvm_builder_end(&bldr, _then);
    klvm_build_ret(&bldr, n);

    klvm_builder_end(&bldr, _else);
    KLVMValue *args1[] = {
        klvm_build_sub(&bldr, n, klvm_const_int32(1), "x1"),
    };
    KLVMValue *r1 = klvm_build_call(&bldr, fn, args1, 1, null);
    KLVMValue *args2[] = {
        klvm_build_sub(&bldr, n, klvm_const_int32(2), ""),
    };
    KLVMValue *r2 = klvm_build_call(&bldr, fn, args2, 1, null);
    klvm_build_ret(&bldr, klvm_build_add(&bldr, r1, r2, null));

    klvm_build_ret(&bldr, klvm_const_int32(100));
}

static void test_func(KLVMModule *m)
{
    KLVMFunc *fn = klvm_init_func(m);

    KLVMBasicBlock *entry = klvm_append_block(fn, "entry");
    KLVMBuilder bldr;
    klvm_builder_end(&bldr, entry);

    KLVMValue *v1 = klvm_build_local(&bldr, desc_from_int32(), "a");
    KLVMValue *v2 = klvm_build_local(&bldr, desc_from_int32(), "b");
    KLVMValue *v3 = klvm_build_local(&bldr, desc_from_int32(), "c");
    KLVMValue *v31 = klvm_build_local(&bldr, desc_from_int32(), "");

    klvm_build_copy(&bldr, v1, klvm_const_int32(1));
    klvm_build_copy(&bldr, v2, klvm_const_int32(2));
    KLVMValue *v4 = klvm_build_add(&bldr, v1, v2, null);
    klvm_build_copy(&bldr, v3, v4);
    KLVMValue *v5 = klvm_build_sub(&bldr, v1, v2, null);
    klvm_build_copy(&bldr, v3, v5);
    klvm_set_name(v5, "xyz");
    klvm_build_ret(&bldr, v3);

    KLVMValue *foo = klvm_add_var(m, "foo", desc_from_int32());
    klvm_build_copy(&bldr, foo, klvm_const_int32(100));
    klvm_build_copy(&bldr, v31, foo);
    klvm_add_var(m, "bar", desc_from_str());
}

int main(int argc, char *argv[])
{
    init_desc();

    KLVMModule *m = klvm_create_module("test");
    test_fib(m);
    test_func(m);
    klvm_dump_module(m);

    KLVMPassGroup group;
    klvm_init_passes(&group);
    klvm_add_unreachblock_pass(&group);
    // klvm_add_dot_pass(&list);
    klvm_add_check_unused_pass(&group);
    klvm_run_passes(&group, m);
    klvm_fini_passes(&group);

    klvm_destroy_module(m);
    fini_desc();
    return 0;
}

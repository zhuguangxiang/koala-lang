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
    KLVMValue *cond = klvm_build_cmple(&bldr, n, klvm_const_int32(1));
    klvm_build_condjmp(&bldr, cond, _then, _else);

    klvm_builder_end(&bldr, _then);
    klvm_build_ret(&bldr, n);

    klvm_builder_end(&bldr, _else);
    KLVMValue *args1[] = {
        klvm_build_sub(&bldr, n, klvm_const_int32(1)),
    };
    KLVMValue *r1 = klvm_build_call(&bldr, fn, args1, 1);
    KLVMValue *args2[] = {
        klvm_build_sub(&bldr, n, klvm_const_int32(2)),
    };
    KLVMValue *r2 = klvm_build_call(&bldr, fn, args2, 1);
    klvm_build_ret(&bldr, klvm_build_add(&bldr, r1, r2));
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
    KLVMValue *v4 = klvm_build_add(&bldr, v1, v2);
    klvm_build_copy(&bldr, v3, v4);
    KLVMValue *v5 = klvm_build_sub(&bldr, v1, v2);
    klvm_build_copy(&bldr, v3, v5);
    klvm_set_name(v5, "xyz");
    klvm_build_ret(&bldr, v3);

    KLVMValue *foo = klvm_add_var(m, "foo", desc_from_int32());
    klvm_build_copy(&bldr, foo, klvm_const_int32(100));
    klvm_build_copy(&bldr, v31, foo);
    klvm_add_var(m, "bar", desc_from_str());
}

static void test_func2(KLVMModule *m)
{
    /*
        func sum(n int) int {
            i := 0
            sum := 0
            while (i < n) {
                sum = sum + i;
                i = i + 1;
            }
            return sum;
        }
     */
    TypeDesc *rtype = desc_from_int32();
    TypeDesc *params[] = { desc_from_int32() };
    TypeDesc *proto = desc_from_proto2(rtype, params, 1);
    KLVMFunc *fn = klvm_add_func(m, "sum", proto);
    KLVMValue *n = klvm_get_param(fn, 0);
    klvm_set_name(n, "n");

    KLVMBasicBlock *entry = klvm_append_block(fn, "entry");
    KLVMBuilder bldr;
    klvm_builder_end(&bldr, entry);

    KLVMValue *i = klvm_build_local(&bldr, desc_from_int32(), "");
    klvm_build_copy(&bldr, i, klvm_const_int32(0));
    KLVMValue *sum = klvm_build_local(&bldr, desc_from_int32(), "sum");
    klvm_build_copy(&bldr, sum, klvm_const_int32(0));

    KLVMBasicBlock *cond = klvm_append_block(fn, "cond");
    // link to entry block
    klvm_build_jmp(&bldr, cond);

    KLVMBasicBlock *loop = klvm_append_block(fn, "");
    KLVMBasicBlock *end = klvm_append_block(fn, "");

    /* condition block */
    klvm_builder_end(&bldr, cond);
    KLVMValue *cmplt = klvm_build_cmplt(&bldr, i, n);
    klvm_build_condjmp(&bldr, cmplt, loop, end);

    /* loop body block */
    klvm_builder_end(&bldr, loop);
    KLVMValue *tmp = klvm_build_add(&bldr, sum, klvm_const_int32(1));
    klvm_build_copy(&bldr, sum, tmp);
    tmp = klvm_build_add(&bldr, i, klvm_const_int32(1));
    klvm_build_copy(&bldr, i, tmp);
    klvm_build_jmp(&bldr, cond);

    /* end block */
    klvm_builder_end(&bldr, end);
    klvm_build_ret(&bldr, sum);
}

int main(int argc, char *argv[])
{
    init_desc();

    KLVMModule *m = klvm_create_module("test");
    test_fib(m);
    test_func(m);
    test_func2(m);
    klvm_dump_module(m);

    KLVMPassGroup group;
    klvm_init_passes(&group);

    // klvm_add_pass(&group, klvm_check_unused_block_pass, null);
    klvm_add_pass(&group, klvm_check_unused_value_pass, null);
    // klvm_add_pass(&group, klvm_print_liveness_pass, null);
    klvm_run_passes(&group, m);

    klvm_fini_passes(&group);

    klvm_dump_module(m);

    klvm_destroy_module(m);
    fini_desc();
    return 0;
}

/*----------------------------------------------------------------------------*\
|* This file is part of the KLVM project, under the MIT License.              *|
|* Copyright (c) 2021-2021 James <zhuguangxiang@gmail.com>                    *|
\*----------------------------------------------------------------------------*/

#include "klvm.h"

/*
gcc -g -fPIC -shared -Wall src/klvm_module.c src/klvm_type.c src/klvm_inst.c \
src/klvm_printer.c src/binheap.c src/hashmap.c src/vector.c -I./include -o \
libklvm.so

gcc -g test/test_klvm.c -I./include -lklvm -L./
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:.
*/

static void test_fib(klvm_module_t *m)
{
    klvm_type_t *rtype = &klvm_type_int32;
    klvm_type_t *params[] = {
        &klvm_type_int32,
        NULL,
    };
    klvm_type_t *proto = klvm_type_proto(rtype, params);
    klvm_value_t *fn = klvm_new_func(m, proto, "fib");
    klvm_value_t *n = klvm_get_param(fn, 0);
    klvm_set_name(n, "n");

    klvm_block_t *entry = klvm_append_block(fn, "entry");
    klvm_builder_t bldr;
    klvm_set_builder(&bldr, entry);

    klvm_block_t *_then = klvm_append_block(fn, "if.then");
    klvm_block_t *_else = klvm_append_block(fn, "if.else");

    // entry block
    klvm_value_t *k = klvm_const_int32(1);
    klvm_value_t *cond = klvm_build_cmple(&bldr, n, k, "");
    klvm_build_branch(&bldr, cond, _then, _else);

    // if.then block
    klvm_set_builder(&bldr, _then);
    klvm_build_ret(&bldr, n);

    // if.else/if.end block
    klvm_set_builder(&bldr, _else);

    klvm_value_t *args[] = {
        klvm_build_sub(&bldr, n, klvm_const_int32(1), ""),
        NULL,
    };
    klvm_value_t *r1 = klvm_build_call(&bldr, fn, args, "");

    args[0] = klvm_build_sub(&bldr, n, klvm_const_int32(2), "x");
    klvm_value_t *r2 = klvm_build_call(&bldr, fn, args, "");
    klvm_build_ret(&bldr, klvm_build_add(&bldr, r1, r2, ""));
}

static void test_func(klvm_module_t *m)
{
    klvm_type_t *rtype = &klvm_type_int32;
    klvm_type_t *params[] = {
        &klvm_type_int32,
        &klvm_type_int32,
        NULL,
    };
    klvm_type_t *proto = klvm_type_proto(rtype, params);
    klvm_value_t *fn = klvm_new_func(m, proto, "add");
    klvm_value_t *v1 = klvm_get_param(fn, 0);
    klvm_value_t *v2 = klvm_get_param(fn, 1);
    klvm_set_name(v1, "v1");
    klvm_set_name(v2, "v2");

    klvm_block_t *entry = klvm_append_block(fn, "entry");
    klvm_builder_t bldr;
    klvm_set_builder(&bldr, entry);

    klvm_value_t *t1 = klvm_build_add(&bldr, v1, v2, "");
    klvm_value_t *ret = klvm_new_local(fn, &klvm_type_int32, "res");
    klvm_build_copy(&bldr, t1, ret);
    klvm_build_ret(&bldr, ret);
    klvm_build_ret(&bldr, klvm_const_int32(-100));

    klvm_block_t *bb2 = klvm_append_block(fn, "test_bb");
    klvm_set_builder(&bldr, bb2);
    klvm_value_t *t2 = klvm_build_sub(&bldr, v1, klvm_const_int32(20), "");
    klvm_build_ret(&bldr, t2);

    klvm_build_goto(&bldr, bb2);
}

static void test_var(klvm_module_t *m)
{
    klvm_value_t *_init_ = klvm_init_func(m);
    klvm_block_t *entry = klvm_append_block(_init_, "entry");
    klvm_builder_t bldr;

    klvm_set_builder(&bldr, entry);

    klvm_value_t *foo = klvm_new_var(m, &klvm_type_int32, "foo");
    klvm_build_copy(&bldr, klvm_const_int32(100), foo);

    klvm_value_t *bar = klvm_new_var(m, &klvm_type_int32, "bar");
    klvm_value_t *k200 = klvm_const_int32(200);
    klvm_value_t *v = klvm_build_add(&bldr, foo, k200, "");
    klvm_build_copy(&bldr, v, bar);

    klvm_value_t *baz = klvm_new_var(m, &klvm_type_int32, "baz");
    klvm_build_copy(&bldr, klvm_build_sub(&bldr, foo, bar, ""), baz);
}

int hello_pass(klvm_func_t *fn, void *data)
{
    printf("hello pass: %s\n", fn->name);
    return 0;
}

int main(int argc, char *argv[])
{
    klvm_module_t *m;
    m = klvm_create_module("test");

    test_var(m);
    test_func(m);
    test_fib(m);

    klvm_dump_module(m);

    klvm_pass_t pass = { hello_pass, NULL };
    klvm_register_pass(m, &pass);
    klvm_run_passes(m);

    klvm_destroy_module(m);

    return 0;
}

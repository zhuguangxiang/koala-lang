/*----------------------------------------------------------------------------*\
|* This file is part of the koala project, under the MIT License.             *|
|* Copyright (c) 2021-2021 James <zhuguangxiang@gmail.com>                    *|
\*----------------------------------------------------------------------------*/

#include "klvm.h"

#ifdef __cplusplus
extern "C" {
#endif

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
    klvm_builder_init(&bldr, entry);

    klvm_block_t *_then = klvm_append_block(fn, "if.then");
    klvm_block_t *_else = klvm_append_block(fn, "if.else");

    // entry block
    klvm_value_t *k = klvm_const_int32(1);
    klvm_value_t *cond = klvm_build_cmple(&bldr, n, k, "");
    klvm_build_branch(&bldr, cond, _then, _else);

    // if.then block
    klvm_builder_init(&bldr, _then);
    klvm_build_ret(&bldr, n);

    // if.else/if.end block
    klvm_builder_init(&bldr, _else);

    klvm_value_t *args[] = {
        klvm_build_sub(&bldr, n, klvm_const_int32(1), ""),
        NULL,
    };
    klvm_value_t *r1 = klvm_build_call(&bldr, fn, args, "");

    args[0] = klvm_build_sub(&bldr, n, klvm_const_int32(2), "x");
    klvm_value_t *r2 = klvm_build_call(&bldr, fn, args, "");
    klvm_build_ret(&bldr, klvm_build_add(&bldr, r1, r2, ""));
}

#ifdef __cplusplus
}
#endif

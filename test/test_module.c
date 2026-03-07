/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "atom.h"
#include "cfuncobject.h"
#include "exception.h"
#include "log.h"
#include "moduleobject.h"
#include "object.h"
#include "opcode.h"
#include "run.h"

#ifdef __cplusplus
extern "C" {
#endif

void test_module(void)
{
    Object *m = kl_new_module("main");

    cp_add_int(m, 100);
    cp_add_str(m, "hello");

    int id = kl_add_rel_mod(m, "std/builtin");
    kl_add_rel_func(m, "print", id);

    kl_do_link(m);

    // /* print(100, "hello") */
    // char _insns[] = {
    //     OP_CONST_INT_IMM8, 0, 100, OP_PUSH, 0, OP_CONST_LOAD, 0, 1, 0, OP_PUSH, 0,
    //     OP_CALL,           1, 0,   2,       0, OP_RETURN,     0,
    // };

    // CodeObject *code = (CodeObject *)kl_new_code("__init__", m, NULL);
    // code->insns = _insns;
    // code->stack_size = 2;

    // Value self = obj_value(code);
    // Value result = object_call(&self, NULL, 0, NULL);
    // ASSERT(IS_NONE(&result));
}

int main(int argc, char *argv[])
{
    init_log(LOG_INFO, NULL, 0);
    init_atom();
    kl_init(argc, argv);
    test_module();
    kl_fini();
    fini_atom();
    return 0;
}

#ifdef __cplusplus
}
#endif

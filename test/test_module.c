/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

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

    module_add_int_const(m, 100);
    module_add_str_const(m, "hello");

    SymbolInfo sym = { .name = "print" };
    module_add_rel(m, "builtin", &sym);

    kl_module_link(m);

    /* print(100, "hello") */
    char _insns[] = {
        OP_CONST_INT_IMM8, 0, 100, OP_PUSH, 0, OP_CONST_LOAD, 0, 1, 0, OP_PUSH, 0,
        OP_CALL,           1, 0,   2,       0, OP_RETURN,     0,
    };

    CodeObject *code = (CodeObject *)kl_new_code("__init__", m, NULL);
    code->cs.insns = _insns;
    code->cs.insns_size = sizeof(_insns);
    code->cs.nargs = 0;
    code->cs.nlocals = 1;
    code->cs.stack_size = 2;

    Value self = obj_value(code);
    Value result = object_call(&self, NULL, 0, NULL);
    ASSERT(IS_NONE(&result));
}

int main(int argc, char *argv[])
{
    init_log(LOG_WARN, NULL, 0);
    kl_init(argc, argv);
    test_module();
    kl_fini();
    return 0;
}

#ifdef __cplusplus
}
#endif

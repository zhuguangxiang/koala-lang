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
#include "stringobject.h"
#include "tupleobject.h"

#ifdef __cplusplus
extern "C" {
#endif

void test_get_int_method(void)
{
    Object *m = kl_new_module("main");

    module_add_int_const(m, 100);
    module_add_str_const(m, "hello");
    module_add_str_const(m, ", ");

    Object *tuple = kl_new_tuple(1);
    Value *items = TUPLE_ITEMS(tuple);
    Object *sep = kl_new_str("sep");
    items[0] = obj_value(sep);
    module_add_obj_const(m, tuple);

    SymbolInfo sym = { .name = "print" };
    module_add_rel(m, "builtin", &sym);
    sym.name = "__hash__";
    module_add_rel(m, "builtin.int", &sym);

    kl_module_link(m);

    /* int_hash := int.__hash__
       print(int_hash)
    */
    char _insns[] = {
        OP_REL_LOAD, 0,       2, 0,           OP_PUSH, 0, OP_CALL,   1,
        0,           1,       0, OP_REL_LOAD, 0,       1, 0,         OP_PUSH,
        0,           OP_CALL, 1, 0,           1,       0, OP_RETURN, 0,
    };

    CodeObject *code = (CodeObject *)kl_new_code("__init__", m, NULL);
    code->cs.insns = _insns;
    code->cs.insns_size = sizeof(_insns);
    code->cs.nargs = 0;
    code->cs.nlocals = 1;
    code->cs.stack_size = 1;

    Value self = obj_value(code);
    Value result = object_call(&self, NULL, 0, NULL);
    ASSERT(IS_NONE(&result));
}

int main(int argc, char *argv[])
{
    init_log(LOG_WARN, NULL, 0);
    kl_init(argc, argv);
    test_get_int_method();
    kl_fini();
    return 0;
}

#ifdef __cplusplus
}
#endif

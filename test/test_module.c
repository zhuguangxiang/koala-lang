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
#include "opcode2.h"
#include "run.h"

#ifdef __cplusplus
extern "C" {
#endif

void test_module(void)
{
    Object *m = kl_new_module("main");

    int s_id = cp_add_str(m, "hello");

    int id = kl_add_rel_mod(m, "std/builtin");
    id = kl_add_rel_func(m, "print", id);

    kl_do_link(m);

    /* print(100, "hello") */
    uint32_t _insns[] = {
        (OP_CONST_INT_IMM << 24) | (0 << 16) | 100,
        (OP_CONST << 24) | (1 << 12) | s_id,
        (OP_PUSH << 24) | 0,
        (OP_PUSH << 24) | 1,
        (OP_CALL << 24) | (0 << 16) | 2 << 8 | id,
        (OP_RETURN_NONE << 24),
    };

    Object *obj = kl_new_code("__init__", m, NULL);
    CodeObject *code = (CodeObject *)obj;
    code->insns = (char *)_insns;
    code->nlocals = 2;
    code->max_nargs = 2;

    Value self = obj_value(code);
    Value result = object_call(&self, NULL, 0);
    ASSERT(is_none(&result));
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

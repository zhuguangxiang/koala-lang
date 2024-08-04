/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "codeobject.h"
#include "log.h"
#include "moduleobject.h"
#include "opcode.h"
#include "run.h"

int main(int argc, char *argv[])
{
    init_log(LOG_INFO, NULL, 0);
    kl_init(argc, argv);

    Object *m = kl_new_module("fib");

    char fib_insns[] = {
        OP_JMP_INT_CMP_GE_IMM8,
        0,
        2,
        7,
        0,
        OP_RETURN,
        0,
        OP_INT_SUB_IMM8,
        1,
        0,
        1,
        OP_PUSH,
        1,
        OP_CALL,
        0,
        0,
        1,
        1,
        OP_INT_SUB_IMM8,
        2,
        0,
        2,
        OP_PUSH,
        2,
        OP_CALL,
        0,
        0,
        1,
        2,
        OP_INT_ADD,
        0,
        1,
        2,
        OP_RETURN,
        0,
    };

    CodeObject *code = (CodeObject *)kl_new_code("fib", m, NULL);
    code->insns = fib_insns;
    code->num_insns = sizeof(fib_insns);
    code->nargs = 1;
    code->nlocals = 4;
    code->stack_size = 1;
    module_add_code(m, (Object *)code);

    // Object *fn = module_get_symbol(m, 0, 0);
    Value args[] = { IntValue(40) };
    Value result = object_call((Object *)code, args, 1);
    printf("%ld\n", result.ival);

    kl_fini();
    return 0;
}

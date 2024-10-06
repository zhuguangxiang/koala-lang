/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "codeobject.h"
#include "klc.h"
#include "log.h"
#include "moduleobject.h"
#include "opcode.h"
#include "run.h"

CodeSpec *get_code_spec(KlcFile *klc);

#ifdef __cplusplus
extern "C" {
#endif

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

    printf("write to klc file\n");

    KlcFile klc;
    init_klc_file(&klc, "example.klc");
    CodeSpec cs;
    cs.insns = fib_insns;
    cs.insns_size = sizeof(fib_insns);
    cs.nargs = 1;
    cs.nlocals = 4;
    cs.stack_size = 1;
    klc_add_code(&klc, &cs);
    write_klc_file(&klc);

    KlcFile klc2;
    init_klc_file(&klc2, "example.klc");
    read_klc_file(&klc2);

    CodeObject *code = (CodeObject *)kl_new_code("fib", m, NULL);
    CodeSpec *cs2 = get_code_spec(&klc2);
    code->cs.insns = cs2->insns;
    code->cs.insns_size = cs2->insns_size;
    code->cs.nargs = cs2->nargs;
    code->cs.nlocals = cs2->nlocals;
    code->cs.stack_size = cs2->stack_size;
    module_add_object(m, "fib", (Object *)code);

    // Object *fn = module_get_symbol(m, 0, 0);
    Value self = obj_value(code);
    Value args[] = { int_value(40) };
    Value result = object_call(&self, args, 1, NULL);
    printf("%ld\n", result.ival);

    kl_fini();
    return 0;
}

#ifdef __cplusplus
}
#endif

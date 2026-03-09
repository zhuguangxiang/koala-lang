/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "atom.h"
#include "codeobject.h"
#include "log.h"
#include "moduleobject.h"
#include "opcode.h"
#include "run.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
func fib(v int) int {
    if v < 2 { return v }
    return fib(v - 1) + fib(v - 2)
}
*/
int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <num>\n", argv[0]);
        return 1;
    }

    init_log(LOG_INFO, NULL, 0);
    init_atom();
    kl_init(argc, argv);

    Object *m = kl_new_module("fib");

    int id = kl_add_rel_mod(m, "fib");
    id = kl_add_rel_func(m, "fib", id);

    /* time = 3.8, python3.13.9 7.1, lua5.4 4.2 */
    uint32_t _insns[] = {
        (OP_JMP_INT_CMP_GE_IMM << 24) | 0 << 16 | 2 << 8 | 1,
        (OP_RETURN << 24) | 0,
        (OP_INT_SUB_IMM << 24) | 1 << 16 | 0 << 8 | 1,
        (OP_PUSH << 24) | 1,
        (OP_CALL << 24) | 1 << 16 | 1 << 8 | id,
        (OP_INT_SUB_IMM << 24) | 2 << 16 | 0 << 8 | 2,
        (OP_PUSH << 24) | 2,
        (OP_CALL << 24) | 0 << 16 | 1 << 8 | id,
        (OP_INT_ADD << 24) | 0 << 16 | 1 << 8 | 0,
        (OP_RETURN << 24) | 0,
    };

    Object *obj = kl_new_code("fib", m, NULL);
    CodeObject *code = (CodeObject *)obj;
    code->insns = (char *)_insns;
    code->nlocals = 3;
    code->max_nargs = 1;

    module_add_obj(m, "fib", obj);

    kl_do_link(m);

    Value self = obj_value(code);
    int input = strtol(argv[1], NULL, 10);
    Value args[] = { int64_value(input) };
    Value result = object_call(&self, args, 1);
    printf("%ld\n", result.ival);

    kl_fini();
    fini_atom();
    return 0;
}

#ifdef __cplusplus
}
#endif

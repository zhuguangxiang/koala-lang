/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "koala.h"

#ifdef __cplusplus
extern "C" {
#endif

void test_module(void)
{
    Object *m = kl_new_module("main");

    int cp_id = kl_mo_add_str(m, "hello");
    int import_id = kl_mo_add_import(m, IMPORT_KIND_FUNC, "std/builtin", "print");

    // kl_do_link(m);

    /* print(100, "hello") */
    uint32_t x = (OP_LOADK << 24) | (1 << 16) | cp_id;

    uint32_t _insns[] = {
        (OP_LOAD_INT_IMM << 24) | (0 << 16) | 100,
        0,
        (OP_PUSH << 24) | 0,
        (OP_PUSH << 24) | 1,
        (OP_CALL << 24) | (1 << 20) | (0xFFFu << 8) | 2,
        0,
        (OP_RET_VOID << 24),
    };

    _insns[1] = x;
    _insns[5] = import_id;

    kl_mo_set_code(m, _insns, COUNT_OF(_insns));

    Object *obj = kl_new_code("__init__", m);
    CodeObject *code = (CodeObject *)obj;
    code->cs.start_pc = 0;
    code->cs.nlocals = 2;
    kl_mo_add_func(m, obj);

    kl_dump_module(m);

    kl_run_module(m);
}

int main(int argc, char *argv[])
{
    koala_initialize();
    test_module();
    koala_finalize();
    return 0;
}

#ifdef __cplusplus
}
#endif

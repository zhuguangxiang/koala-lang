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

void test_type_call(void)
{
    Object *m = kl_new_module("test_type_call");
    module_add_type(m, &int_type);

    /*
    int(100, 16)
    */
    char _insns[] = {
        OP_PUSH_IMM8, 100, OP_PUSH_IMM8, 16, OP_CALL, 0, 0, 2, 0, OP_RETURN, 0,
    };

    CodeObject *code = (CodeObject *)kl_new_code("init", m, NULL);
    code->cs.insns = _insns;
    code->cs.insns_size = sizeof(_insns);
    code->cs.nargs = 0;
    code->cs.nlocals = 1;
    code->cs.stack_size = 1;
    module_add_code(m, (Object *)code);

    Value self = obj_value(code);
    Value result = object_call(&self, NULL, 0, NULL);
    if (IS_ERROR(&result)) {
        print_exc();
    } else {
        printf("%ld\n", result.ival);
    }
}

int main(int argc, char *argv[])
{
    init_log(LOG_INFO, NULL, 0);
    kl_init(argc, argv);
    test_type_call();
    kl_fini();
    return 0;
}

#ifdef __cplusplus
}
#endif

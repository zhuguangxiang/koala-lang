/*===-- test_vm.c --------------------------------====-------------*- C -*-===*\
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) 2020 James, https://github.com/zhuguangxiang                 *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* Test vm in `vm.h` and `vm.c`                                               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "codeobject.h"
#include "koala.h"
#include "opcode.h"
#include <assert.h>

void test_vm(void)
{
    KoalaState *ks = kl_new_state();
    uint8_t codes[] = {
        OP_LOAD_0,
        OP_LOAD_1,
        OP_ADD,
        OP_RETURN_VALUE,
    };
    Object *code = code_new(codes, COUNT_OF(codes));
    kl_push_code(ks, code);
    kl_push_byte(ks, 100);
    kl_push_byte(ks, 200);
    kl_do_call(ks, 2);
}

int main(int argc, char *argv[])
{
    init_core_types();
    init_code_type();
    test_vm();
    return 0;
}

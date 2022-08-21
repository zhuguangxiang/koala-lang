
#include "eval.h"
#include "func.h"

/*
func add(a int) int {
    return a + a + 200
}

var res = add(100)
-------------------------
add:
    op_i32_const 200
    op_local_get 0
    op_i32_add
    op_local_get 0
    op_i32_add
    op_return_value
=====
op_i32_const 100
op_call 0
op_local_set 0
=====
reloc_method_list:
    module
    name
    address
global_list:

*/

uint8_t add_codes[] = {
    OP_I32_CONST,
    200,
    0,
    0,
    0,
    OP_LOCAL_GET,
    0,
    OP_I32_ADD,
    OP_LOCAL_GET,
    0,
    OP_I32_ADD,
    OP_RETURN_VALUE,
};

KlLocal locals[10] = {
    { "a", NULL, 0, 1 },
    { "res", NULL, 0, 1 },
};

KlFunc add_meth = {
    .name = "add",
    .num_params = 1,
    .param_size = 1,
    .stack_size = 1,
    .ret_size = 1,
    .code_size = sizeof(add_codes),
    .codes = add_codes,
    .locals = locals,
    .num_locals = 1,
};

KlFunc *psudo_get_method(int index)
{
    return &add_meth;
}

void kl_func_update_stacksize(KlFunc *func);

void test_add()
{
    uint8_t codes[] = {
        OP_I32_CONST, 100, 0, 0, 0, OP_CALL, 0, 0, OP_LOCAL_SET, 0, OP_RETURN,
    };

    KlFunc meth = {
        .name = "_start_",
        .code_size = sizeof(codes),
        .codes = codes,
        .stack_size = 1,
        .locals = locals + 1,
        .num_locals = 1,
    };

    StkVal stack[1024] = { 0 };
    KlState ks = {
        .base = stack,
        .last = stack + 1024,
    };

    ks.cf = &ks.base_cf;

    KlFrame *cf = ks.cf;
    cf->base = ks.base;
    cf->top = cf->base;

    ks.top = cf->top;

    kl_do_call(&ks, &meth);
}

int main()
{
    test_add();
    return 0;
}

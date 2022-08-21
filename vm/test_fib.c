
#include "eval.h"
#include "func.h"

/*
func fib(a int) int {
    if a <= 2 {
        return a
    }
    return fib(a - 1) + fib(a - 2)
}

var res = fib(40)
-------------------------
fib:
    op_i32_const 2
    op_local_get 0
    op_jmp_i32_cmpgt 3
    op_local_get 0
    op_return_value
    op_i32_const 1
    op_local_get 0
    op_i32_sub
    op_call 0
    op_i32_const 2
    op_local_get 0
    op_i32_sub
    op_call 0
    op_i32_add
    op_return_value

=====
op_i32_const 40
op_call 0
op_local_set 0
=====
reloc_method_list:
    module
    name
    address
global_list:

*/

uint8_t fib_codes[] = {
    OP_LOCAL_GET_I32,
    0,
    0,
    OP_I32_CONST_2,
    OP_JMP_INT32_GT,
    2,
    0,
    OP_I32_CONST_1,
    OP_I32_RETURN,

    OP_LOCAL_GET_I32,
    0,
    0,
    OP_I32_CONST_1,
    OP_I32_SUB,
    OP_CALL,
    0,
    0,

    OP_LOCAL_GET_I32,
    0,
    0,
    OP_I32_CONST_2,
    OP_I32_SUB,
    OP_CALL,
    0,
    0,

    OP_I32_ADD,
    OP_I32_RETURN,
};

KlLocal locals[10] = {
    { "a", NULL, 0, 1 },
    { "res", NULL, 0, 1 },
};

KlFunc fib_meth = {
    .name = "fib",
    .num_params = 1,
    .param_size = 1,
    .stack_size = 1,
    .ret_size = 1,
    .code_size = sizeof(fib_codes),
    .codes = fib_codes,
    .locals = locals,
    .num_locals = 1,
};

KlFunc *psudo_get_method(int index)
{
    return &fib_meth;
}

void kl_func_update_stacksize(KlFunc *func);

void test_fib()
{
    uint8_t codes[] = {
        OP_I8_PUSH, 40, OP_CALL, 0, 0, OP_LOCAL_SET_I32, 0, 0, OP_RETURN,
    };

    KlFunc meth = {
        .name = "_start_",
        .code_size = sizeof(codes),
        .codes = codes,
        .stack_size = 1,
        .locals = locals + 1,
        .num_locals = 1,
    };

    StkVal stack[1024];
    KlState ks = {
        .base = stack,
        .last = stack + 1024,
    };

    ks.cf = &ks.base_cf;

    KlFrame *cf = ks.cf;
    cf->back = NULL;
    cf->prev = NULL;
    cf->base = ks.base;
    cf->top = cf->base;

    ks.top = cf->top;

    kl_do_call(&ks, &meth);
    // char buf[128];
    // snprintf(buf, 127, "%d\n", (int)ks.top[0]);
    // assert(ks.top == ks.base);
    printf("%d\n", (int)ks.top[0]);
}

int main()
{
    test_fib();
    return 0;
}

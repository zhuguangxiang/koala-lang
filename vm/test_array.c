
#include "eval.h"
#include "func.h"

struct Point {
    int8_t x;
    int8_t y;
};
/*

arr := [int8; 100]
for i in 0 ..< arr.length {
    arr[i] = i
}

array_i8_new 100
local_array_set 0, 0

i32_const_0
local_i32_set 4, 0

loop:
local_i32_get 4, 0
local_lable_get 0, 0
array_length
jmp_i32_ge next

local_i32_get 4, 0
local_i32_get 4, 0
local_lable_get 0, 0
array_i8_set

local_i32_inc 4, 0
jmp loop

next:
return arr

*/

KlLocal locals[10] = {
    { "arr", NULL, 0, 4 },
    { "i", NULL, 4, 1 },
};

KlFunc *psudo_get_method(int index)
{
    return NULL;
}

void kl_func_update_stacksize(KlFunc *func);
void kl_fini_state(KlState *ks);

struct Array {
    int size;
    char gc;
    int8_t *data;
};

void test_array()
{
    uint8_t codes[] = {
        OP_ARRAY_I8_NEW,
        128,
        0,
        OP_LOCAL_ARRAY_SET,
        0,
        0,
        OP_I32_CONST_0,
        OP_LOCAL_I32_SET,
        4,
        0,

        OP_LOCAL_I32_GET,
        4,
        0,
        OP_LOCAL_LABEL_GET,
        0,
        0,
        OP_ARRAY_LENGTH,
        OP_JMP_INT32_GE,
        17,
        0,

        OP_LOCAL_I32_GET,
        4,
        0,
        OP_LOCAL_I32_GET,
        4,
        0,
        OP_LOCAL_LABEL_GET,
        0,
        0,
        OP_ARRAY_I8_SET,

        OP_LOCAL_I32_INC,
        4,
        0,
        1,

        OP_JMP,
        0xe5,
        0xff,

        OP_LOCAL_ARRAY_GET,
        0,
        0,

        OP_ARRAY_RETURN,
    };

    KlFunc meth = {
        .name = "_start_",
        .code_size = sizeof(codes),
        .codes = codes,
        .stack_size = 4 + 1,
        .locals = locals + 0,
        .num_locals = 2,
    };

    StkVal stack[1024] = { 0 };
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
    assert(ks.top == ks.base);
    // printf("%d\n", (int)ks.top[0]);
    struct Array *arr = (struct Array *)ks.top;
    for (int i = 0; i < 128; i++) {
        printf("%d\n", arr->data[i]);
    }
    kl_fini_state(&ks);
}

int main()
{
    test_array();
    return 0;
}

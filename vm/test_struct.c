
#include "eval.h"
#include "func.h"

struct Point {
    int8_t x;
    int8_t y;
};

/*
struct Point {
    x int8
    y int8

    func add(o Point) {
        this.x += o.x
        this.y += o.y
    }
}

var p = Point()
p.x = 12
p.y = 24
var p1 = Point()
p1.x = 36
p1.y = 48
p.add(p1)

-------------------------
Point.add:
    local_ref_get 0
    field_i8_get 0
    local_label_get 2
    field_i8_get 0
    i32_add
    local_ref_get 0
    field_i8_set 0

    local_ref_get 0
    field_i8_get 1
    local_label_get 2
    field_i8_get 1
    i32_add
    local_ref_get 0
    field_i8_set 1

    return

=====
local_label_get 0
i8_push 12
field_i8_set 0

local_label_get 0
i8_push 24
field_i8_set 1

local_label_get 0
local_sized_get 1 1
op_call 0
=====
reloc_method_list:
    module
    name
    address
global_list:

*/

uint8_t point_add_codes[] = {
    OP_LOCAL_REF_GET,
    0,
    0,
    OP_FIELD_I8_GET,
    0,
    0,
    OP_LOCAL_LABEL_GET,
    2,
    0,
    OP_FIELD_I8_GET,
    0,
    0,
    OP_I32_ADD,
    OP_LOCAL_REF_GET,
    0,
    0,
    OP_FIELD_I8_SET,
    0,
    0,

    OP_LOCAL_REF_GET,
    0,
    0,
    OP_FIELD_I8_GET,
    1,
    0,
    OP_LOCAL_LABEL_GET,
    2,
    0,
    OP_FIELD_I8_GET,
    1,
    0,
    OP_I32_ADD,
    OP_LOCAL_REF_GET,
    0,
    0,
    OP_FIELD_I8_SET,
    1,
    0,

    OP_RETURN,
};

KlLocal locals[10] = {
    { "a", NULL, 0, 1 },
    { "b", NULL, 1, 1 },
    { "res1", NULL, 0, 1 },
    { "res2", NULL, 1, 1 },
};

KlFunc point_add_meth = {
    .name = "Point.add",
    .param_size = 3,
    .stack_size = 3,
    .ret_size = 0,
    .code_size = sizeof(point_add_codes),
    .codes = point_add_codes,
    .locals = locals,
    .num_locals = 2,
};

KlFunc *psudo_get_method(int index)
{
    return &point_add_meth;
}

void kl_func_update_stacksize(KlFunc *func);
void kl_fini_state(KlState *ks);

void test_struct()
{
    uint8_t codes[] = {
        OP_I8_PUSH,
        12,
        OP_LOCAL_LABEL_GET,
        0,
        0,
        OP_FIELD_I8_SET,
        0,
        0,
        OP_I8_PUSH,
        24,
        OP_LOCAL_LABEL_GET,
        0,
        0,
        OP_FIELD_I8_SET,
        1,
        0,
        OP_I8_PUSH,
        36,
        OP_LOCAL_LABEL_GET,
        1,
        0,
        OP_FIELD_I8_SET,
        0,
        0,
        OP_I8_PUSH,
        48,
        OP_LOCAL_LABEL_GET,
        1,
        0,
        OP_FIELD_I8_SET,
        1,
        0,

        OP_LOCAL_LABEL_GET,
        0,
        0,
        OP_LOCAL_SIZED_GET,
        1,
        0,
        1,
        0,
        OP_CALL,
        0,
        0,
        OP_RETURN,
    };

    KlFunc meth = {
        .name = "_start_",
        .code_size = sizeof(codes),
        .codes = codes,
        .stack_size = 2,
        .locals = locals + 2,
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
    struct Point *p = (struct Point *)cf->base;
    assert(p->x == 48);
    // assert(p->y == 72);
    printf("(12, 24) + (36, 48) = (%d, %d)\n", p->x, p->y);
    kl_fini_state(&ks);
}

int main()
{
    test_struct();
    return 0;
}

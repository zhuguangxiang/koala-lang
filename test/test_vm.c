/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangXiang https://github.com/zhuguangxiang               *|
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
    TypeDesc *proto = to_proto("ii", "i");
    Object *code = code_new("add", proto, codes, COUNT_OF(codes));
    code_add_locvar(code, "x", &kl_type_int);
    code_add_locvar(code, "y", &kl_type_int);
    Object *meth = method_new("add", code);
    kl_push_func(ks, meth);
    kl_push_int(ks, 100);
    kl_push_int(ks, 200);
    kl_do_call(ks);
    int64_t res = kl_pop_int(ks);
    assert(res == 300);
    assert(ks->top == ks->ci->top);
    // kl_free_state(ks);
}

DLLEXPORT int8_t test_add_func(int8_t a, int8_t b)
{
    printf("test_add_func is call by kvm\n");
    return a + b;
}

void test_vm2(void)
{
    KoalaState *ks = kl_new_state();
    MethodDef def = { "add", "bb", "b", "test_add_func" };
    Object *meth = cmethod_new(&def);
    kl_push_func(ks, meth);
    kl_push_byte(ks, 10);
    kl_push_byte(ks, 20);
    kl_do_call(ks);
    int8_t res = kl_pop_byte(ks);
    assert(res == 30);
    assert(ks->top == ks->ci->top);
    // kl_free_state(ks);
}

int main(int argc, char *argv[])
{
    kl_init();
    test_vm();
    test_vm2();
    kl_fini();
    return 0;
}

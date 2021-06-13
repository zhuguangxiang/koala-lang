/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "core/core.h"

/*
trait A

trait B: A

trait C: B

trait D: A

class E: A, D, C, B

https://stackoverflow.com/questions/34242536/linearization-order-in-scala
output: CBDA
*/

MethodDef A_methods[] = {
    METHOD_DEF("fooA", "i32", "i32", nil),
    METHOD_DEF("__str__", "i32", "i32", nil),
};

MethodDef B_methods[] = {
    METHOD_DEF("fooB", "i32", "i32", nil),
    METHOD_DEF("__str__", "i32", "i32", nil),
};

MethodDef C_methods[] = {
    METHOD_DEF("fooC", "i32", "i32", nil),
    METHOD_DEF("__str__", "i32", "i32", nil),
};

MethodDef D_methods[] = {
    METHOD_DEF("fooD", "i32", "i32", nil),
    METHOD_DEF("__str__", "i32", "i32", nil),
};

MethodDef E_methods[] = {
    METHOD_DEF("fooE", "i32", "i32", nil),
    METHOD_DEF("__str__", "i32", "i32", nil),
};

void test_mixin_order(void)
{
    TypeInfo *A_type = type_new("A", TF_TRAIT);
    type_add_methods(A_type, A_methods, COUNT_OF(A_methods));
    type_ready(A_type);

    TypeInfo *B_type = type_new("B", TF_TRAIT);
    type_set_base(B_type, A_type);
    type_add_methods(B_type, B_methods, COUNT_OF(B_methods));
    type_ready(B_type);

    TypeInfo *C_type = type_new("C", TF_TRAIT);
    type_set_base(C_type, B_type);
    type_add_methods(C_type, C_methods, COUNT_OF(C_methods));
    type_ready(C_type);

    TypeInfo *D_type = type_new("D", TF_TRAIT);
    type_set_base(D_type, A_type);
    type_add_methods(D_type, D_methods, COUNT_OF(D_methods));
    type_ready(D_type);

    TypeInfo *E_type = type_new("E", TF_CLASS);
    type_set_base(E_type, A_type);
    type_add_trait(E_type, D_type);
    type_add_trait(E_type, C_type);
    type_add_trait(E_type, B_type);
    type_add_methods(E_type, E_methods, COUNT_OF(E_methods));
    type_ready(E_type);

    type_show(A_type);
    type_show(B_type);
    type_show(C_type);
    type_show(D_type);
    type_show(E_type);
}

int main(int argc, char *argv[])
{
    init_core_pkg();
    test_mixin_order();
    return 0;
}

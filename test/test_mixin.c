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

void test_mixin_order(void)
{
    TypeInfo *A_type = type_new("A", TF_TRAIT);
    type_ready(A_type);

    TypeInfo *B_type = type_new("B", TF_TRAIT);
    type_set_base(B_type, A_type);
    type_ready(B_type);

    TypeInfo *C_type = type_new("C", TF_TRAIT);
    type_set_base(C_type, B_type);
    type_ready(C_type);

    TypeInfo *D_type = type_new("D", TF_TRAIT);
    type_set_base(D_type, A_type);
    type_ready(D_type);

    TypeInfo *E_type = type_new("E", TF_CLASS);
    type_set_base(E_type, A_type);
    type_add_trait(E_type, D_type);
    type_add_trait(E_type, C_type);
    type_add_trait(E_type, B_type);
    type_ready(E_type);

    type_show(A_type);
    type_show(B_type);
    type_show(C_type);
    type_show(D_type);
    type_show(E_type);
}

int main(int argc, char *argv[])
{
    test_mixin_order();
    return 0;
}

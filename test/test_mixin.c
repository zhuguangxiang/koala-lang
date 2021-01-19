/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangXiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "object.h"

/*
trait A

trait B: A

trait C: B

trait D: A

class E: A, D, C, B

https://stackoverflow.com/questions/34242536/linearization-order-in-scala
output: CBDA
*/

TypeObject *A_type;
TypeObject *B_type;
TypeObject *C_type;
TypeObject *D_type;
TypeObject *E_type;

void test_mixin_order(void)
{
    A_type = type_new_pub_trait("A");
    B_type = type_new_pub_trait("B");
    C_type = type_new_pub_trait("C");
    D_type = type_new_pub_trait("D");
    E_type = type_new_class("E");

    type_append_base(B_type, A_type);
    type_append_base(C_type, B_type);
    type_append_base(D_type, A_type);

    type_append_base(E_type, A_type);
    type_append_base(E_type, D_type);
    type_append_base(E_type, C_type);
    type_append_base(E_type, B_type);

    type_ready(A_type);
    type_ready(B_type);
    type_ready(C_type);
    type_ready(D_type);
    type_ready(E_type);

    type_show(A_type);
    type_show(B_type);
    type_show(C_type);
    type_show(D_type);
    type_show(E_type);
}

int main(int argc, char *argv[])
{
    init_core_types();
    test_mixin_order();
    return 0;
}

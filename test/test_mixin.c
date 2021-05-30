/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 */

#include "vm/object.h"

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
    TypeObjectRef A_type = kl_type_new_simple("std", "A", TF_TRAIT | TF_PUB);
    kl_type_ready(A_type);

    TypeObjectRef B_type =
        kl_type_new("std", "B", TF_PUB | TF_TRAIT, NULL, A_type, NULL);
    kl_type_ready(B_type);

    TypeObjectRef C_type =
        kl_type_new("std", "C", TF_PUB | TF_TRAIT, NULL, B_type, NULL);
    kl_type_ready(C_type);

    TypeObjectRef D_type =
        kl_type_new("std", "D", TF_PUB | TF_TRAIT, NULL, A_type, NULL);
    kl_type_ready(D_type);

    VectorRef traits = vector_create(PTR_SIZE);
    vector_push_back(traits, &D_type);
    vector_push_back(traits, &C_type);
    vector_push_back(traits, &B_type);

    TypeObjectRef E_type =
        kl_type_new("std", "E", TF_PUB | TF_CLASS, NULL, A_type, traits);
    kl_type_ready(E_type);

    kl_type_show(A_type);
    kl_type_show(B_type);
    kl_type_show(C_type);
    kl_type_show(D_type);
    kl_type_show(E_type);
}

int main(int argc, char *argv[])
{
    kl_init_types();
    test_mixin_order();
    kl_fini_types();
    return 0;
}

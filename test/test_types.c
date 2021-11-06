/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include <assert.h>
#include "koala.h"
#include "util/mm.h"
#include "vm.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
trait T1 {
    func eat(s string);
}

trait T2 {
    func fly(s string) {

    }
}

trait TA : T1, T2 {
    func eat_fly();
}

class B1 {

}

class B : B1, T2 {
    func fly(s string) {

    }
}

class C : B, TA {

}

*/

int kl_T2_fly(KlState *ks)
{
    printf("T2 fly called()\n");
    return 0;
}

int kl_B_fly(KlState *ks)
{
    printf("B fly called()\n");
    return 0;
}

int kl_B_hash(KlState *ks)
{
    printf("B hash called()\n");
    return 0;
}

int main(int argc, char *argv[])
{
    kl_init();

    TypeInfo T1 = {
        .name = "T1",
        .flags = TP_TRAIT,
    };

    TypeInfo T2 = {
        .name = "T2",
        .flags = TP_TRAIT,
    };

    TypeInfo TA = {
        .name = "TA",
        .flags = TP_TRAIT,
    };

    TypeInfo B1 = {
        .name = "B1",
        .flags = TP_CLASS,
    };

    TypeInfo B = {
        .name = "B",
        .flags = TP_CLASS,
    };

    TypeInfo C = {
        .name = "C",
        .flags = TP_CLASS,
    };

    TypeDesc *desc = str_to_proto("s", null);
    type_add_cfunc(&T1, "eat", desc, null);
    type_add_cfunc(&T2, "fly", desc, kl_T2_fly);
    type_add_cfunc(&TA, "eat_fly", null, null);

    TypeDesc *desc2 = str_to_proto(null, "i32");
    type_add_cfunc(&B, "__hash__", desc2, kl_B_hash);
    type_add_cfunc(&B, "fly", desc, kl_B_fly);

    type_ready(&T1);
    type_ready(&T2);

    type_show(&T1);
    type_show(&T2);

    type_set_base(&TA, &T1);
    type_add_trait(&TA, &T2);

    type_ready(&TA);
    type_show(&TA);

    type_set_base(&B, &B1);
    type_add_trait(&B, &T2);

    type_set_base(&C, &B);
    type_add_trait(&C, &TA);

    type_ready(&B1);
    type_ready(&B);
    type_ready(&C);

    type_show(&B1);
    type_show(&B);
    type_show(&C);

    KlState ks;

    KlFunc *fn;
    KlCFunc cfunc;

    // B.fly
    fn = B.vtbl[0]->func[4];
    cfunc = (KlCFunc)fn->ptr;
    cfunc(&ks);

    // T2.fly
    fn = B.vtbl[1]->func[4];
    cfunc = (KlCFunc)fn->ptr;
    cfunc(&ks);

    // T2.__hash__
    fn = B.vtbl[1]->func[0];
    cfunc = (KlCFunc)fn->ptr;
    cfunc(&ks);

    kl_fini();

    return 0;
}

#ifdef __cplusplus
}
#endif

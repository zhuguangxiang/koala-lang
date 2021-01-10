/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangxiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "klc.h"

void test_klc(void)
{
    klc_file_t *klc = klc_create();
    klc_add_var(klc, "foo", &kl_type_int);
    klc_add_var(klc, "foo", &kl_type_bool);
    klc_show(klc);
    klc_destroy(klc);
}

int main(int argc, char *argv[])
{
    test_klc();
    return 0;
}

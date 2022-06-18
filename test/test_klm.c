/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2018-2022 James <zhuguangxiang@gmail.com>
 */

#include "koalac/klm/klm.h"

int main(int argc, char *argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    KLMPackage pkg;
    klm_init_pkg(&pkg, (char *)"");
    KLMFunc *func = klm_add_func(&pkg, (char *)"add", NULL);
    KLMCodeBlock *block = klm_append_block(func, (char *)"");
    klm_fini_pkg(&pkg);
    return 0;
}

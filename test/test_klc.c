/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "codeobject.h"
#include "klc.h"
#include "log.h"
#include "moduleobject.h"
#include "opcode.h"
#include "run.h"

#ifdef __cplusplus
extern "C" {
#endif

int main(int argc, char *argv[])
{
    init_log(LOG_INFO, NULL, 0);
    kl_init(argc, argv);

    KlcFile klc;
    init_klc_file(&klc, "test_klc.klc");
    KlcVar *var = klc_add_var(&klc, "hello", "s", 0, 0);
    uint16_t index = klc_add_int(&klc, 100, 1, 2);
    var->const_index = index;
    uint16_t index2 = klc_add_int(&klc, 100, 1, 2);
    ASSERT(index == index2);

    index = klc_add_float(&klc, 100.123);
    index2 = klc_add_float(&klc, 100.123);
    ASSERT(index == index2);

    KlcFunc *fn = klc_add_func(&klc, "print", NULL, 0);
    klc_func_add_arg(fn, "objs", "Object", 0);
    uint16_t sep_index = klc_add_str(&klc, " ", 1);
    klc_func_add_arg(fn, "sep", "s", sep_index);
    klc_func_add_ann(fn, "native", "builtin_print", NULL);

    write_klc_file(&klc);
    fini_klc_file(&klc);

    kl_fini();
    return 0;
}

#ifdef __cplusplus
}
#endif

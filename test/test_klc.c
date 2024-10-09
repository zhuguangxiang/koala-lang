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

    printf("write to klc file\n");

    KlcFile klc = { 0 };
    init_klc_file(&klc, "test_klc.klc");
    klc.num_vars = 1;
    klc.num_funcs = 1;
    klc_add_var(&klc, "hello", "s");
    klc_add_func(&klc, "foo", "s:i");
    write_klc_file(&klc);

    KlcFile klc2 = { 0 };
    init_klc_file(&klc2, "test_klc.klc");
    read_klc_file(&klc2, 0);
    klc_dump(&klc2);
    kl_fini();
    return 0;
}

#ifdef __cplusplus
}
#endif

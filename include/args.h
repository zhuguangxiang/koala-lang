/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_ARGS_H_
#define _KOALA_ARGS_H_

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_PATH_LEN 1024

typedef struct KoalaOptions {
    int compile_only;            // -c
    int enable_int_trap;         // --int-trap
    int enable_float_trap;       // --float-trap
    char *dump;                  // --dump=<list>
    const char *output;          // -o <file>
    const char *input;           // input file
    char pkg_name[MAX_PATH_LEN]; // --package-name=<name>
} KoalaOptions;

int kl_parse_args(int argc, char *argv[], KoalaOptions *opt);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_ARGS_H_ */

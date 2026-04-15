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

typedef struct KoalaOptions {
    int compile_only;   // -c
    char *dump;         // --dump=<stage>
    const char *output; // -o <file>
    const char *input;  // input file
} KoalaOptions;

int kl_parse_args(int argc, char *argv[], KoalaOptions *opt);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_ARGS_H_ */

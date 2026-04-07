/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_CODE_SPEC_H_
#define _KOALA_CODE_SPEC_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _CodeSpec {
    /* name */
    char *name;
    /* file name */
    char *filename;
    /* line table(addr2line) */
    void *line_table;
    /* all locals, include parameters */
    int nlocals;
    /* max call arguments */
    int max_call_args;
    /* size of codes */
    int code_size;
    /* start_pc of this code */
    int start_pc;
} CodeSpec;

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_CODE_SPEC_H_ */

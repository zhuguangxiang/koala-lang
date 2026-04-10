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
    /* all locals, include parameters */
    int nlocals;
    /* start_pc of this code */
    int start_pc;
    /* number of insns */
    int num_insns;
    /* max call arguments */
    int max_call_args;
    /* name */
    char *name;
    /* file name */
    char *filename;
    /* line table(addr2line) */
    void *line_table;
} CodeSpec;

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_CODE_SPEC_H_ */

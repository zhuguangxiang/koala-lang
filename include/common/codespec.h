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
    /* number of positional parameters(must be passed), not include KW parameters */
    int nargs;
    /* all locals, include parameters(pos-args&kw-args) */
    int nlocals;
    /* max number of call arguments, the callframe size is nlocals + max_nargs */
    int max_nargs;
    /* size of instructions */
    int insns_size;
    /* instructions */
    char *insns;
} CodeSpec;

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_CODE_SPEC_H_ */

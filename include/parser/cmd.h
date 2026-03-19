/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_CMD_H_
#define _KOALA_CMD_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _DumpFlags {
    DUMP_NONE = 0,
    DUMP_IR = 1 << 0,
    DUMP_OPT_IR = 1 << 1,
    DUMP_LIR = 1 << 2,
    DUMP_VREG = 1 << 3,
    DUMP_CGEN = 1 << 4,
    DUMP_ALL = 0xFFFFFFFF,
} DumpFlags;

typedef struct _CompileOptions {
    int enable_opt;
    int enable_isel;
    int enable_cgen;
    int regalloc;
    DumpFlags dump;
} CompileOptions;

#define opt_dump_has(opt, flag) (((opt).dump & (flag)) != 0)

extern CompileOptions opt;

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_CMD_H_ */

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
    int enable_genir;
    int enable_opt;
    int enable_isel;
    int enable_lsra;
    int enable_cgen;
    int enable_fusion;
    int enable_tail_call;
    int build_stdlib;
    int enable_write_klc;
    DumpFlags dump;
} CompileOptions;

extern CompileOptions cmd_opt;

#define genir_enabled()     (cmd_opt.enable_genir)
#define opt_enabled()       (cmd_opt.enable_opt)
#define isel_enabled()      (cmd_opt.enable_isel)
#define fusion_enabled()    (cmd_opt.enable_fusion)
#define lsra_enabled()      (cmd_opt.enable_lsra)
#define cgen_enabled()      (cmd_opt.enable_cgen)
#define tail_call_enabled() (cmd_opt.enable_tail_call)
#define is_build_stdlib()   (cmd_opt.build_stdlib)
#define write_klc_enabled() (cmd_opt.enable_write_klc)

#define dump_ir_enabled()     ((cmd_opt.dump & DUMP_IR) != 0)
#define dump_opt_ir_enabled() ((cmd_opt.dump & DUMP_OPT_IR) != 0)
#define dump_lir_enabled()    ((cmd_opt.dump & DUMP_LIR) != 0)
#define dump_vreg_enabled()   ((cmd_opt.dump & DUMP_VREG) != 0)
#define dump_cgen_enabled()   ((cmd_opt.dump & DUMP_CGEN) != 0)

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_CMD_H_ */

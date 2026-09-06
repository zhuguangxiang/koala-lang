/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_CMD_H_
#define _KOALA_CMD_H_

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_PATH_LEN 1024

typedef enum _DumpFlags {
    DUMP_NONE = 0,
    DUMP_NO_OPT_IR = 1 << 0,
    DUMP_SSA_IR = 1 << 1,
    DUMP_IR = 1 << 2,
    DUMP_LIR = 1 << 3,
    DUMP_VREG = 1 << 4,
    DUMP_CODE = 1 << 5,
    DUMP_ITABLE = 1 << 6,
    DUMP_ALL = 0xFFFFFFFF,
} DumpFlags;

typedef struct _CompileOptions {
    int enable_genir;
    int enable_ssa;
    int enable_opt;
    int enable_isel;
    int enable_lsra;
    int enable_cgen;
    int enable_fusion;
    int enable_tail_call;
    int build_stdlib;
    int enable_write_klc;
    int enable_int_trap;
    int enable_float_trap;
    int strip_lineinfo;
    DumpFlags dump;
    char pkg_path[MAX_PATH_LEN];
} CompileOptions;

extern CompileOptions cmd_opt;

#define genir_enabled()     (cmd_opt.enable_genir)
#define ssa_enabled()       (cmd_opt.enable_ssa)
#define opt_enabled()       (cmd_opt.enable_opt)
#define isel_enabled()      (cmd_opt.enable_isel)
#define fusion_enabled()    (cmd_opt.enable_fusion)
#define lsra_enabled()      (cmd_opt.enable_lsra)
#define cgen_enabled()      (cmd_opt.enable_cgen)
#define tail_call_enabled() (cmd_opt.enable_tail_call)
#define is_build_stdlib()   (cmd_opt.build_stdlib)
#define write_klc_enabled() (cmd_opt.enable_write_klc)
#define int_cast_mode()     (cmd_opt.enable_int_trap ? 0 : 1)
#define float_cast_mode()   (cmd_opt.enable_float_trap ? 0 : 1)
#define strip_lineinfo()    (cmd_opt.strip_lineinfo)

#define dump_no_opt_ir_enabled() ((cmd_opt.dump & DUMP_NO_OPT_IR) != 0)
#define dump_ssa_enabled()       ((cmd_opt.dump & DUMP_SSA_IR) != 0)
#define dump_ir_enabled()        ((cmd_opt.dump & DUMP_IR) != 0)
#define dump_lir_enabled()       ((cmd_opt.dump & DUMP_LIR) != 0)
#define dump_vreg_enabled()      ((cmd_opt.dump & DUMP_VREG) != 0)
#define dump_code_enabled()      ((cmd_opt.dump & DUMP_CODE) != 0)
#define dump_itable_enabled()    ((cmd_opt.dump & DUMP_ITABLE) != 0)

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_CMD_H_ */

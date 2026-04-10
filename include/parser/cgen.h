/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_CGEN_H_
#define _KOALA_CGEN_H_

#include "ir.h"
#include "pass.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Module-level machine code generation context.
 * Holds the global .text buffer, function list, fixups, and
 * all state required to linearize and encode the entire module.
 */
typedef struct _KlMachModule {
    /* Original high-level IR module. */
    KlrModule *origin;

    /* All lowered machine functions in final layout order. */
    Vector funcs;

    /* Internal fixups for intra-module references (rel32 patches). */
    Vector fixups;

    /* remove duplicated import_entry */
    HashMap import_map;

    /* Imported functions referenced by this module. */
    Vector import_table;

    /* remove duplicated const */
    HashMap cp_map;

    /* Constant pool shared by the entire module. */
    Vector const_pool;

    /* Module-level code buffer (.text). All functions are laid out here. */
    CodeBuffer codes;

    /* Module-level PC allocator used during linearization. */
    int pc;
} KlMachModule;

/*
 * Represents a function's region inside the module-level .text buffer.
 * Each function occupies a contiguous PC interval assigned during
 * linearization.
 */
typedef struct _KlMachFunc {
    /* Owning module context. */
    KlMachModule *m;

    /* Basic blocks in final linearized order. */
    List bb_list;

    /* entry machine block */
    struct _KlMachBlock *entry;

    /* func index */
    int index;

    /* Module-level start PC of this function inside the .text buffer. */
    int start_pc;

    /* Total number of machine instructions in this function. */
    int total_insns;

    /**
     * Set to true if process_fused_jumps() modifies the MachInsn
     * structure (e.g., fallback expansion, long-branch insertion, fused-jmp
     * rewriting). When true, the final layout must be recomputed before patching.
     */
    int changed;

    /* branches(jmp_cond) needed to be lowered */
    Vector branches;

    /* Original IR-level function. */
    KlrFunc *origin;
} KlMachFunc;

/*
 * Machine-level basic block.
 * Each block corresponds to a contiguous PC range inside its parent function.
 */
typedef struct _KlMachBlock {
    /* Parent machine function. */
    KlMachFunc *fn;

    /* Link node for func->bb_list. */
    List link;

    /* Module-level PC range [start_pc, end_pc). */
    int start_pc;
    int end_pc;

    /* Linearized machine instructions in this block. */
    Vector insns;

    /* Original IR basic block. */
    KlrBasicBlock *origin;
} KlMachBlock;

typedef struct _KlMachOper {
    enum {
        MACH_OPER_NONE,

        /* register operands */
        MACH_OPER_R,  /* 8-bit register */
        MACH_OPER_RX, /* 12-bit register */

        /* immediate operands */
        MACH_OPER_IMM,  /* 8-bit immediate */
        MACH_OPER_IMM2, /* 16-bit immediate */

        /* offset operands */
        MACH_OPER_OFF2, /* 16-bit signed offset */

        /* index operands */
        MACH_OPER_IDX2 /* 16-bit index */
    } kind;

    int32_t value; /* bit-exact integer payload */
} KlMachOper;

/*
 * Canonical lowered machine instruction (LIR).
 * Used by instruction selection, LSRA, fixup patching, and final encoding.
 * Each instruction has a module-level absolute PC.
 */
typedef struct _KlMachInsn {
    /* Parent basic block. */
    KlMachBlock *bb;

    /* Lowered opcode after instruction selection. */
    OpCode op;

    /* Fixed 4-byte encoding format used by the final bytecode emitter. */
    OpFormat format;

    /* Physical operands. */
    int opers[3];

    /* Target function for direct (intra-module) calls. */
    KlMachFunc *target_fn;

    /* Branch target basic block (machine-level). */
    KlMachBlock *target;

    /* import table index */
    int import_index;

    /* fixup_flag */
    int fixup_flag;
#define KL_MACH_FIXUP_REL32  1
#define KL_MACH_FIXUP_IMPORT 2
#define KL_MACH_FIXUP_FUNCID 3

    /* Linearized instruction index (module-level absolute PC). */
    int pc;

    /* Original IR instruction for debugging. */
    KlrInsn *origin;
} KlMachInsn;

#define KL_MACH_CONST_I64 0
#define KL_MACH_CONST_U64 1
#define KL_MACH_CONST_F64 2
#define KL_MACH_CONST_STR 3

typedef struct {
    HashMapEntry hnode;
    uint8_t tag;
    int index;
    union {
        int64_t i64;
        uint64_t u64;
        double f64;
        char *str;
    };
} KlMachConst;

typedef enum {
    IMPORT_FUNC,   /* free function */
    IMPORT_GLOBAL, /* global variable */
    IMPORT_TYPE,   /* type object */
    IMPORT_METHOD, /* method of a type (slot-based) */
    IMPORT_FIELD   /* field of a type (offset-based) */
} ImportKind;

typedef struct KlMachImport {
    HashMapEntry hnode;
    int kind;
    int index;
    char *path;
    char *name;
} KlMachImport;

// clang-format off
#define mach_insn_is(mi, a) ((mi)->op == (a))
#define mach_insn_or(mi, a, b) (mach_insn_is(mi, a) || mach_insn_is(mi, b))
// clang-format on

#define NEXT_BLOCK(mb) list_next(mb, link, &(mb)->fn->bb_list)
void kl_lower_operands(KlrFunc *fn, KlMachModule *ctx);
void kl_do_codegen(KlrModule *module);

int kl_mach_const_add_int(KlMachModule *ctx, int64_t v);
int kl_mach_const_add_uint(KlMachModule *ctx, uint64_t v);
int kl_mach_const_add_float(KlMachModule *ctx, double v);
int kl_mach_const_add_str(KlMachModule *ctx, char *s);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_CGEN_H_ */

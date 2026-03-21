/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "jit/jit.h"
#include "opcode.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _jit_ctx_t {
    CodeObject *code;
    gcc_jit_context *ctx;
    gcc_jit_function *func;
    gcc_jit_param *params[16];
    gcc_jit_lvalue *regs[16];
} jit_ctx_t;

typedef struct _jit_block_t {
    int has_terminal;
    gcc_jit_block *block;
} jit_block_t;

#define I_OP(i) ((i) >> 24)

#define I_A(i) (((i) >> 16) & 0xFF)
#define I_B(i) (((i) >> 8) & 0xFF)
#define I_C(i) ((i) & 0xFF)

#define I_Ax(i) (((i) >> 12) & 0x0FFF)
#define I_Bx(i) ((i) & 0x0FFF)

#define I_Bxx(i) ((i) & 0xFFFF)

static gcc_jit_lvalue *get_reg(jit_ctx_t *jit, int idx)
{
    if (idx < 0 || idx >= 16) return NULL;
    if (!jit->regs[idx]) {
        char name[16];
        sprintf(name, "r%d", idx);
        // Create the local symbol only when first referenced
        jit->regs[idx] =
            gcc_jit_function_new_local(jit->func, NULL, jit->int64_type, name);
    }
    return jit->regs[idx];
}

void translate(jit_block_t *b, uint32_t insn, int pc, gcc_jit_context *ctx)
{
    OpCode op = I_OP(insn);
    int rd, rs, rt, imm, idx, off;

    switch (op) {
        case OP_INT_SUB_IMM: {
            // r_ra = r_rb - imm
            gcc_jit_block_add_assignment_op(
                kb->block, NULL, get_reg(jit, ra), GCC_JIT_BINARY_OP_MINUS,
                jit->int64_type, gcc_jit_lvalue_as_rvalue(get_reg(jit, rb)),
                gcc_jit_context_new_rvalue_from_int(ctx, jit->int64_type, imm));
            break;
        }

        case OP_JMP_INT_CMP_GE_IMM: {
            // if (r_ra >= rb_imm) jmp pc + 1 + rc_off
            gcc_jit_rvalue *cond = gcc_jit_context_new_comparison(
                ctx, NULL, GCC_JIT_COMPARISON_GE,
                gcc_jit_lvalue_as_rvalue(get_reg(jit, ra)),
                gcc_jit_context_new_rvalue_from_int(ctx, jit->int64_type, rb));

            gcc_jit_block *target = jit->kb_map[pc + 1 + rc].jit_block;
            gcc_jit_block *fallthrough = jit->kb_map[pc + 1].jit_block;

            gcc_jit_block_end_with_conditional(kb->block, NULL, cond, target,
                                               fallthrough);
            kb->terminated = true;
            break;
        }

        case OP_PUSH: {
            // In a simple Fib JIT, we assume only 1 arg for recursion
            // We use a temp rvalue to hold the argument for the next CALL
            jit->temp_arg = gcc_jit_lvalue_as_rvalue(get_reg(jit, ra));
            break;
        }

        case OP_CALL: {
            // Native Recursive Call: r_ra = func(jit->temp_arg)
            gcc_jit_rvalue *res = gcc_jit_context_new_call(ctx, NULL, jit->func,
                                                           1, &jit->temp_arg);
            gcc_jit_block_add_assignment(kb->block, NULL, get_reg(jit, ra),
                                         res);
            break;
        }

        case OP_INT_ADD: {
            // r_ra = r_rb + r_rc
            gcc_jit_block_add_assignment_op(
                kb->block, NULL, get_reg(jit, ra), GCC_JIT_BINARY_OP_PLUS,
                jit->int64_type, gcc_jit_lvalue_as_rvalue(get_reg(jit, rb)),
                gcc_jit_lvalue_as_rvalue(get_reg(jit, rc)));
            break;
        }

        case OP_RET: {
            gcc_jit_block_end_with_return(
                kb->block, NULL, gcc_jit_lvalue_as_rvalue(get_reg(jit, ra)));
            kb->terminated = true;
            break;
        }
    }
}

void *koala_jit_compile(uint32_t *insns, int count, bool *leaders)
{
    jit_ctx_t jit = { 0 };
    jit.ctx = gcc_jit_context_acquire();
    jit.int64_type = gcc_jit_context_get_type(jit.ctx, GCC_JIT_TYPE_INT64_T);

    // Define function: int64_t fib(int64_t n)
    gcc_jit_param *param_n =
        gcc_jit_context_new_param(jit.ctx, NULL, jit.int64_type, "n");
    jit.func =
        gcc_jit_context_new_function(jit.ctx, NULL, GCC_JIT_FUNCTION_EXPORTED,
                                     jit.int64_type, "jit_fib", 1, &param_n, 0);

    // 1. Create Blocks
    jit.kb_map = calloc(count, sizeof(kl_block_t));
    for (int i = 0; i < count; i++) {
        if (leaders[i] || i == 0)
            jit.kb_map[i].jit_block =
                gcc_jit_function_new_block(jit.func, NULL);
    }

    // 2. Entry Sync: Connect Param 'n' to Virtual Register 'r0'
    // This is the ONLY manual move from the C-world to the JIT-world
    gcc_jit_block_add_assignment(jit.kb_map[0].jit_block, NULL,
                                 get_reg(&jit, 0),
                                 gcc_jit_param_as_rvalue(param_n));

    // 3. Translation Loop
    kl_block_t *current_kb = &jit.kb_map[0];
    for (int i = 0; i < count; i++) {
        if (i > 0 && leaders[i]) {
            if (!current_kb->terminated) {
                gcc_jit_block_end_with_jump(current_kb->jit_block, NULL,
                                            jit.kb_map[i].jit_block);
            }
            current_kb = &jit.kb_map[i];
        }
        translate_insn(&jit, current_kb, insns[i], i);
    }

    // 4. Compilation
    gcc_jit_result *res = gcc_jit_context_compile(jit.ctx);
    void *fn_ptr = gcc_jit_result_get_code(res, "jit_fib");

    // Clean up context, but keep result for the executable code
    return fn_ptr;
}

void scan_leaders(CodeObject *code)
{
    gcc_jit_context_set_int_option(ctx, GCC_JIT_INT_OPTION_OPTIMIZATION_LEVEL,
                                   3);

    gcc_jit_context *ctx = gcc_jit_context_acquire();
    gcc_jit_type *int_type = gcc_jit_context_get_type(ctx, GCC_JIT_TYPE_LONG);

    gcc_jit_param *param_name =
        gcc_jit_context_new_param(ctx, NULL, int_type, "v");

    gcc_jit_function *func =
        gcc_jit_context_new_function(ctx, NULL, GCC_JIT_FUNCTION_EXPORTED,
                                     int_type, "fib", 1, &param_name, 0);

    int leaders[code->cs.insns_size];
    memset(leaders, 0, sizeof(leaders));

    jit_block_t blocks[code->cs.insns_size];
    memset(blocks, 0, sizeof(blocks));

    uint32_t *insns = (uint32_t *)code->cs.insns;
    uint32_t insn_count = code->cs.insns_size;

    leaders[0] = 1;

    uint32_t insn;
    OpCode op;
    int rd, rs, rt, imm, idx, off, target;

    gcc_jit_rvalue *regs[code->cs.nlocals];

    for (int i = 0; i < insn_count; i++) {
        insn = insns[i];
        op = I_OP(insn);

        switch (op) {
            case OP_JMP_INT_CMP_GE_IMM: {
                rd = I_A(insn);
                imm = I_B(insn);
                off = I_C(insn);

                target = i + 1 + off;

                if (target >= 0 && target < insn_count) {
                    leaders[target] = 1;
                }

                if (i + 1 < insn_count) {
                    leaders[i + 1] = 1;
                }
                break;
            }

            case OP_RET_VOID:
            case OP_RET: {
                if (i + 1 < insn_count) {
                    leaders[i + 1] = 1;
                }
                break;
            }

                // call is a gc safepoint, treat as a leader
                // case OP_CALL: {
                //     if (i + 1 < insn_count) {
                //         leaders[i + 1] = 1;
                //     }
                //     break;
                // }

            default: {
                // skip other instructions
                break;
            }
        }
    }

    for (int i = 0; i < insn_count; i++) {
        if (leaders[i]) {
            char name[32];
            sprintf(name, "block_at_pc_%d", i);
            blocks[i].block = gcc_jit_function_new_block(func, name);
        }
    }

    jit_block_t *current_b = &blocks[0];
    for (int i = 0; i < insn_count; i++) {
        if (i > 0 && leaders[i]) {
            if (!current_b->has_terminal) {
                gcc_jit_block_end_with_jump(current_b->block, NULL,
                                            blocks[i].block);
            }
            current_b = &blocks[i];
        }

        translate(current_b, insns[i], i);
    }
}

#ifdef __cplusplus
}
#endif

/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * Baseline x86-64 JIT (v1). See DESIGN.md for the full design.
 *
 * Strategy: Sparkplug-style baseline. Each whitelisted bytecode instruction is
 * lowered to a fixed sequence of machine instructions that mirrors the
 * interpreter's semantics on the TValue register file, minus dispatch, decode
 * and type checks. Registers stay in the caller's frame (cf->locals); RBX holds
 * the register-file base, so register i lives at [RBX + i*16] (tag) and
 * [RBX + i*16 + 8] (payload). No register allocation is performed.
 */

#include "jit/jit.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <sys/mman.h>
#include <unistd.h>
#include "emit.h"
#include "modobj.h"
#include "opcode_only.h"
#include "printer.h"
#include "vm.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- bytecode decoding (must match runtime/eval.c) ---- */

/* Runtime int<->int cast (do_cast.c): may abort in trap mode, never TAG_ERROR. */
void do_int_cast(TValue *regs, int rd, int rs, int mode, int dst_ti);
void do_float_cast(TValue *regs, int rd, int rs, int mode, int dst_ti);
void do_float_to_int(TValue *regs, int rd, int rs, int mode, int dst_ti);
void do_int_to_float(TValue *regs, int rd, int rs, int mode, int dst_ti);

#define I_OP(i)             ((i) >> 24)
#define I_VAL(i, shr, mask) (((i) >> (shr)) & ((1u << (mask)) - 1))

/* Sentinel ALU code for multiplication (imul has its own encoding). */
#define ALU_MUL 8

static inline int32_t sval(uint32_t val, int bits)
{
    int shift = 32 - bits;
    return ((int32_t)(val << shift)) >> shift;
}

/* TValue layout: { uintptr_t tag; union { int64_t ival; ... }; } = 16 bytes. */
#define TAG_DISP(r) ((int32_t)((r) * 16))
#define VAL_DISP(r) ((int32_t)((r) * 16 + 8))

/* A pending rel32 jump: patch_off is the buffer offset of the 4-byte disp. */
typedef struct _Fixup {
    size_t patch_off;
    int target_pc;
} Fixup;

/* Sentinel fixup target: the function's error epilogue, whose native offset
 * is only known after the whole body has been emitted. */
#define JIT_ERR_TARGET (-1)

/* Register cache pools (DESIGN §15.4): caller-saved, unused by the cache-aware
 * emitters (they use RAX/RCX/RDX/XMM0/R11 as scratch). Every helper / runtime
 * call clobbers them, which is why the copies are dropped there. */
#define RC_MAX 12
static const int rc_gpr_pool[] = { RSI, RDI, R8, R9, R10 };
static const int rc_xmm_pool[] = { XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7 };
#define RC_NGPR ((int)(sizeof(rc_gpr_pool) / sizeof(rc_gpr_pool[0])))
#define RC_NXMM ((int)(sizeof(rc_xmm_pool) / sizeof(rc_xmm_pool[0])))

typedef struct _RcSlot {
    int vm;   /* VM register */
    int phys; /* GPR or XMM number */
    int xmm;
} RcSlot;

/* Pass-0 record: VM register vm is read/written at pc as int (0), float (1) or
 * either (-1, plain moves). Drives the promotion choice only, not correctness. */
typedef struct _RcVote {
    int pc, vm, cls, def;
} RcVote;

typedef struct _JitCtx {
    EmitBuf buf;
    size_t *pc_map; /* pc -> native offset, size num_insns */
    Fixup *fixups;
    int nfixups;
    int cap_fixups;
    int num_insns;
    int failed;
    int fail_pc;        /* pc of the unsupported instruction (if failed) */
    uint32_t fail_insn; /* the offending instruction word */
    int self_only;      /* direct self calls: helpers must set ks->stack_top */
    int nlocals;
    int cur_pc;         /* bytecode pc being translated */

    /* Write-through register cache (DESIGN §15). Memory stays the truth; a
     * promoted VM register may also have a copy in rc[s].phys, valid when bit s
     * of rc_valid is set. */
    int rc_n;
    RcSlot rc[RC_MAX];
    uint32_t rc_valid;
    int rc_preload;     /* pc 0 is a jump target: load every copy at entry */
    int rc_check;       /* KOALA_JIT_CHECK_CACHE: verify each hit, ud2 on mismatch */
    uint32_t *rc_in;    /* per pc: state at a jump target (from the last pass) */
    uint32_t *rc_meet;  /* per pc: meet of incoming edge states (this pass) */
    uint8_t *rc_pred;   /* per pc: some edge reaches it (this pass) */
    uint8_t *rc_jt;     /* per pc: jump target (pass 0) */
    uint8_t *rc_jt_new; /* per pc: jump target (this pass) */
    int rc_collect;     /* pass 0: record operand votes and back edges */
    RcVote *votes;
    int nvotes, cap_votes;
    int *backs; /* pairs (target, source) of backward jumps */
    int nbacks, cap_backs;
} JitCtx;

/* KOALA_JIT_TRACE: 0/1 switch, default off (unset or "0" disables tracing). */
static int jit_trace(void)
{
    static int cached = -1;
    if (cached < 0) {
        const char *v = getenv("KOALA_JIT_TRACE");
        cached = (v && v[0] != '0');
    }
    return cached;
}

/* ---- KOALA_JIT_STATS: compile / bail counts, printed to stderr at exit ---- */

#define STATS_MAX_KEYS    64
#define STATS_MAX_SAMPLES 4

typedef struct _StatEntry {
    char key[48];
    int count;
    int nsamples;
    const char *samples[STATS_MAX_SAMPLES];
} StatEntry;

static struct {
    int compiled;
    size_t code_bytes; /* emitted machine code, excluding alignment padding */
    size_t pages;      /* pages the code occupies (committed RX pages) */
    int chunks;        /* arena chunks mapped */
    int nkeys;
    StatEntry keys[STATS_MAX_KEYS];
} jit_stats_data;

static int jit_stats(void)
{
    static int cached = -1;
    if (cached < 0) {
        const char *v = getenv("KOALA_JIT_STATS");
        cached = (v && v[0] != '0');
    }
    return cached;
}

static int stat_cmp(const void *a, const void *b)
{
    return ((const StatEntry *)b)->count - ((const StatEntry *)a)->count;
}

static void jit_stats_print(void)
{
    int bailed = 0;
    for (int i = 0; i < jit_stats_data.nkeys; i++) bailed += jit_stats_data.keys[i].count;
    qsort(jit_stats_data.keys, jit_stats_data.nkeys, sizeof(StatEntry), stat_cmp);
    fprintf(stderr, "[jit stats] compiled %d, not compiled %d\n", jit_stats_data.compiled,
            bailed);
    fprintf(stderr, "[jit stats] code %zu bytes, %zu pages, %d chunks\n",
            jit_stats_data.code_bytes, jit_stats_data.pages, jit_stats_data.chunks);
    for (int i = 0; i < jit_stats_data.nkeys; i++) {
        StatEntry *e = &jit_stats_data.keys[i];
        fprintf(stderr, "  %6d  %-28s", e->count, e->key);
        for (int k = 0; k < e->nsamples; k++)
            fprintf(stderr, "%s%s", k ? ", " : "  e.g. ", e->samples[k]);
        if (e->count > e->nsamples) fprintf(stderr, ", ...");
        fprintf(stderr, "\n");
    }
}

static void jit_stats_init(void)
{
    static int inited = 0;
    if (!inited) {
        inited = 1;
        atexit(jit_stats_print);
    }
}

/* Code arena (DESIGN §16): functions are bump-allocated, 16-byte aligned, into
 * chunks reserved PROT_NONE. Each allocation flips only the pages it touches to
 * RW, copies the code and flips them back to RX, so no page is ever W+X. Code is
 * never freed (process lifetime, like v1's per-function mappings). */
#define JIT_CODE_ALIGN 16

static struct {
    uint8_t *base;
    size_t size;
    size_t used;
    size_t page;
    size_t chunk; /* default chunk size; KOALA_JIT_CODE_CHUNK=<bytes> (tests) */
} jit_arena;

static void *jit_code_alloc(const uint8_t *code, size_t sz)
{
    if (!jit_arena.page) {
        jit_arena.page = (size_t)sysconf(_SC_PAGESIZE);
        jit_arena.chunk = 1u << 20;
        const char *v = getenv("KOALA_JIT_CODE_CHUNK");
        if (v && atol(v) > 0) jit_arena.chunk = (size_t)atol(v);
        jit_arena.chunk = (jit_arena.chunk + jit_arena.page - 1) & ~(jit_arena.page - 1);
    }
    size_t page = jit_arena.page;
    size_t off = (jit_arena.used + JIT_CODE_ALIGN - 1) & ~(size_t)(JIT_CODE_ALIGN - 1);
    if (!jit_arena.base || off + sz > jit_arena.size) {
        size_t csz = (sz + page - 1) & ~(page - 1);
        if (csz < jit_arena.chunk) csz = jit_arena.chunk;
        void *m = mmap(NULL, csz, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
        if (m == MAP_FAILED) return NULL;
        /* The old chunk's unused tail stays PROT_NONE and unbacked. */
        jit_arena.base = m;
        jit_arena.size = csz;
        jit_arena.used = 0;
        off = 0;
        jit_stats_data.chunks++;
    }
    /* Pages [lo, hi) hold the new code; page lo may also hold earlier code, which
     * is not executing now (compilation is synchronous on this thread and
     * finishes before any native frame resumes). The alignment gap is left as
     * is (zero from the fresh mapping, never executed). */
    size_t lo = off & ~(page - 1);
    size_t hi = (off + sz + page - 1) & ~(page - 1);
    size_t fresh = hi - ((jit_arena.used + page - 1) & ~(page - 1));
    if (mprotect(jit_arena.base + lo, hi - lo, PROT_READ | PROT_WRITE) != 0) return NULL;
    memcpy(jit_arena.base + off, code, sz);
    if (mprotect(jit_arena.base + lo, hi - lo, PROT_READ | PROT_EXEC) != 0) {
        /* Earlier code on page lo would no longer be executable. */
        fprintf(stderr, "jit: mprotect(RX) failed\n");
        abort();
    }
    jit_arena.used = off + sz;
    jit_stats_data.code_bytes += sz;
    jit_stats_data.pages += fresh / page;
    return jit_arena.base + off;
}

static void jit_stats_add(const char *key, const char *name)
{
    jit_stats_init();
    StatEntry *e = NULL;
    for (int i = 0; i < jit_stats_data.nkeys; i++) {
        if (!strcmp(jit_stats_data.keys[i].key, key)) {
            e = &jit_stats_data.keys[i];
            break;
        }
    }
    if (!e) {
        if (jit_stats_data.nkeys >= STATS_MAX_KEYS) return;
        e = &jit_stats_data.keys[jit_stats_data.nkeys++];
        snprintf(e->key, sizeof(e->key), "%s", key);
    }
    e->count++;
    if (e->nsamples < STATS_MAX_SAMPLES) e->samples[e->nsamples++] = name;
}

/* A function is not compiled: count it (KOALA_JIT_STATS) and trace it
 * (KOALA_JIT_TRACE, unless quiet). pc < 0: no instruction involved. */
static void jit_bail(const char *name, int pc, uint32_t insn, const char *reason, int quiet)
{
    if (jit_stats()) {
        char key[48];
        if (pc >= 0 && (I_OP(insn) == OP_CALL || I_OP(insn) == OP_TAIL_CALL))
            snprintf(key, sizeof(key), "op %s flg=%d", jit_op_name(insn), I_VAL(insn, 20, 4));
        else if (pc >= 0)
            snprintf(key, sizeof(key), "op %s", jit_op_name(insn));
        else
            snprintf(key, sizeof(key), "%s", reason);
        jit_stats_add(key, name);
    }
    if (!quiet && jit_trace()) jit_print_bail(name, pc, insn, reason, stderr);
}

int kl_jit_enabled(void)
{
    static int cached = -1;
    if (cached < 0) {
        const char *v = getenv("KOALA_JIT");
        cached = !(v && v[0] == '0');
    }
    return cached;
}

uint32_t kl_jit_threshold(void)
{
    static int cached = -1;
    if (cached < 0) {
        const char *v = getenv("KOALA_JIT_THRESHOLD");
        int n = v ? atoi(v) : 1;
        cached = n < 1 ? 1 : n;
    }
    return (uint32_t)cached;
}

/* ---- frame setup ---- */

/*
 * Prologue (System V AMD64):
 *   rdi = regs (TValue*), rsi = out (TValue*), rdx = ks (KoalaState*)
 *   push rbp; mov rbp,rsp; push rbx; sub rsp,32
 *   mov [rbp-16], rsi   ; save out pointer
 *   mov [rbp-24], rdx   ; save ks
 *   mov rbx, rdi        ; register-file base
 * [rbp-40, rbp-24) is a scratch TValue: the result of a void direct self call.
 * Only caller-saved scratch (rax/rcx/rdx) is used in the body, so RBX is the
 * single callee-saved register we must preserve. The body runs with
 * rsp == 8 (mod 16).
 */
#define FRAME_OUT     (-16)
#define FRAME_KS      (-24)
#define FRAME_SCRATCH (-40)

static void emit_prologue(JitCtx *j)
{
    EmitBuf *b = &j->buf;
    emit_push_reg(b, RBP);
    emit_mov_reg_reg(b, RBP, RSP);
    emit_push_reg(b, RBX);
    emit_sub_rsp_imm32(b, 32);
    emit_mov_mem_reg(b, RBP, FRAME_OUT, RSI);
    emit_mov_mem_reg(b, RBP, FRAME_KS, RDX);
    emit_mov_reg_reg(b, RBX, RDI);
}

/* Restore the frame and return. The result TValue must already be in *out. */
static void emit_epilogue(JitCtx *j)
{
    EmitBuf *b = &j->buf;
    emit_add_rsp_imm32(b, 32);
    emit_pop_reg(b, RBX);
    emit_pop_reg(b, RBP);
    emit_ret(b);
}

/* Load the out pointer into RCX. */
static void emit_load_out(JitCtx *j) { emit_mov_reg_mem(&j->buf, RCX, RBP, FRAME_OUT); }

/* ---- write-through register cache (DESIGN §15) ---- */

static int jit_env_flag(const char *name, int dflt)
{
    const char *v = getenv(name);
    if (!v || !*v) return dflt;
    return v[0] != '0';
}

/* KOALA_JIT_REGCACHE=0 disables the cache (plain v1 emission). */
static int jit_regcache(void)
{
    static int cached = -1;
    if (cached < 0) cached = jit_env_flag("KOALA_JIT_REGCACHE", 1);
    return cached;
}

/* KOALA_JIT_CHECK_CACHE=1: before using a cached copy, compare it with memory
 * and trap (ud2) on mismatch. Turns a missed invalidation into a crash at the
 * exact instruction. */
static int jit_check_cache(void)
{
    static int cached = -1;
    if (cached < 0) cached = jit_env_flag("KOALA_JIT_CHECK_CACHE", 0);
    return cached;
}

#define RC_ALL(j) ((uint32_t)((1u << (j)->rc_n) - 1))

static void *grow(void *p, int *cap, int need, size_t elem)
{
    if (need <= *cap) return p;
    *cap = *cap ? *cap * 2 : 32;
    if (*cap < need) *cap = need;
    return realloc(p, (size_t)*cap * elem);
}

static void rc_vote2(JitCtx *j, int vm, int cls, int def)
{
    if (!j->rc_collect) return;
    j->votes = grow(j->votes, &j->cap_votes, j->nvotes + 1, sizeof(RcVote));
    j->votes[j->nvotes++] = (RcVote){ j->cur_pc, vm, cls, def };
}

static void rc_vote(JitCtx *j, int vm, int cls) { rc_vote2(j, vm, cls, 0); }

static int rc_slot(JitCtx *j, int vm)
{
    for (int s = 0; s < j->rc_n; s++)
        if (j->rc[s].vm == vm) return s;
    return -1;
}

/* An edge reaches bytecode pc t with cache state `state`. */
static void rc_edge(JitCtx *j, int t, uint32_t state)
{
    j->rc_meet[t] &= state;
    j->rc_pred[t] = 1;
}

static void rc_check_hit(JitCtx *j, int s)
{
    EmitBuf *b = &j->buf;
    RcSlot *r = &j->rc[s];
    if (r->xmm) {
        emit_movq_gpr_xmm(b, R11, r->phys);
        emit_alu_reg_mem(b, ALU_CMP, R11, RBX, VAL_DISP(r->vm));
    } else {
        emit_alu_reg_mem(b, ALU_CMP, r->phys, RBX, VAL_DISP(r->vm));
    }
    size_t je = emit_pos(b);
    emit_jcc_rel32(b, CC_E, 0);
    emit_ud2(b);
    emit_patch_i32(b, je + 2, (int32_t)(emit_pos(b) - (je + 6)));
}

/* Make slot s's copy valid (load on a miss) and return its register. */
static int rc_get(JitCtx *j, int s)
{
    RcSlot *r = &j->rc[s];
    if (j->rc_valid & (1u << s)) {
        if (j->rc_check) rc_check_hit(j, s);
    } else {
        if (r->xmm)
            emit_movsd_reg_mem(&j->buf, r->phys, RBX, VAL_DISP(r->vm));
        else
            emit_mov_reg_mem(&j->buf, r->phys, RBX, VAL_DISP(r->vm));
        j->rc_valid |= 1u << s;
    }
    return r->phys;
}

/* vm as an int operand: its GPR copy, or -1 to read [rbx+VAL_DISP(vm)]. */
static int rc_gpr(JitCtx *j, int vm)
{
    rc_vote(j, vm, 0);
    int s = rc_slot(j, vm);
    if (s < 0 || j->rc[s].xmm) return -1;
    return rc_get(j, s);
}

/* vm as a float operand: its XMM copy, or -1. */
static int rc_xmm(JitCtx *j, int vm)
{
    rc_vote(j, vm, 1);
    int s = rc_slot(j, vm);
    if (s < 0 || !j->rc[s].xmm) return -1;
    return rc_get(j, s);
}

/* GPR dst = payload bits of vm (an XMM copy is used only when already valid). */
static void rc_load_gpr(JitCtx *j, int dst, int vm, int cls)
{
    EmitBuf *b = &j->buf;
    rc_vote(j, vm, cls);
    int s = rc_slot(j, vm);
    if (s >= 0 && (!j->rc[s].xmm || (j->rc_valid & (1u << s)))) {
        int p = rc_get(j, s);
        if (j->rc[s].xmm)
            emit_movq_gpr_xmm(b, dst, p);
        else if (p != dst)
            emit_mov_reg_reg(b, dst, p);
    } else {
        emit_mov_reg_mem(b, dst, RBX, VAL_DISP(vm));
    }
}

/* XMM dst = payload bits of vm. */
static void rc_load_xmm(JitCtx *j, int dst, int vm)
{
    EmitBuf *b = &j->buf;
    rc_vote(j, vm, 1);
    int s = rc_slot(j, vm);
    if (s >= 0 && (j->rc[s].xmm || (j->rc_valid & (1u << s)))) {
        int p = rc_get(j, s);
        if (!j->rc[s].xmm)
            emit_movq_xmm_gpr(b, dst, p);
        else if (p != dst)
            emit_movaps_reg_reg(b, dst, p);
    } else {
        emit_movsd_reg_mem(b, dst, RBX, VAL_DISP(vm));
    }
}

/* dst = dst OP vm (ALU_* or ALU_MUL). */
static void rc_alu(JitCtx *j, int op, int dst, int vm)
{
    EmitBuf *b = &j->buf;
    int p = rc_gpr(j, vm);
    if (op == ALU_MUL) {
        if (p >= 0) emit_imul_reg_reg(b, dst, p);
        else emit_imul_reg_mem(b, dst, RBX, VAL_DISP(vm));
    } else {
        if (p >= 0) emit_alu_reg_reg(b, op, p, dst);
        else emit_alu_reg_mem(b, op, dst, RBX, VAL_DISP(vm));
    }
}

/* xmm dst = dst OP vm (SSE_*). */
static void rc_sse(JitCtx *j, int op, int dst, int vm)
{
    int p = rc_xmm(j, vm);
    if (p >= 0) emit_sse_reg_reg(&j->buf, op, dst, p);
    else emit_sse_reg_mem(&j->buf, op, dst, RBX, VAL_DISP(vm));
}

/* Where to compute a result for rd: rd's own machine register when it is
 * promoted in the class (xmm) and is not the other operand `avoid` (which must
 * still be readable after the first operand is loaded into it); else the
 * scratch register. Computing in place keeps the dependency chain free of
 * scratch round trips (DESIGN §15.6). */
static int rc_dst(JitCtx *j, int rd, int avoid, int xmm)
{
    int s = rc_slot(j, rd);
    if (s < 0 || j->rc[s].xmm != xmm || rd == avoid) return xmm ? XMM0 : RAX;
    return j->rc[s].phys;
}

/* vm's payload in memory was just written from src (an XMM when src_xmm):
 * bring its copy up to date. */
static void rc_def(JitCtx *j, int vm, int src, int src_xmm, int cls)
{
    EmitBuf *b = &j->buf;
    rc_vote2(j, vm, cls, 1);
    int s = rc_slot(j, vm);
    if (s < 0) return;
    RcSlot *r = &j->rc[s];
    if (r->xmm == src_xmm) {
        if (r->phys != src) {
            if (r->xmm) emit_movaps_reg_reg(b, r->phys, src);
            else emit_mov_reg_reg(b, r->phys, src);
        }
    } else if (r->xmm) {
        emit_movq_xmm_gpr(b, r->phys, src);
    } else {
        emit_movq_gpr_xmm(b, r->phys, src);
    }
    j->rc_valid |= 1u << s;
}

/* vm's payload in memory was just set to the constant v. */
static void rc_def_imm(JitCtx *j, int vm, int64_t v, int cls)
{
    EmitBuf *b = &j->buf;
    rc_vote2(j, vm, cls, 1);
    int s = rc_slot(j, vm);
    if (s < 0) return;
    RcSlot *r = &j->rc[s];
    int dst = r->xmm ? R11 : r->phys;
    if (v == (int32_t)v) emit_mov_reg_imm32(b, dst, (int32_t)v);
    else emit_mov_reg_imm64(b, dst, (uint64_t)v);
    if (r->xmm) emit_movq_xmm_gpr(b, r->phys, R11);
    j->rc_valid |= 1u << s;
}

/* Entry: load every copy (only when pc 0 is a jump target, see §15.5). */
static void rc_emit_preload(JitCtx *j)
{
    j->rc_valid = 0;
    if (!j->rc_n || !j->rc_preload) return;
    for (int s = 0; s < j->rc_n; s++) rc_get(j, s);
}

/* ---- jump emission with fixups ---- */

/* cc < 0 => unconditional JMP. target_pc is an absolute bytecode pc. */
static void emit_jump(JitCtx *j, int cc, int target_pc)
{
    EmitBuf *b = &j->buf;
    if (cc < 0)
        emit_jmp_rel32(b, 0);
    else
        emit_jcc_rel32(b, cc, 0);
    /* an instruction may emit several jumps (error / stack-overflow exits) */
    if (j->nfixups == j->cap_fixups) {
        j->cap_fixups = j->cap_fixups ? j->cap_fixups * 2 : 16;
        j->fixups = realloc(j->fixups, j->cap_fixups * sizeof(Fixup));
    }
    j->fixups[j->nfixups].patch_off = emit_pos(b) - 4;
    j->fixups[j->nfixups].target_pc = target_pc;
    j->nfixups++;
    if (target_pc >= 0 && target_pc < j->num_insns) {
        rc_edge(j, target_pc, j->rc_valid);
        j->rc_jt_new[target_pc] = 1;
        if (j->rc_collect && target_pc <= j->cur_pc) {
            j->backs = grow(j->backs, &j->cap_backs, j->nbacks + 2, sizeof(int));
            j->backs[j->nbacks++] = target_pc;
            j->backs[j->nbacks++] = j->cur_pc;
        }
    }
}

/* ---- instruction helpers ---- */

static void emit_store_tag(JitCtx *j, int rd, int32_t tag)
{
    emit_mov_mem_imm32(&j->buf, RBX, TAG_DISP(rd), tag);
}

/* Store RAX as the payload of rd with an INT64 tag. */
static void emit_store_int64(JitCtx *j, int rd)
{
    emit_mov_mem_reg(&j->buf, RBX, VAL_DISP(rd), RAX);
    emit_store_tag(j, rd, TAG_INT64);
}

/* Store GPR r as the payload of rd with an explicit tag, refreshing rd's copy. */
static void emit_store_reg_tagged(JitCtx *j, int rd, int r, int32_t tag)
{
    emit_mov_mem_reg(&j->buf, RBX, VAL_DISP(rd), r);
    emit_store_tag(j, rd, tag);
    rc_def(j, rd, r, 0, 0);
}

/* Store RAX as the payload of rd with an explicit tag (INT64 / UINT64). */
static void emit_store_tagged(JitCtx *j, int rd, int32_t tag)
{
    emit_store_reg_tagged(j, rd, RAX, tag);
}

/* rd = rs ALU rt, result tagged `tag`. alu_op is an ALU_* code or ALU_MUL.
 * The payload op is identical for the INT and UINT families; only the result
 * tag differs (two's-complement add/sub/mul/bitwise are sign-agnostic). */
static void emit_alu_rrr(JitCtx *j, int alu_op, int rd, int rs, int rt, int32_t tag)
{
    int d = rc_dst(j, rd, rt, 0);
    rc_load_gpr(j, d, rs, 0);
    rc_alu(j, alu_op, d, rt);
    emit_store_reg_tagged(j, rd, d, tag);
}

/* rd = rs ALU imm, result tagged `tag`. */
static void emit_alu_rrimm(JitCtx *j, int alu_op, int rd, int rs, int32_t imm,
                           int32_t tag)
{
    EmitBuf *b = &j->buf;
    int d = rc_dst(j, rd, -1, 0);
    rc_load_gpr(j, d, rs, 0);
    if (alu_op == ALU_MUL) {
        emit_mov_reg_imm32(b, RCX, imm);
        emit_imul_reg_reg(b, d, RCX);
    } else {
        emit_alu_reg_imm32(b, alu_op, d, imm);
    }
    emit_store_reg_tagged(j, rd, d, tag);
}

static void emit_raise_at(JitCtx *j, int what, int pc_abs);

/* rd = rs / rt (is_mod=0) or rs % rt (is_mod=1). Signed uses cqo+idiv; unsigned
 * uses xor edx,edx+div. Result tag INT64 / UINT64 accordingly. Like the
 * interpreter, a zero divisor raises "division by zero", and a signed divisor
 * of -1 gives -rs (INT64_MIN wraps) and 0 for the remainder: idiv would trap. */
static void emit_div_rrr(JitCtx *j, int rd, int rs, int rt, int is_mod,
                         int is_unsigned, int pc_abs)
{
    EmitBuf *b = &j->buf;
    /* both operands are fetched before the raise path (one cache state per op) */
    rc_load_gpr(j, RCX, rt, 0);
    rc_load_gpr(j, RAX, rs, 0);
    emit_alu_reg_imm32(b, ALU_CMP, RCX, 0);
    size_t jnz = emit_pos(b);
    emit_jcc_rel32(b, CC_NE, 0);
    emit_raise_at(j, JIT_RAISE_DIV_ZERO, pc_abs);
    emit_patch_i32(b, jnz + 2, (int32_t)(emit_pos(b) - (jnz + 6)));

    size_t jdone = 0;
    if (!is_unsigned) {
        emit_alu_reg_imm32(b, ALU_CMP, RCX, -1);
        size_t jne = emit_pos(b);
        emit_jcc_rel32(b, CC_NE, 0);
        if (is_mod)
            emit_alu_reg_reg(b, ALU_XOR, RDX, RDX);
        else
            emit_neg_reg(b, RAX);
        jdone = emit_pos(b);
        emit_jmp_rel32(b, 0);
        emit_patch_i32(b, jne + 2, (int32_t)(emit_pos(b) - (jne + 6)));
    }
    if (is_unsigned) {
        emit_alu_reg_reg(b, ALU_XOR, RDX, RDX);
        emit_div_reg(b, RCX);
    } else {
        emit_cqo(b);
        emit_idiv_reg(b, RCX);
    }
    if (jdone) emit_patch_i32(b, jdone + 1, (int32_t)(emit_pos(b) - (jdone + 5)));
    emit_mov_mem_reg(b, RBX, VAL_DISP(rd), is_mod ? RDX : RAX);
    emit_store_tag(j, rd, is_unsigned ? TAG_UINT64 : TAG_INT64);
    rc_def(j, rd, is_mod ? RDX : RAX, 0, 0);
}

static void emit_div_rrimm(JitCtx *j, int rd, int rs, int32_t imm, int is_mod,
                           int is_unsigned)
{
    EmitBuf *b = &j->buf;
    rc_load_gpr(j, RAX, rs, 0);
    emit_mov_reg_imm32(b, RCX, imm);
    if (is_unsigned) {
        emit_alu_reg_reg(b, ALU_XOR, RDX, RDX);
        emit_div_reg(b, RCX);
    } else {
        emit_cqo(b);
        emit_idiv_reg(b, RCX);
    }
    emit_mov_mem_reg(b, RBX, VAL_DISP(rd), is_mod ? RDX : RAX);
    emit_store_tag(j, rd, is_unsigned ? TAG_UINT64 : TAG_INT64);
    rc_def(j, rd, is_mod ? RDX : RAX, 0, 0);
}

/* rd = (rs CC rt) as a BOOL. */
static void emit_int_cmp_rrr(JitCtx *j, int cc, int rd, int rs, int rt)
{
    EmitBuf *b = &j->buf;
    rc_load_gpr(j, RAX, rs, 0);
    rc_alu(j, ALU_CMP, RAX, rt);
    emit_setcc_reg(b, cc, RAX);
    emit_mov_mem_reg(b, RBX, VAL_DISP(rd), RAX);
    emit_store_tag(j, rd, TAG_BOOL);
    rc_def(j, rd, RAX, 0, 0);
}

static void emit_int_cmp_rrimm(JitCtx *j, int cc, int rd, int rs, int32_t imm)
{
    EmitBuf *b = &j->buf;
    rc_load_gpr(j, RAX, rs, 0);
    emit_alu_reg_imm32(b, ALU_CMP, RAX, imm);
    emit_setcc_reg(b, cc, RAX);
    emit_mov_mem_reg(b, RBX, VAL_DISP(rd), RAX);
    emit_store_tag(j, rd, TAG_BOOL);
    rc_def(j, rd, RAX, 0, 0);
}

/* Fused compare-and-branch: if (rs CC rt) goto target_pc. */
static void emit_jmp_cmp_rrr(JitCtx *j, int cc, int rs, int rt, int target_pc)
{
    int p = rc_gpr(j, rs);
    if (p < 0) {
        p = RAX;
        emit_mov_reg_mem(&j->buf, RAX, RBX, VAL_DISP(rs));
    }
    rc_alu(j, ALU_CMP, p, rt);
    emit_jump(j, cc, target_pc);
}

static void emit_jmp_cmp_rrimm(JitCtx *j, int cc, int rs, int32_t imm, int target_pc)
{
    EmitBuf *b = &j->buf;
    int p = rc_gpr(j, rs);
    if (p >= 0) emit_alu_reg_imm32(b, ALU_CMP, p, imm);
    else emit_alu_mem_imm32(b, ALU_CMP, RBX, VAL_DISP(rs), imm);
    emit_jump(j, cc, target_pc);
}

/* Register-form shift (OP_INT/UINT_SHL/SHR). Unlike the _IMM forms, the count
 * is validated at run time: kl_jit_shift_check mirrors the interpreter's
 * kl_check_shift_count + SAVE_PC + trace, returning 0 on raise so the native
 * code branches to the error epilogue. shift_kind: 0=shl, 1=sar, 2=shr.
 *   rdi = &regs[rt] (the count TValue), esi = pc_abs */
static void emit_reg_shift(JitCtx *j, int rd, int rs, int rt, int shift_kind,
                           int32_t tag, int pc_abs)
{
    EmitBuf *b = &j->buf;
    emit_mov_reg_reg(b, RDI, RBX);
    emit_alu_reg_imm32(b, ALU_ADD, RDI, rt * 16);
    emit_mov_reg_imm32(b, RSI, pc_abs);
    emit_sub_rsp_imm32(b, 8);
    emit_mov_reg_imm64(b, RAX, (uint64_t)(uintptr_t)kl_jit_shift_check);
    emit_call_reg(b, RAX);
    emit_add_rsp_imm32(b, 8);
    emit_alu_reg_imm32(b, ALU_CMP, RAX, 0);
    emit_jump(j, CC_E, JIT_ERR_TARGET);
    /* count is valid: rax = value, rcx = count, shift by cl */
    emit_mov_reg_mem(b, RAX, RBX, VAL_DISP(rs));
    emit_mov_reg_mem(b, RCX, RBX, VAL_DISP(rt));
    if (shift_kind == 0)
        emit_shl_reg_cl(b, RAX);
    else if (shift_kind == 1)
        emit_sar_reg_cl(b, RAX);
    else
        emit_shr_reg_cl(b, RAX);
    emit_store_tagged(j, rd, tag);
}

/* Float comparison opcode -> compare code (0=EQ 1=NE 2=LT 3=LE 4=GT 5=GE). */
static int float_cmp_op(OpCode op)
{
    switch (op) {
        case OP_FLOAT_EQ:
        case OP_JMP_FLOAT_EQ:
            return 0;
        case OP_FLOAT_NE:
        case OP_JMP_FLOAT_NE:
            return 1;
        case OP_FLOAT_LT:
        case OP_JMP_FLOAT_LT:
            return 2;
        case OP_FLOAT_LE:
        case OP_JMP_FLOAT_LE:
            return 3;
        case OP_FLOAT_GT:
        case OP_JMP_FLOAT_GT:
            return 4;
        case OP_FLOAT_GE:
        case OP_JMP_FLOAT_GE:
            return 5;
        default:
            return -1;
    }
}

/* rd = rs OP rt (scalar double, SSE2), result tag FLOAT64. The payload lives in
 * the same 8-byte slot as ival; values spill to the TValue file between ops, so
 * XMM0 is safe scratch. */
static void emit_float_arith(JitCtx *j, int sse_op, int rd, int rs, int rt)
{
    EmitBuf *b = &j->buf;
    int d = rc_dst(j, rd, rt, 1);
    rc_load_xmm(j, d, rs);
    rc_sse(j, sse_op, d, rt);
    emit_movsd_mem_reg(b, RBX, VAL_DISP(rd), d);
    emit_store_tag(j, rd, TAG_FLOAT64);
    rc_def(j, rd, d, 1, 1);
}

/* rd = -rs (double). IEEE negation is a sign-bit flip, done on the raw bits with
 * GPRs (bit-exact with the interpreter's `-fval`, including NaN/zero). */
static void emit_float_neg(JitCtx *j, int rd, int rs)
{
    EmitBuf *b = &j->buf;
    rc_load_gpr(j, RAX, rs, 1);
    emit_mov_reg_imm64(b, RDX, (uint64_t)0x8000000000000000ULL);
    /* emit_alu_reg_reg writes its last argument: dst = dst OP src, so RAX = RAX ^ RDX */
    emit_alu_reg_reg(b, ALU_XOR, RDX, RAX);
    emit_mov_mem_reg(b, RBX, VAL_DISP(rd), RAX);
    emit_store_tag(j, rd, TAG_FLOAT64);
    rc_def(j, rd, RAX, 0, 1);
}

/* rd = fmod(rs, rt): args in xmm0/xmm1, result in xmm0 (System V). */
static void emit_float_mod(JitCtx *j, int rd, int rs, int rt)
{
    EmitBuf *b = &j->buf;
    emit_movsd_reg_mem(b, XMM0, RBX, VAL_DISP(rs));
    emit_movsd_reg_mem(b, XMM1, RBX, VAL_DISP(rt));
    emit_sub_rsp_imm32(b, 8);
    emit_mov_reg_imm64(b, RAX, (uint64_t)(uintptr_t)fmod);
    emit_call_reg(b, RAX);
    emit_add_rsp_imm32(b, 8);
    emit_movsd_mem_reg(b, RBX, VAL_DISP(rd), XMM0);
    emit_store_tag(j, rd, TAG_FLOAT64);
}

/* Inline IEEE compare (C semantics, matching the interpreter's OP_FLOAT_*):
 * ucomisd sets ZF/PF/CF = 1/1/1 when unordered (NaN). GT/GE use `a > b` =
 * above / above-or-equal (false when unordered); LT/LE swap the operands. EQ
 * needs ZF=1 && PF=0, NE is ZF=0 || PF=1. Returns the condition code for the
 * single-flag cases, or -1 for EQ/NE (handled by the callers). */
static int emit_float_ucomi(JitCtx *j, int rs, int rt, int cmp_op)
{
    EmitBuf *b = &j->buf;
    int swap = cmp_op == 2 || cmp_op == 3;
    int a = swap ? rt : rs, c = swap ? rs : rt;
    int pa = rc_xmm(j, a);
    if (pa < 0) {
        pa = XMM0;
        emit_movsd_reg_mem(b, XMM0, RBX, VAL_DISP(a));
    }
    int pc = rc_xmm(j, c);
    if (pc >= 0) emit_ucomisd_reg_reg(b, pa, pc);
    else emit_ucomisd_reg_mem(b, pa, RBX, VAL_DISP(c));
    switch (cmp_op) {
        case 2: case 4: return CC_A;
        case 3: case 5: return CC_AE;
        default: return -1;
    }
}

/* rd = (rs CC rt) as BOOL. */
static void emit_float_cmp_bool(JitCtx *j, int rd, int rs, int rt, int cmp_op)
{
    EmitBuf *b = &j->buf;
    int cc = emit_float_ucomi(j, rs, rt, cmp_op);
    if (cc >= 0) {
        emit_setcc_reg(b, cc, RAX);
    } else {
        /* rcx = PF test, rax = ZF test; combine (setcc only writes al) */
        emit_setcc_reg(b, cmp_op == 0 ? CC_NP : CC_P, RCX);
        emit_setcc_reg(b, cmp_op == 0 ? CC_E : CC_NE, RAX);
        emit_alu_reg_reg(b, cmp_op == 0 ? ALU_AND : ALU_OR, RCX, RAX);
    }
    emit_mov_mem_reg(b, RBX, VAL_DISP(rd), RAX);
    emit_store_tag(j, rd, TAG_BOOL);
    rc_def(j, rd, RAX, 0, 0);
}

/* Fused float compare-and-branch: if (rs CC rt) goto target_pc. */
static void emit_float_jmp_cmp(JitCtx *j, int rs, int rt, int cmp_op, int target_pc)
{
    EmitBuf *b = &j->buf;
    int cc = emit_float_ucomi(j, rs, rt, cmp_op);
    if (cc >= 0) {
        emit_jump(j, cc, target_pc);
    } else if (cmp_op == 0) {
        size_t jp = emit_pos(b); /* unordered: not equal, fall through */
        emit_jcc_rel32(b, CC_P, 0);
        emit_jump(j, CC_E, target_pc);
        emit_patch_i32(b, jp + 2, (int32_t)(emit_pos(b) - (jp + 6)));
    } else {
        emit_jump(j, CC_P, target_pc);
        emit_jump(j, CC_NE, target_pc);
    }
}

/* Cast helper call: fn(regs, rd, rs, mode, dst_ti). May abort (trap mode); the
 * helper writes regs[rd] directly, no TAG_ERROR. Used by int/float casts. */
static void emit_cast_call(JitCtx *j, int rd, int rs, int mode, int dst_ti, void *fn)
{
    EmitBuf *b = &j->buf;
    emit_mov_reg_reg(b, RDI, RBX);
    emit_mov_reg_imm32(b, RSI, rd);
    emit_mov_reg_imm32(b, RDX, rs);
    emit_mov_reg_imm32(b, RCX, mode);
    emit_mov_reg_imm32(b, R8, dst_ti);
    emit_sub_rsp_imm32(b, 8);
    emit_mov_reg_imm64(b, RAX, (uint64_t)(uintptr_t)fn);
    emit_call_reg(b, RAX);
    emit_add_rsp_imm32(b, 8);
}

/* OP_NUM_* -> slot id; *is_cmp set for the comparison family. -1 if not a num op. */
static int num_slot(OpCode op, int *is_cmp)
{
    *is_cmp = 0;
    switch (op) {
        case OP_NUM_ADD: return SLOT_ADD;
        case OP_NUM_SUB: return SLOT_SUB;
        case OP_NUM_MUL: return SLOT_MUL;
        case OP_NUM_DIV: return SLOT_DIV;
        case OP_NUM_MOD: return SLOT_MOD;
        case OP_NUM_SHL: return SLOT_SHL;
        case OP_NUM_SHR: return SLOT_SHR;
        case OP_NUM_AND: return SLOT_BIT_AND;
        case OP_NUM_OR: return SLOT_BIT_OR;
        case OP_NUM_XOR: return SLOT_BIT_XOR;
        case OP_NUM_EQ: *is_cmp = 1; return SLOT_EQ;
        case OP_NUM_NE: *is_cmp = 1; return SLOT_NE;
        case OP_NUM_LT: *is_cmp = 1; return SLOT_LT;
        case OP_NUM_LE: *is_cmp = 1; return SLOT_LE;
        case OP_NUM_GT: *is_cmp = 1; return SLOT_GT;
        case OP_NUM_GE: *is_cmp = 1; return SLOT_GE;
        default: return -1;
    }
}

/* Generic numeric protocol op: call kl_jit_num_arith/cmp(&regs[rs], &regs[rt],
 * slot, pc). The helper returns a TValue (tag in rax, payload in rdx); on
 * TAG_ERROR jump to the error epilogue (the helper already traced this frame),
 * otherwise store the result into regs[rd]. */
static void emit_num_binop(JitCtx *j, int rd, int rs, int rt, int slot, int is_cmp,
                           int pc_abs)
{
    EmitBuf *b = &j->buf;
    emit_mov_reg_reg(b, RDI, RBX);
    emit_alu_reg_imm32(b, ALU_ADD, RDI, rs * 16);
    emit_mov_reg_reg(b, RSI, RBX);
    emit_alu_reg_imm32(b, ALU_ADD, RSI, rt * 16);
    emit_mov_reg_imm32(b, RDX, slot);
    emit_mov_reg_imm32(b, RCX, pc_abs);
    emit_sub_rsp_imm32(b, 8);
    void *fn = is_cmp ? (void *)kl_jit_num_cmp : (void *)kl_jit_num_arith;
    emit_mov_reg_imm64(b, RAX, (uint64_t)(uintptr_t)fn);
    emit_call_reg(b, RAX);
    emit_add_rsp_imm32(b, 8);
    emit_alu_reg_imm32(b, ALU_CMP, RAX, TAG_ERROR);
    emit_jump(j, CC_E, JIT_ERR_TARGET);
    emit_mov_mem_reg(b, RBX, TAG_DISP(rd), RAX);
    emit_mov_mem_reg(b, RBX, VAL_DISP(rd), RDX);
}

/* ---- A group: object / container / string / IO ---- */

/* dst = &regs[vm] */
static void emit_reg_addr(JitCtx *j, int dst, int vm)
{
    emit_mov_reg_reg(&j->buf, dst, RBX);
    emit_alu_reg_imm32(&j->buf, ALU_ADD, dst, vm * 16);
}
/* store a returned TValue (rax=tag, rdx=payload) into regs[rd] */
static void emit_store_ret(JitCtx *j, int rd)
{
    emit_mov_mem_reg(&j->buf, RBX, TAG_DISP(rd), RAX);
    emit_mov_mem_reg(&j->buf, RBX, VAL_DISP(rd), RDX);
}
/* rax == TAG_ERROR -> error epilogue */
static void emit_err_check(JitCtx *j)
{
    emit_alu_reg_imm32(&j->buf, ALU_CMP, RAX, TAG_ERROR);
    emit_jump(j, CC_E, JIT_ERR_TARGET);
}
/* A direct self call does not maintain ks->stack_top (the levels share one
 * CallFrame), so in a self-only function every helper call first sets it to
 * this level's call area, rbx + nlocals: helpers build values there or push
 * frames for koala code above it. rax/r11 only: the args are already loaded. */
static void emit_call_abs(JitCtx *j, void *fn)
{
    if (j->self_only) {
        emit_mov_reg_mem(&j->buf, RAX, RBP, FRAME_KS);
        emit_mov_reg_reg(&j->buf, R11, RBX);
        emit_alu_reg_imm32(&j->buf, ALU_ADD, R11, j->nlocals * 16);
        emit_mov_mem_reg(&j->buf, RAX, (int32_t)offsetof(KoalaState, stack_top), R11);
    }
    emit_mov_reg_imm64(&j->buf, RAX, (uint64_t)(uintptr_t)fn);
    emit_call_reg(&j->buf, RAX);
}

/* Generic slot call: kl_jit_slot3(&regs[a0], &regs[a1], &regs[a2], nargs, slot, pc).
 * a1/a2 < 0 pass NULL. store=1 writes the result to regs[rd]. */
static void emit_slot3(JitCtx *j, int rd, int a0, int a1, int a2, int nargs,
                       int slot, int pc_abs, int store)
{
    EmitBuf *b = &j->buf;
    emit_reg_addr(j, RDI, a0);
    if (a1 >= 0) emit_reg_addr(j, RSI, a1); else emit_mov_reg_imm32(b, RSI, 0);
    if (a2 >= 0) emit_reg_addr(j, RDX, a2); else emit_mov_reg_imm32(b, RDX, 0);
    emit_mov_reg_imm32(b, RCX, nargs);
    emit_mov_reg_imm32(b, R8, slot);
    emit_mov_reg_imm32(b, R9, pc_abs);
    emit_sub_rsp_imm32(b, 8);
    emit_call_abs(j, (void *)kl_jit_slot3);
    emit_add_rsp_imm32(b, 8);
    emit_err_check(j);
    if (store && rd != 0xFFF) emit_store_ret(j, rd);
}

static void emit_get_item_imm(JitCtx *j, int rd, int rs, int imm, int pc_abs)
{
    EmitBuf *b = &j->buf;
    emit_reg_addr(j, RDI, rs);
    emit_mov_reg_imm64(b, RSI, (uint64_t)(int64_t)imm);
    emit_mov_reg_imm32(b, RDX, pc_abs);
    emit_sub_rsp_imm32(b, 8);
    emit_call_abs(j, (void *)kl_jit_get_item_imm);
    emit_add_rsp_imm32(b, 8);
    emit_err_check(j);
    if (rd != 0xFFF) emit_store_ret(j, rd);
}

static void emit_set_item_imm(JitCtx *j, int rd, int rs, int imm, int pc_abs)
{
    EmitBuf *b = &j->buf;
    emit_reg_addr(j, RDI, rd);
    emit_mov_reg_imm64(b, RSI, (uint64_t)(int64_t)imm);
    emit_reg_addr(j, RDX, rs);
    emit_mov_reg_imm32(b, RCX, pc_abs);
    emit_sub_rsp_imm32(b, 8);
    emit_call_abs(j, (void *)kl_jit_set_item_imm);
    emit_add_rsp_imm32(b, 8);
    emit_err_check(j);
}

static void emit_hash(JitCtx *j, int rd, int rs, int pc_abs)
{
    EmitBuf *b = &j->buf;
    emit_reg_addr(j, RDI, rs);
    emit_mov_reg_imm32(b, RSI, pc_abs);
    emit_sub_rsp_imm32(b, 8);
    emit_call_abs(j, (void *)kl_jit_hash);
    emit_add_rsp_imm32(b, 8);
    emit_err_check(j);
    if (rd != 0xFFF) emit_store_ret(j, rd);
}

static void emit_fmt(JitCtx *j, int rs, int rt, int pc_abs)
{
    EmitBuf *b = &j->buf;
    emit_reg_addr(j, RDI, rs);
    emit_reg_addr(j, RSI, rt);
    emit_mov_reg_imm32(b, RDX, pc_abs);
    emit_sub_rsp_imm32(b, 8);
    emit_call_abs(j, (void *)kl_jit_fmt);
    emit_add_rsp_imm32(b, 8);
    emit_err_check(j);
}

static void emit_print(JitCtx *j, int nargs, int pc_abs)
{
    EmitBuf *b = &j->buf;
    emit_mov_reg_imm32(b, RDI, nargs);
    emit_mov_reg_imm32(b, RSI, pc_abs);
    emit_sub_rsp_imm32(b, 8);
    emit_call_abs(j, (void *)kl_jit_print);
    emit_add_rsp_imm32(b, 8);
    emit_err_check(j);
}

static void emit_new(JitCtx *j, int rd, TypeObject *tp)
{
    EmitBuf *b = &j->buf;
    emit_mov_reg_imm64(b, RDI, (uint64_t)(uintptr_t)tp);
    emit_sub_rsp_imm32(b, 8);
    emit_call_abs(j, (void *)kl_jit_new);
    emit_add_rsp_imm32(b, 8);
    emit_store_ret(j, rd);
}

static void emit_build_intern(JitCtx *j, int rd, int tag, int count)
{
    EmitBuf *b = &j->buf;
    emit_mov_reg_imm32(b, RDI, tag);
    emit_mov_reg_imm32(b, RSI, count);
    emit_sub_rsp_imm32(b, 8);
    emit_call_abs(j, (void *)kl_jit_build_intern);
    emit_add_rsp_imm32(b, 8);
    if (rd != 0xFFF) emit_store_ret(j, rd);
}

/* fields[off] lives at obj + 16 + off*16 (InstObject: _type, _nfields, then
 * TValue fields[]). to_obj() yields the payload pointer for both ref and intf. */
static void emit_get_field(JitCtx *j, int rd, int rs, int off)
{
    EmitBuf *b = &j->buf;
    int fd = 16 + off * 16;
    emit_mov_reg_mem(b, RAX, RBX, VAL_DISP(rs));
    emit_mov_reg_mem(b, RDX, RAX, fd);
    emit_mov_reg_mem(b, RCX, RAX, fd + 8);
    emit_mov_mem_reg(b, RBX, TAG_DISP(rd), RDX);
    emit_mov_mem_reg(b, RBX, VAL_DISP(rd), RCX);
}

static void emit_set_field(JitCtx *j, int rd, int rs, int off)
{
    EmitBuf *b = &j->buf;
    int fd = 16 + off * 16;
    emit_mov_reg_mem(b, RAX, RBX, VAL_DISP(rd));
    emit_mov_reg_mem(b, RDX, RBX, TAG_DISP(rs));
    emit_mov_reg_mem(b, RCX, RBX, VAL_DISP(rs));
    emit_mov_mem_reg(b, RAX, fd, RDX);
    emit_mov_mem_reg(b, RAX, fd + 8, RCX);
}

static void emit_global_get(JitCtx *j, int rs, TValue *addr)
{
    EmitBuf *b = &j->buf;
    emit_mov_reg_imm64(b, RAX, (uint64_t)(uintptr_t)addr);
    emit_mov_reg_mem(b, RDX, RAX, 0);
    emit_mov_reg_mem(b, RCX, RAX, 8);
    emit_mov_mem_reg(b, RBX, TAG_DISP(rs), RDX);
    emit_mov_mem_reg(b, RBX, VAL_DISP(rs), RCX);
}

static void emit_global_set(JitCtx *j, int rs, TValue *addr)
{
    EmitBuf *b = &j->buf;
    emit_mov_reg_imm64(b, RAX, (uint64_t)(uintptr_t)addr);
    emit_mov_reg_mem(b, RDX, RBX, TAG_DISP(rs));
    emit_mov_reg_mem(b, RCX, RBX, VAL_DISP(rs));
    emit_mov_mem_reg(b, RAX, 0, RDX);
    emit_mov_mem_reg(b, RAX, 8, RCX);
}

/* rd = (regs[rs].tag == TAG_NIL) ? 1 : 0  (cc = CC_E for eq_null, CC_NE for ne) */
static void emit_ref_null(JitCtx *j, int rd, int rs, int cc)
{
    EmitBuf *b = &j->buf;
    emit_mov_reg_mem(b, RAX, RBX, TAG_DISP(rs));
    emit_alu_reg_imm32(b, ALU_CMP, RAX, TAG_NIL);
    emit_setcc_reg(b, cc, RAX);
    emit_mov_mem_reg(b, RBX, VAL_DISP(rd), RAX);
    emit_store_tag(j, rd, TAG_BOOL);
}

static void emit_jmp_ref_null(JitCtx *j, int rs, int cc, int target)
{
    EmitBuf *b = &j->buf;
    emit_mov_reg_mem(b, RAX, RBX, TAG_DISP(rs));
    emit_alu_reg_imm32(b, ALU_CMP, RAX, TAG_NIL);
    emit_jump(j, cc, target);
}

/* if (regs[rs].ival) regs[rd] = regs[rt] */
static void emit_move_true(JitCtx *j, int rd, int rs, int rt)
{
    EmitBuf *b = &j->buf;
    emit_alu_mem_imm32(b, ALU_CMP, RBX, VAL_DISP(rs), 0);
    size_t jcc = emit_pos(b);
    emit_jcc_rel32(b, CC_E, 0);
    emit_mov_reg_mem(b, RAX, RBX, TAG_DISP(rt));
    emit_mov_reg_mem(b, RDX, RBX, VAL_DISP(rt));
    emit_mov_mem_reg(b, RBX, TAG_DISP(rd), RAX);
    emit_mov_mem_reg(b, RBX, VAL_DISP(rd), RDX);
    emit_patch_i32(b, jcc + 2, (int32_t)(emit_pos(b) - (jcc + 6)));
}

/* Ordinary call (flag=0) through the runtime helper kl_jit_call_helper, which
 * mirrors the interpreter's OP_CALL: IS_CFUNC -> kl_call_cfunc, else kl_call_code
 * (which itself dispatches to native when the callee is JIT-compiled, interpreter
 * otherwise). By the VM sliding-window convention the outgoing args sit at
 * caller_regs + nlocals, which equals ks->stack_top at a native call site; the
 * result lands in caller_regs[rd].
 *   rdi = args = RBX + nlocals*16
 *   rsi = out  = RBX + rd*16 (NULL when rd == 0xFFF, a void call)
 *   rdx = fn   (resolved ENTRY(payload), embedded as imm64)
 *   rcx = nargs
 *   r8  = pc_abs (absolute bytecode pc of this OP_CALL, for cf->pc/traceback)
 * The helper returns the callee's TValue in rax:rdx; a TAG_ERROR result jumps
 * to the error epilogue, mirroring the interpreter's `goto error`.
 * RBX/RBP are callee-saved (prologue/epilogue preserve them); rax/rcx/rdx/rsi/rdi
 * are scratch. The body runs with rsp ≡ 8 (mod 16), so pad 8 to reach the 16-byte
 * alignment the ABI requires at a call site. The helper address is absolute
 * (imm64) and called indirectly, so the code is position-independent: no fixup. */
static void emit_helper_call(JitCtx *j, int rd, int nargs, int nlocals, Object *fn,
                             int pc_abs)
{
    EmitBuf *b = &j->buf;
    emit_mov_reg_reg(b, RDI, RBX);
    emit_alu_reg_imm32(b, ALU_ADD, RDI, nlocals * 16);
    if (rd == 0xFFF) {
        emit_mov_reg_imm32(b, RSI, 0);
    } else {
        emit_mov_reg_reg(b, RSI, RBX);
        emit_alu_reg_imm32(b, ALU_ADD, RSI, rd * 16);
    }
    emit_mov_reg_imm64(b, RDX, (uint64_t)(uintptr_t)fn);
    emit_mov_reg_imm32(b, RCX, nargs);
    emit_mov_reg_imm32(b, R8, pc_abs);
    emit_sub_rsp_imm32(b, 8);
    emit_mov_reg_imm64(b, RAX, (uint64_t)(uintptr_t)kl_jit_call_helper);
    emit_call_reg(b, RAX);
    emit_add_rsp_imm32(b, 8);
    /* rax = ret.tag: propagate errors like the interpreter's OP_CALL */
    emit_alu_reg_imm32(b, ALU_CMP, RAX, TAG_ERROR);
    emit_jump(j, CC_E, JIT_ERR_TARGET);
}

/* OP_CALL flag 1 (callee_reg < 0: trait method `index` of the receiver) or
 * flag 2 (function value in callee_reg) through kl_jit_call_dyn. */
static void emit_dyn_call(JitCtx *j, int rd, int nargs, int nlocals, int callee_reg,
                          int index, int pc_abs)
{
    EmitBuf *b = &j->buf;
    emit_reg_addr(j, RDI, nlocals);
    if (rd == 0xFFF)
        emit_mov_reg_imm32(b, RSI, 0);
    else
        emit_reg_addr(j, RSI, rd);
    if (callee_reg >= 0)
        emit_reg_addr(j, RDX, callee_reg);
    else
        emit_mov_reg_imm32(b, RDX, 0);
    emit_mov_reg_imm32(b, RCX, index);
    emit_mov_reg_imm32(b, R8, nargs);
    emit_mov_reg_imm32(b, R9, pc_abs);
    emit_sub_rsp_imm32(b, 8);
    emit_call_abs(j, (void *)kl_jit_call_dyn);
    emit_add_rsp_imm32(b, 8);
    emit_err_check(j);
}

/* OP_TAIL_CALL to another function through kl_jit_tail_call, which writes *out
 * (the native callee's result or the JIT_TAG_TAILCALL request), then return. */
static void emit_tail_call(JitCtx *j, int nargs, int nlocals, Object *fn, int callee_reg,
                           int index, int pc_abs)
{
    EmitBuf *b = &j->buf;
    emit_reg_addr(j, RDI, nlocals);
    emit_mov_reg_mem(b, RSI, RBP, FRAME_OUT);
    emit_mov_reg_imm64(b, RDX, (uint64_t)(uintptr_t)fn);
    if (callee_reg >= 0)
        emit_reg_addr(j, RCX, callee_reg);
    else
        emit_mov_reg_imm32(b, RCX, 0);
    emit_mov_reg_imm32(b, R8, index);
    emit_mov_reg_imm64(b, R9, ((uint64_t)(uint32_t)pc_abs << 8) | (uint32_t)nargs);
    emit_sub_rsp_imm32(b, 8);
    emit_call_abs(j, (void *)kl_jit_tail_call);
    emit_add_rsp_imm32(b, 8);
    emit_err_check(j);
    emit_epilogue(j);
}

/* kl_jit_raise_at(what, pc), then the error epilogue. */
static void emit_raise_at(JitCtx *j, int what, int pc_abs)
{
    EmitBuf *b = &j->buf;
    emit_mov_reg_imm32(b, RDI, what);
    emit_mov_reg_imm32(b, RSI, pc_abs);
    emit_sub_rsp_imm32(b, 8);
    emit_call_abs(j, (void *)kl_jit_raise_at);
    emit_add_rsp_imm32(b, 8);
    emit_jump(j, -1, JIT_ERR_TARGET);
}

/* rax = ks; rcx = rbx + n * 16 */
static void emit_stack_slot(JitCtx *j, int n)
{
    EmitBuf *b = &j->buf;
    emit_mov_reg_mem(b, RAX, RBP, FRAME_KS);
    emit_mov_reg_reg(b, RCX, RBX);
    emit_alu_reg_imm32(b, ALU_ADD, RCX, n * 16);
}

/* Direct self-recursive call: re-enter this function's native entry (offset 0)
 * with a plain near call, skipping kl_call_code's frame machinery. Only used when
 * EVERY call in the function targets itself. The levels share the outer CallFrame
 * (ks->cf) and leave ks->stack_top alone: helper calls set it (emit_call_abs),
 * and the native entry points reset it on return.
 *   - check that the new level's registers and call area fit the VM stack, and
 *     that the native stack has room, else raise "stack overflow" (the
 *     interpreter only asserts);
 *   - an error result traces this level at the call site (kl_jit_self_error),
 *     as the interpreter does for each frame, and takes the error epilogue.
 * The outgoing args already sit at RBX + nlocals*16 (the sliding window). A void
 * call (rd == 0xFFF) returns into the scratch slot of this machine frame: the
 * callee always writes *out. The target is offset 0: rel = 0 - (pos_of_E8 + 5).
 *   rdi = args = RBX + nlocals*16
 *   rsi = out  = RBX + rd*16, or the scratch slot
 *   rdx = ks */
static void emit_self_call(JitCtx *j, int rd, int nlocals, int max_call_args, int pc_abs)
{
    EmitBuf *b = &j->buf;
    /* the new level: registers at rbx + nlocals, then its own call area */
    emit_stack_slot(j, 2 * nlocals + max_call_args);
    emit_alu_reg_mem(b, ALU_CMP, RCX, RAX, (int32_t)offsetof(KoalaState, stack_limit));
    size_t jbe = emit_pos(b);
    emit_jcc_rel32(b, CC_BE, 0);
    emit_raise_at(j, JIT_RAISE_STACK_OVERFLOW, pc_abs);
    emit_patch_i32(b, jbe + 2, (int32_t)(emit_pos(b) - (jbe + 6)));
    /* and the native stack: rsp > ks->c_stack_limit (NULL: unknown, no check) */
    emit_alu_reg_mem(b, ALU_CMP, RSP, RAX, (int32_t)offsetof(KoalaState, c_stack_limit));
    size_t ja = emit_pos(b);
    emit_jcc_rel32(b, CC_A, 0);
    emit_raise_at(j, JIT_RAISE_STACK_OVERFLOW, pc_abs);
    emit_patch_i32(b, ja + 2, (int32_t)(emit_pos(b) - (ja + 6)));

    emit_mov_reg_reg(b, RDI, RBX);
    emit_alu_reg_imm32(b, ALU_ADD, RDI, nlocals * 16);
    if (rd == 0xFFF) {
        emit_mov_reg_reg(b, RSI, RBP);
        emit_alu_reg_imm32(b, ALU_ADD, RSI, FRAME_SCRATCH);
    } else {
        emit_mov_reg_reg(b, RSI, RBX);
        emit_alu_reg_imm32(b, ALU_ADD, RSI, rd * 16);
    }
    emit_mov_reg_reg(b, RDX, RAX);
    emit_sub_rsp_imm32(b, 8);
    int32_t rel = -(int32_t)(emit_pos(b) + 5);
    emit_call_rel32(b, rel);
    emit_add_rsp_imm32(b, 8);

    if (rd == 0xFFF)
        emit_alu_mem_imm32(b, ALU_CMP, RBP, FRAME_SCRATCH, TAG_ERROR);
    else
        emit_alu_mem_imm32(b, ALU_CMP, RBX, TAG_DISP(rd), TAG_ERROR);
    size_t jne = emit_pos(b);
    emit_jcc_rel32(b, CC_NE, 0);
    emit_mov_reg_imm32(b, RDI, pc_abs);
    emit_sub_rsp_imm32(b, 8);
    emit_call_abs(j, (void *)kl_jit_self_error);
    emit_add_rsp_imm32(b, 8);
    emit_jump(j, -1, JIT_ERR_TARGET);
    emit_patch_i32(b, jne + 2, (int32_t)(emit_pos(b) - (jne + 6)));
}

/* The direct call writes CallFrame fields with 8-byte stores: pc and nlocals as
 * one word, tailcalls together with the trailing padding. */
_Static_assert(offsetof(CallFrame, nlocals) == offsetof(CallFrame, pc) + 4, "CallFrame layout");
_Static_assert(offsetof(CallFrame, tailcalls) + 8 <= sizeof(CallFrame), "CallFrame layout");

/* JIT -> JIT direct call of a local koala function `co` (OP_CALL flag 0). When
 * co is compiled and a cached CallFrame is free, build and link the callee's
 * frame inline -- exactly what kl_call_code does: locals = the outgoing args
 * (rbx + nlocals, on the VM stack), ks->cf = the new frame, ks->stack_top above
 * its locals -- and call its native entry. Everything stays on the VM stack and
 * the frame chain, so a GC can enumerate it like interpreted frames. Otherwise
 * (callee not compiled yet, cache empty) fall back to kl_jit_call_helper.
 *   - stack checks as for direct self calls ("stack overflow");
 *   - a JIT_TAG_TAILCALL result: the callee tail-called another function in
 *     this frame, kl_jit_tail_continue runs it to the end;
 *   - then unlink the frame back to the cache, restore stack_top, and trace an
 *     error at the call site (kl_jit_self_error), as the helper path does. */
/* Bit t set: tag t (<= TAG_VAL_MAX) passes the native entry guard, as in
 * eval.c's jit_params_ok; tags above it are references / interface values. */
static uint64_t jit_tag_ok_mask(void)
{
    uint64_t mask = 0;
    for (int t = 0; t <= TAG_VAL_MAX; t++) {
        TValue v = { .tag = t };
        if (is_int(&v) || is_uint(&v) || is_bool(&v) || is_float(&v)) mask |= 1ull << t;
    }
    return mask;
}

static void emit_direct_call(JitCtx *j, int rd, int nargs, int nlocals, CodeObject *co,
                             int pc_abs)
{
    EmitBuf *b = &j->buf;
    int cn = co->cs.nlocals;
    Object *mod = co->owner;
    if (!IS_MODULE(mod)) mod = ((TypeObject *)mod)->module;

    /* entry guard (jit_params_ok): a nil / error / no-value argument takes the
     * slow path, which runs the callee in the interpreter */
    size_t guard_fail[256];
    uint64_t mask = jit_tag_ok_mask();
    for (int i = 0; i < nargs; i++) {
        emit_mov_reg_mem(b, RAX, RBX, TAG_DISP(nlocals + i));
        emit_alu_reg_imm32(b, ALU_CMP, RAX, TAG_VAL_MAX);
        size_t ok = emit_pos(b);
        emit_jcc_rel32(b, CC_A, 0);
        emit_mov_reg_reg(b, RCX, RAX);
        emit_mov_reg_imm64(b, RDX, mask);
        emit_shr_reg_cl(b, RDX);
        emit_alu_reg_imm32(b, ALU_AND, RDX, 1);
        guard_fail[i] = emit_pos(b);
        emit_jcc_rel32(b, CC_E, 0);
        emit_patch_i32(b, ok + 2, (int32_t)(emit_pos(b) - (ok + 6)));
    }

    /* rax = ks; the callee's registers and call area must fit the VM stack */
    emit_stack_slot(j, nlocals + cn + co->cs.max_call_args);
    emit_alu_reg_mem(b, ALU_CMP, RCX, RAX, (int32_t)offsetof(KoalaState, stack_limit));
    size_t jbe = emit_pos(b);
    emit_jcc_rel32(b, CC_BE, 0);
    emit_raise_at(j, JIT_RAISE_STACK_OVERFLOW, pc_abs);
    emit_patch_i32(b, jbe + 2, (int32_t)(emit_pos(b) - (jbe + 6)));
    emit_alu_reg_mem(b, ALU_CMP, RSP, RAX, (int32_t)offsetof(KoalaState, c_stack_limit));
    size_t ja = emit_pos(b);
    emit_jcc_rel32(b, CC_A, 0);
    emit_raise_at(j, JIT_RAISE_STACK_OVERFLOW, pc_abs);
    emit_patch_i32(b, ja + 2, (int32_t)(emit_pos(b) - (ja + 6)));

    /* an empty frame cache is refilled out of line (deep recursion) */
    emit_mov_reg_mem(b, RCX, RAX, (int32_t)offsetof(KoalaState, cf_cache));
    emit_alu_reg_imm32(b, ALU_CMP, RCX, 0);
    size_t have = emit_pos(b);
    emit_jcc_rel32(b, CC_NE, 0);
    emit_mov_reg_reg(b, RDI, RAX);
    emit_sub_rsp_imm32(b, 8);
    emit_call_abs(j, (void *)kl_jit_refill_cf);
    emit_add_rsp_imm32(b, 8);
    emit_patch_i32(b, have + 2, (int32_t)(emit_pos(b) - (have + 6)));

    /* r10 = co, r11 = co->jit->fn (else slow) */
    emit_mov_reg_imm64(b, R10, (uint64_t)(uintptr_t)co);
    emit_mov_reg_mem(b, R11, R10, (int32_t)offsetof(CodeObject, jit));
    emit_alu_reg_imm32(b, ALU_CMP, R11, 0);
    size_t slow1 = emit_pos(b);
    emit_jcc_rel32(b, CC_E, 0);
    emit_mov_reg_mem(b, R11, R11, (int32_t)offsetof(JitEntry, fn));

    /* rdx = ks, rcx = a cached frame, popped off the cache */
    emit_mov_reg_reg(b, RDX, RAX);
    emit_mov_reg_mem(b, RCX, RDX, (int32_t)offsetof(KoalaState, cf_cache));
    emit_mov_reg_mem(b, RSI, RCX, (int32_t)offsetof(CallFrame, back));
    emit_mov_mem_reg(b, RDX, (int32_t)offsetof(KoalaState, cf_cache), RSI);

    /* fill the frame as _new_frame does, link it as ks->cf */
    emit_mov_mem_reg(b, RCX, (int32_t)offsetof(CallFrame, code), R10);
    emit_mov_reg_imm64(b, RSI, (uint64_t)(uintptr_t)mod);
    emit_mov_mem_reg(b, RCX, (int32_t)offsetof(CallFrame, module), RSI);
    emit_mov_reg_imm64(b, RSI, (uint64_t)(uint32_t)cn << 32); /* pc = 0, nlocals */
    emit_mov_mem_reg(b, RCX, (int32_t)offsetof(CallFrame, pc), RSI);
    emit_mov_mem_imm32(b, RCX, (int32_t)offsetof(CallFrame, tailcalls), 0);
    emit_mov_mem_reg(b, RCX, (int32_t)offsetof(CallFrame, ks), RDX);
    emit_reg_addr(j, RDI, nlocals);
    emit_mov_mem_reg(b, RCX, (int32_t)offsetof(CallFrame, locals), RDI);
    emit_mov_reg_mem(b, RSI, RDX, (int32_t)offsetof(KoalaState, cf));
    emit_mov_mem_reg(b, RCX, (int32_t)offsetof(CallFrame, back), RSI);
    emit_mov_mem_reg(b, RDX, (int32_t)offsetof(KoalaState, cf), RCX);
    emit_reg_addr(j, RSI, nlocals + cn);
    emit_mov_mem_reg(b, RDX, (int32_t)offsetof(KoalaState, stack_top), RSI);

    /* co->jit->fn(args, out, ks). out must not alias the callee's frame (a
     * result register in the call area is an argument slot, which a pending tail
     * call's marker would overwrite): use the scratch slot, copied to rd after. */
    int direct_out = rd != 0xFFF && rd < nlocals;
    int out_base = direct_out ? RBX : RBP;
    int32_t out_disp = direct_out ? rd * 16 : FRAME_SCRATCH;
    emit_mov_reg_reg(b, RSI, out_base);
    emit_alu_reg_imm32(b, ALU_ADD, RSI, out_disp);
    emit_sub_rsp_imm32(b, 8);
    emit_call_reg(b, R11);
    emit_add_rsp_imm32(b, 8);

    /* a tail call to another function is pending in the callee's frame */
    emit_alu_mem_imm32(b, ALU_CMP, out_base, out_disp, JIT_TAG_TAILCALL);
    size_t jne_tc = emit_pos(b);
    emit_jcc_rel32(b, CC_NE, 0);
    emit_mov_reg_reg(b, RDI, out_base);
    emit_alu_reg_imm32(b, ALU_ADD, RDI, out_disp);
    emit_sub_rsp_imm32(b, 8);
    emit_call_abs(j, (void *)kl_jit_tail_continue);
    emit_add_rsp_imm32(b, 8);
    emit_patch_i32(b, jne_tc + 2, (int32_t)(emit_pos(b) - (jne_tc + 6)));

    /* unlink the frame (rcx) back to the cache, as _pop_frame does */
    emit_mov_reg_mem(b, RAX, RBP, FRAME_KS);
    emit_mov_reg_mem(b, RCX, RAX, (int32_t)offsetof(KoalaState, cf));
    emit_mov_reg_mem(b, RDX, RCX, (int32_t)offsetof(CallFrame, back));
    emit_mov_mem_reg(b, RAX, (int32_t)offsetof(KoalaState, cf), RDX);
    emit_mov_reg_mem(b, RDX, RAX, (int32_t)offsetof(KoalaState, cf_cache));
    emit_mov_mem_reg(b, RCX, (int32_t)offsetof(CallFrame, back), RDX);
    emit_mov_mem_reg(b, RAX, (int32_t)offsetof(KoalaState, cf_cache), RCX);
    emit_reg_addr(j, RDX, nlocals);
    emit_mov_mem_reg(b, RAX, (int32_t)offsetof(KoalaState, stack_top), RDX);

    emit_alu_mem_imm32(b, ALU_CMP, out_base, out_disp, TAG_ERROR);
    size_t jne = emit_pos(b);
    emit_jcc_rel32(b, CC_NE, 0);
    emit_mov_reg_imm32(b, RDI, pc_abs);
    emit_sub_rsp_imm32(b, 8);
    emit_call_abs(j, (void *)kl_jit_self_error);
    emit_add_rsp_imm32(b, 8);
    emit_jump(j, -1, JIT_ERR_TARGET);
    emit_patch_i32(b, jne + 2, (int32_t)(emit_pos(b) - (jne + 6)));
    if (rd != 0xFFF && !direct_out) {
        emit_mov_reg_mem(b, RAX, RBP, FRAME_SCRATCH);
        emit_mov_reg_mem(b, RDX, RBP, FRAME_SCRATCH + 8);
        emit_store_ret(j, rd);
    }
    size_t jdone = emit_pos(b);
    emit_jmp_rel32(b, 0);

    /* slow path: through kl_call_code */
    emit_patch_i32(b, slow1 + 2, (int32_t)(emit_pos(b) - (slow1 + 6)));
    for (int i = 0; i < nargs; i++)
        emit_patch_i32(b, guard_fail[i] + 2, (int32_t)(emit_pos(b) - (guard_fail[i] + 6)));
    emit_helper_call(j, rd, nargs, nlocals, (Object *)co, pc_abs);
    emit_patch_i32(b, jdone + 1, (int32_t)(emit_pos(b) - (jdone + 5)));
}

/* ---- opcode classification ---- */

/* Signed integer comparison opcode -> condition code (-1 if not a compare). */
static int cmp_cc(OpCode op)
{
    switch (op) {
        case OP_INT_EQ:
        case OP_INT_EQ_IMM:
        case OP_JMP_INT_EQ:
        case OP_JMP_INT_EQ_IMM:
            return CC_E;
        case OP_INT_NE:
        case OP_INT_NE_IMM:
        case OP_JMP_INT_NE:
        case OP_JMP_INT_NE_IMM:
            return CC_NE;
        case OP_INT_LT:
        case OP_INT_LT_IMM:
        case OP_JMP_INT_LT:
        case OP_JMP_INT_LT_IMM:
            return CC_L;
        case OP_INT_LE:
        case OP_INT_LE_IMM:
        case OP_JMP_INT_LE:
        case OP_JMP_INT_LE_IMM:
            return CC_LE;
        case OP_INT_GT:
        case OP_INT_GT_IMM:
        case OP_JMP_INT_GT:
        case OP_JMP_INT_GT_IMM:
            return CC_G;
        case OP_INT_GE:
        case OP_INT_GE_IMM:
        case OP_JMP_INT_GE:
        case OP_JMP_INT_GE_IMM:
            return CC_GE;
        /* unsigned comparisons (uint64 family); EQ/NE reg-form reuse OP_INT_EQ/NE */
        case OP_UINT_EQ_IMM:
            return CC_E;
        case OP_UINT_NE_IMM:
            return CC_NE;
        case OP_UINT_LT:
        case OP_UINT_LT_IMM:
        case OP_JMP_UINT_LT:
        case OP_JMP_UINT_LT_IMM:
            return CC_B;
        case OP_UINT_LE:
        case OP_UINT_LE_IMM:
        case OP_JMP_UINT_LE:
        case OP_JMP_UINT_LE_IMM:
            return CC_BE;
        case OP_UINT_GT:
        case OP_UINT_GT_IMM:
        case OP_JMP_UINT_GT:
        case OP_JMP_UINT_GT_IMM:
            return CC_A;
        case OP_UINT_GE:
        case OP_UINT_GE_IMM:
        case OP_JMP_UINT_GE:
        case OP_JMP_UINT_GE_IMM:
            return CC_AE;
        default:
            return -1;
    }
}

/* Integer arithmetic opcode -> ALU code (-1 if not arithmetic). */
static int arith_alu(OpCode op)
{
    switch (op) {
        case OP_INT_ADD:
        case OP_INT_ADD_IMM:
            return ALU_ADD;
        case OP_INT_SUB:
        case OP_INT_SUB_IMM:
            return ALU_SUB;
        case OP_INT_MUL:
        case OP_INT_MUL_IMM:
            return ALU_MUL;
        case OP_INT_AND:
        case OP_INT_AND_IMM:
            return ALU_AND;
        case OP_INT_OR:
        case OP_INT_OR_IMM:
            return ALU_OR;
        case OP_INT_XOR:
        case OP_INT_XOR_IMM:
            return ALU_XOR;
        /* uint64 family: identical payload ops, result tag differs (see is_unsigned_op) */
        case OP_UINT_ADD:
        case OP_UINT_ADD_IMM:
            return ALU_ADD;
        case OP_UINT_SUB:
        case OP_UINT_SUB_IMM:
            return ALU_SUB;
        case OP_UINT_MUL:
        case OP_UINT_MUL_IMM:
            return ALU_MUL;
        case OP_UINT_AND:
        case OP_UINT_AND_IMM:
            return ALU_AND;
        case OP_UINT_OR:
        case OP_UINT_OR_IMM:
            return ALU_OR;
        case OP_UINT_XOR:
        case OP_UINT_XOR_IMM:
            return ALU_XOR;
        default:
            return -1;
    }
}

/* True for the uint64-family opcodes reaching the default branch: their immediate
 * operands are zero-extended (I_VAL, not I_SVAL) and their result tag is UINT64. */
static int is_unsigned_op(OpCode op)
{
    switch (op) {
        case OP_UINT_ADD:
        case OP_UINT_ADD_IMM:
        case OP_UINT_SUB:
        case OP_UINT_SUB_IMM:
        case OP_UINT_MUL:
        case OP_UINT_MUL_IMM:
        case OP_UINT_AND:
        case OP_UINT_AND_IMM:
        case OP_UINT_OR:
        case OP_UINT_OR_IMM:
        case OP_UINT_XOR:
        case OP_UINT_XOR_IMM:
        case OP_UINT_EQ_IMM:
        case OP_UINT_NE_IMM:
        case OP_UINT_LT:
        case OP_UINT_LT_IMM:
        case OP_UINT_LE:
        case OP_UINT_LE_IMM:
        case OP_UINT_GT:
        case OP_UINT_GT_IMM:
        case OP_UINT_GE:
        case OP_UINT_GE_IMM:
        case OP_JMP_UINT_LT:
        case OP_JMP_UINT_LT_IMM:
        case OP_JMP_UINT_LE:
        case OP_JMP_UINT_LE_IMM:
        case OP_JMP_UINT_GT:
        case OP_JMP_UINT_GT_IMM:
        case OP_JMP_UINT_GE:
        case OP_JMP_UINT_GE_IMM:
            return 1;
        default:
            return 0;
    }
}

/* True for the *_IMM forms (immediate operand in the low 8 bits). */
static int is_imm_op(OpCode op)
{
    switch (op) {
        case OP_INT_ADD_IMM:
        case OP_INT_SUB_IMM:
        case OP_INT_MUL_IMM:
        case OP_INT_DIV_IMM:
        case OP_INT_MOD_IMM:
        case OP_INT_AND_IMM:
        case OP_INT_OR_IMM:
        case OP_INT_XOR_IMM:
        case OP_INT_SHL_IMM:
        case OP_INT_SHR_IMM:
        case OP_INT_EQ_IMM:
        case OP_INT_NE_IMM:
        case OP_INT_LT_IMM:
        case OP_INT_LE_IMM:
        case OP_INT_GT_IMM:
        case OP_INT_GE_IMM:
        case OP_JMP_INT_EQ_IMM:
        case OP_JMP_INT_NE_IMM:
        case OP_JMP_INT_LT_IMM:
        case OP_JMP_INT_LE_IMM:
        case OP_JMP_INT_GT_IMM:
        case OP_JMP_INT_GE_IMM:
            return 1;
        case OP_UINT_ADD_IMM:
        case OP_UINT_SUB_IMM:
        case OP_UINT_MUL_IMM:
        case OP_UINT_DIV_IMM:
        case OP_UINT_MOD_IMM:
        case OP_UINT_AND_IMM:
        case OP_UINT_OR_IMM:
        case OP_UINT_XOR_IMM:
        case OP_UINT_SHL_IMM:
        case OP_UINT_SHR_IMM:
        case OP_UINT_EQ_IMM:
        case OP_UINT_NE_IMM:
        case OP_UINT_LT_IMM:
        case OP_UINT_LE_IMM:
        case OP_UINT_GT_IMM:
        case OP_UINT_GE_IMM:
        case OP_JMP_UINT_LT_IMM:
        case OP_JMP_UINT_LE_IMM:
        case OP_JMP_UINT_GT_IMM:
        case OP_JMP_UINT_GE_IMM:
            return 1;
        default:
            return 0;
    }
}

/* True for the fused compare-and-branch opcodes (OP_JMP_INT_*). */
static int is_jmp_op(OpCode op)
{
    switch (op) {
        case OP_JMP_INT_EQ:
        case OP_JMP_INT_NE:
        case OP_JMP_INT_LT:
        case OP_JMP_INT_LE:
        case OP_JMP_INT_GT:
        case OP_JMP_INT_GE:
        case OP_JMP_INT_EQ_IMM:
        case OP_JMP_INT_NE_IMM:
        case OP_JMP_INT_LT_IMM:
        case OP_JMP_INT_LE_IMM:
        case OP_JMP_INT_GT_IMM:
        case OP_JMP_INT_GE_IMM:
            return 1;
        case OP_JMP_UINT_LT:
        case OP_JMP_UINT_LE:
        case OP_JMP_UINT_GT:
        case OP_JMP_UINT_GE:
        case OP_JMP_UINT_LT_IMM:
        case OP_JMP_UINT_LE_IMM:
        case OP_JMP_UINT_GT_IMM:
        case OP_JMP_UINT_GE_IMM:
            return 1;
        default:
            return 0;
    }
}

/* ---- per-instruction translation ---- */

/* Returns the number of bytecode words consumed (1, or 2 for OP_CALL+payload).
 * self_only: every call in this function targets itself, so OP_CALL lowers to a
 * direct native self-call instead of the runtime helper. */
static int translate_insn(JitCtx *j, uint32_t insn, int pc, ModuleObject *m, CodeObject *code,
                          int self_only)
{
    EmitBuf *b = &j->buf;
    OpCode op = (OpCode)I_OP(insn);
    int rd, rs, rt, imm, ti, off;

    switch (op) {
        case OP_MOVE:
            rd = I_VAL(insn, 12, 12);
            rs = I_VAL(insn, 0, 12);
            emit_mov_reg_mem(b, RAX, RBX, TAG_DISP(rs));
            rc_load_gpr(j, RDX, rs, -1);
            emit_mov_mem_reg(b, RBX, TAG_DISP(rd), RAX);
            emit_mov_mem_reg(b, RBX, VAL_DISP(rd), RDX);
            rc_def(j, rd, RDX, 0, -1);
            break;

        case OP_LOAD_INT_IMM:
            rd = I_VAL(insn, 16, 8);
            ti = I_VAL(insn, 12, 4);
            imm = sval(I_VAL(insn, 0, 12), 12);
            emit_mov_mem_imm32(b, RBX, TAG_DISP(rd), ti);
            emit_mov_mem_imm32(b, RBX, VAL_DISP(rd), imm);
            rc_def_imm(j, rd, imm, 0);
            break;

        case OP_LOAD_UINT_IMM:
            rd = I_VAL(insn, 16, 8);
            ti = I_VAL(insn, 12, 4);
            imm = (int)I_VAL(insn, 0, 12);
            emit_mov_mem_imm32(b, RBX, TAG_DISP(rd), ti);
            emit_mov_mem_imm32(b, RBX, VAL_DISP(rd), imm);
            rc_def_imm(j, rd, imm, 0);
            break;

        case OP_LOADK: {
            /* Snapshot the constant pool slot at compile time (constants are
             * immutable for the module lifetime); embed tag + payload bits. */
            rd = I_VAL(insn, 16, 8);
            int idx = I_VAL(insn, 0, 16);
            TValue *pool = VECTOR_ITEMS(&m->const_pool, TValue);
            TValue cv = pool[idx];
            emit_mov_reg_imm64(b, RAX, (uint64_t)cv.ival);
            emit_mov_mem_reg(b, RBX, VAL_DISP(rd), RAX);
            emit_store_tag(j, rd, (int32_t)cv.tag);
            rc_def(j, rd, RAX, 0, cv.tag == TAG_FLOAT64 ? 1 : 0);
            break;
        }

        case OP_LOAD_TAG: {
            rd = I_VAL(insn, 8, 12);
            int tag = I_VAL(insn, 0, 8);
            TValue cv = kl_jit_tag_value(tag);
            emit_mov_reg_imm64(b, RAX, (uint64_t)cv.ival);
            emit_mov_mem_reg(b, RBX, VAL_DISP(rd), RAX);
            emit_store_tag(j, rd, (int32_t)cv.tag);
            rc_def(j, rd, RAX, 0, -1);
            break;
        }

        case OP_INT_NEG:
            rd = I_VAL(insn, 12, 12);
            rs = I_VAL(insn, 0, 12);
            {
                int d = rc_dst(j, rd, -1, 0);
                rc_load_gpr(j, d, rs, 0);
                emit_neg_reg(b, d);
                emit_store_reg_tagged(j, rd, d, TAG_INT64);
            }
            break;

        case OP_INT_NOT: {
            /* '~' keeps uint64 and gives int64 otherwise (a widened narrow int) */
            rd = I_VAL(insn, 12, 12);
            rs = I_VAL(insn, 0, 12);
            emit_mov_reg_mem(b, RCX, RBX, TAG_DISP(rs));
            rc_load_gpr(j, RAX, rs, 0);
            emit_not_reg(b, RAX);
            emit_store_int64(j, rd);
            rc_def(j, rd, RAX, 0, 0);
            emit_alu_reg_imm32(b, ALU_CMP, RCX, TAG_UINT64);
            size_t jne = emit_pos(b);
            emit_jcc_rel32(b, CC_NE, 0);
            emit_store_tag(j, rd, TAG_UINT64);
            emit_patch_i32(b, jne + 2, (int32_t)(emit_pos(b) - (jne + 6)));
            break;
        }

        case OP_INT_SHL_IMM:
        case OP_INT_SHR_IMM:
            rd = I_VAL(insn, 16, 8);
            rs = I_VAL(insn, 8, 8);
            imm = sval(I_VAL(insn, 0, 8), 8);
            {
                int d = rc_dst(j, rd, -1, 0);
                rc_load_gpr(j, d, rs, 0);
                if (op == OP_INT_SHL_IMM)
                    emit_shl_reg_imm8(b, d, (uint8_t)imm);
                else
                    emit_sar_reg_imm8(b, d, (uint8_t)imm);
                emit_store_reg_tagged(j, rd, d, TAG_INT64);
            }
            break;

        case OP_UINT_SHL_IMM:
        case OP_UINT_SHR_IMM:
            rd = I_VAL(insn, 16, 8);
            rs = I_VAL(insn, 8, 8);
            imm = (int)I_VAL(insn, 0, 8);
            {
                int d = rc_dst(j, rd, -1, 0);
                rc_load_gpr(j, d, rs, 0);
                if (op == OP_UINT_SHL_IMM)
                    emit_shl_reg_imm8(b, d, (uint8_t)imm);
                else
                    emit_shr_reg_imm8(b, d, (uint8_t)imm);
                emit_store_reg_tagged(j, rd, d, TAG_UINT64);
            }
            break;

        case OP_INT_SHL:
        case OP_INT_SHR:
        case OP_UINT_SHL:
        case OP_UINT_SHR: {
            rd = I_VAL(insn, 16, 8);
            rs = I_VAL(insn, 8, 8);
            rt = I_VAL(insn, 0, 8);
            int kind = (op == OP_INT_SHR) ? 1 : (op == OP_UINT_SHR) ? 2 : 0;
            int32_t stag =
                (op == OP_UINT_SHL || op == OP_UINT_SHR) ? TAG_UINT64 : TAG_INT64;
            emit_reg_shift(j, rd, rs, rt, kind, stag, code->cs.start_pc + pc);
            break;
        }

        case OP_INT_DIV:
        case OP_INT_MOD:
            rd = I_VAL(insn, 16, 8);
            rs = I_VAL(insn, 8, 8);
            rt = I_VAL(insn, 0, 8);
            emit_div_rrr(j, rd, rs, rt, op == OP_INT_MOD, 0, code->cs.start_pc + pc);
            break;

        case OP_INT_DIV_IMM:
        case OP_INT_MOD_IMM:
            rd = I_VAL(insn, 16, 8);
            rs = I_VAL(insn, 8, 8);
            imm = sval(I_VAL(insn, 0, 8), 8);
            if (imm == 0 || imm == -1) goto unsupported; // isel keeps these in a register
            emit_div_rrimm(j, rd, rs, imm, op == OP_INT_MOD_IMM, 0);
            break;

        case OP_UINT_DIV:
        case OP_UINT_MOD:
            rd = I_VAL(insn, 16, 8);
            rs = I_VAL(insn, 8, 8);
            rt = I_VAL(insn, 0, 8);
            emit_div_rrr(j, rd, rs, rt, op == OP_UINT_MOD, 1, code->cs.start_pc + pc);
            break;

        case OP_UINT_DIV_IMM:
        case OP_UINT_MOD_IMM:
            rd = I_VAL(insn, 16, 8);
            rs = I_VAL(insn, 8, 8);
            imm = (int)I_VAL(insn, 0, 8);
            if (imm == 0) goto unsupported; // isel keeps a zero divisor in a register
            emit_div_rrimm(j, rd, rs, imm, op == OP_UINT_MOD_IMM, 1);
            break;

        case OP_INT_CAST: {
            rd = I_VAL(insn, 16, 8);
            rs = I_VAL(insn, 8, 8);
            int flag = I_VAL(insn, 0, 8);
            emit_cast_call(j, rd, rs, flag & 0x3, (flag >> 2) & 0x3F,
                           (void *)do_int_cast);
            break;
        }

        case OP_FLOAT_CAST:
        case OP_FLOAT_TO_INT:
        case OP_INT_TO_FLOAT: {
            rd = I_VAL(insn, 16, 8);
            rs = I_VAL(insn, 8, 8);
            int flag = I_VAL(insn, 0, 8);
            void *fn = op == OP_FLOAT_CAST ? (void *)do_float_cast
                     : op == OP_FLOAT_TO_INT ? (void *)do_float_to_int
                                             : (void *)do_int_to_float;
            emit_cast_call(j, rd, rs, flag & 0x3, (flag >> 2) & 0x3F, fn);
            break;
        }

        case OP_FLOAT_ADD:
        case OP_FLOAT_SUB:
        case OP_FLOAT_MUL:
        case OP_FLOAT_DIV: {
            rd = I_VAL(insn, 16, 8);
            rs = I_VAL(insn, 8, 8);
            rt = I_VAL(insn, 0, 8);
            int sop = op == OP_FLOAT_ADD ? SSE_ADD
                    : op == OP_FLOAT_SUB ? SSE_SUB
                    : op == OP_FLOAT_MUL ? SSE_MUL
                                         : SSE_DIV;
            emit_float_arith(j, sop, rd, rs, rt);
            break;
        }

        case OP_FLOAT_MOD:
            rd = I_VAL(insn, 16, 8);
            rs = I_VAL(insn, 8, 8);
            rt = I_VAL(insn, 0, 8);
            emit_float_mod(j, rd, rs, rt);
            break;

        case OP_FLOAT_NEG:
            rd = I_VAL(insn, 12, 12);
            rs = I_VAL(insn, 0, 12);
            emit_float_neg(j, rd, rs);
            break;

        case OP_FLOAT_EQ:
        case OP_FLOAT_NE:
        case OP_FLOAT_LT:
        case OP_FLOAT_LE:
        case OP_FLOAT_GT:
        case OP_FLOAT_GE:
            rd = I_VAL(insn, 16, 8);
            rs = I_VAL(insn, 8, 8);
            rt = I_VAL(insn, 0, 8);
            emit_float_cmp_bool(j, rd, rs, rt, float_cmp_op(op));
            break;

        case OP_JMP_FLOAT_EQ:
        case OP_JMP_FLOAT_NE:
        case OP_JMP_FLOAT_LT:
        case OP_JMP_FLOAT_LE:
        case OP_JMP_FLOAT_GT:
        case OP_JMP_FLOAT_GE:
            rs = I_VAL(insn, 16, 8);
            rt = I_VAL(insn, 8, 8);
            off = sval(I_VAL(insn, 0, 8), 8);
            emit_float_jmp_cmp(j, rs, rt, float_cmp_op(op), pc + 1 + off);
            break;

        case OP_LAND:
        case OP_LOR:
            /* bool payload is 0/1, so logical and/or == bitwise and/or */
            rd = I_VAL(insn, 16, 8);
            rs = I_VAL(insn, 8, 8);
            rt = I_VAL(insn, 0, 8);
            {
                int d = rc_dst(j, rd, rt, 0);
                rc_load_gpr(j, d, rs, 0);
                rc_alu(j, op == OP_LAND ? ALU_AND : ALU_OR, d, rt);
                emit_store_reg_tagged(j, rd, d, TAG_BOOL);
            }
            break;

        case OP_LNOT:
            rd = I_VAL(insn, 12, 12);
            rs = I_VAL(insn, 0, 12);
            {
                int d = rc_dst(j, rd, -1, 0);
                rc_load_gpr(j, d, rs, 0);
                emit_alu_reg_imm32(b, ALU_XOR, d, 1);
                emit_store_reg_tagged(j, rd, d, TAG_BOOL);
            }
            break;

        case OP_RET:
            rs = I_VAL(insn, 0, 12);
            emit_load_out(j);
            emit_mov_reg_mem(b, RAX, RBX, TAG_DISP(rs));
            rc_load_gpr(j, RDX, rs, -1);
            emit_mov_mem_reg(b, RCX, 0, RAX);
            emit_mov_mem_reg(b, RCX, 8, RDX);
            emit_epilogue(j);
            break;

        case OP_RET_INT_IMM:
            ti = I_VAL(insn, 16, 8);
            imm = sval(I_VAL(insn, 0, 16), 16);
            emit_load_out(j);
            emit_mov_mem_imm32(b, RCX, 0, ti);
            emit_mov_mem_imm32(b, RCX, 8, imm);
            emit_epilogue(j);
            break;

        case OP_RET_UINT_IMM:
            ti = I_VAL(insn, 16, 8);
            imm = (int)I_VAL(insn, 0, 16);
            emit_load_out(j);
            emit_mov_mem_imm32(b, RCX, 0, ti);
            emit_mov_mem_imm32(b, RCX, 8, imm);
            emit_epilogue(j);
            break;

        case OP_RET_TAG: {
            int tag = I_VAL(insn, 0, 8);
            TValue cv = kl_jit_tag_value(tag);
            emit_load_out(j);
            emit_mov_mem_imm32(b, RCX, 0, (int32_t)cv.tag);
            emit_mov_reg_imm64(b, RAX, (uint64_t)cv.ival);
            emit_mov_mem_reg(b, RCX, 8, RAX);
            emit_epilogue(j);
            break;
        }

        case OP_RET_CONST: {
            int idx = I_VAL(insn, 0, 16);
            TValue *pool = VECTOR_ITEMS(&m->const_pool, TValue);
            TValue cv = pool[idx];
            emit_load_out(j);
            emit_mov_mem_imm32(b, RCX, 0, (int32_t)cv.tag);
            emit_mov_reg_imm64(b, RAX, (uint64_t)cv.ival);
            emit_mov_mem_reg(b, RCX, 8, RAX);
            emit_epilogue(j);
            break;
        }

        case OP_RET_VOID:
            emit_load_out(j);
            emit_mov_mem_imm32(b, RCX, 0, TAG_NIL);
            emit_mov_mem_imm32(b, RCX, 8, 0);
            emit_epilogue(j);
            break;

        case OP_JMP:
            off = sval(I_VAL(insn, 0, 16), 16);
            emit_jump(j, -1, pc + 1 + off);
            break;

        case OP_JMP_TRUE:
        case OP_JMP_FALSE:
            rd = I_VAL(insn, 16, 8);
            off = sval(I_VAL(insn, 0, 16), 16);
            {
                int p = rc_gpr(j, rd);
                if (p >= 0) emit_alu_reg_imm32(b, ALU_CMP, p, 0);
                else emit_alu_mem_imm32(b, ALU_CMP, RBX, VAL_DISP(rd), 0);
            }
            emit_jump(j, op == OP_JMP_TRUE ? CC_NE : CC_E, pc + 1 + off);
            break;

        case OP_CALL: {
            /* Two words: | op:8 | flag:4 | rd:12 | nargs:8 | + payload.
             * flag=0: entry index, a local CodeObject or imported CFuncObject,
             * called through the runtime helper; flag=1: trait method index of
             * the receiver's itable; flag=2: register holding a function value.
             * Flags 1/2 resolve at run time (kl_jit_call_dyn). */
            int flg = I_VAL(insn, 20, 4);
            rd = I_VAL(insn, 8, 12);
            int nargs = I_VAL(insn, 0, 8);
            uint32_t payload = m->codes[code->cs.start_pc + pc + 1];
            if (flg == 1 || flg == 2) {
                emit_dyn_call(j, rd, nargs, code->cs.nlocals, flg == 2 ? (int)payload : -1,
                              (int)payload, code->cs.start_pc + pc);
                return 2;
            }
            if (flg != 0) {
                j->failed = 1;
                j->fail_pc = pc;
                j->fail_insn = insn;
                return 2;
            }
            if (self_only) {
                emit_self_call(j, rd, code->cs.nlocals, code->cs.max_call_args,
                               code->cs.start_pc + pc);
                return 2;
            }
            uint32_t *insns = m->codes + code->cs.start_pc;
            int32_t entry_index = (int32_t)insns[pc + 1];
            Object **entry_table = VECTOR_ITEMS(&m->funcs, Object *);
            Object *fn = entry_table[entry_index];
            if (IS_CODE(fn))
                emit_direct_call(j, rd, nargs, code->cs.nlocals, (CodeObject *)fn,
                                 code->cs.start_pc + pc);
            else
                emit_helper_call(j, rd, nargs, code->cs.nlocals, fn,
                                 code->cs.start_pc + pc);
            return 2; /* consumed the payload word as well */
        }

        case OP_TAIL_CALL:
            /* Self tail recursion (single word, no payload): the new args are
             * already in the register file and the frame is reused, so jump back
             * to the body start (bytecode pc 0, after the prologue). Terminator,
             * no fallthrough — mirrors the interpreter resetting pc to start_pc.
             * A tail call to another function (flag 8|x, payload as OP_CALL's)
             * goes through kl_jit_tail_call and returns (see emit_tail_call). */
            if (I_VAL(insn, 20, 4) != 0) {
                int flg = I_VAL(insn, 20, 4) & 7;
                uint32_t payload = m->codes[code->cs.start_pc + pc + 1];
                Object *fn = NULL;
                if (flg == 0) {
                    Object **entry_table = VECTOR_ITEMS(&m->funcs, Object *);
                    fn = entry_table[(int32_t)payload];
                } else if (flg != 1 && flg != 2) {
                    j->failed = 1;
                    j->fail_pc = pc;
                    j->fail_insn = insn;
                    return 2;
                }
                emit_tail_call(j, I_VAL(insn, 0, 8), code->cs.nlocals, fn,
                               flg == 2 ? (int)payload : -1, flg == 1 ? (int)payload : 0,
                               code->cs.start_pc + pc);
                return 2;
            }
            emit_jump(j, -1, 0);
            break;

        case OP_GET_FIELD:
            rd = I_VAL(insn, 16, 8);
            rs = I_VAL(insn, 8, 8);
            off = I_VAL(insn, 0, 8);
            emit_get_field(j, rd, rs, off);
            break;

        case OP_SET_FIELD:
            rd = I_VAL(insn, 16, 8);
            rs = I_VAL(insn, 8, 8);
            off = I_VAL(insn, 0, 8);
            emit_set_field(j, rd, rs, off);
            break;

        case OP_MAKE_INTF:
            rd = I_VAL(insn, 16, 8);
            rs = I_VAL(insn, 8, 8);
            emit_reg_addr(j, RDI, rd);
            emit_reg_addr(j, RSI, rs);
            emit_mov_reg_imm32(b, RDX, I_VAL(insn, 0, 8));
            emit_sub_rsp_imm32(b, 8);
            emit_call_abs(j, (void *)kl_jit_make_intf);
            emit_add_rsp_imm32(b, 8);
            break;

        case OP_UPCAST_INTF:
            /* regs[rd] = { itab->parents[idx], regs[rs].obj } */
            rd = I_VAL(insn, 16, 8);
            rs = I_VAL(insn, 8, 8);
            emit_mov_reg_mem(b, RAX, RBX, TAG_DISP(rs));
            emit_mov_reg_mem(b, RAX, RAX, (int32_t)offsetof(IntfTable, parents));
            emit_mov_reg_mem(b, RAX, RAX, I_VAL(insn, 0, 8) * 8);
            emit_mov_reg_mem(b, RDX, RBX, VAL_DISP(rs));
            emit_mov_mem_reg(b, RBX, TAG_DISP(rd), RAX);
            emit_mov_mem_reg(b, RBX, VAL_DISP(rd), RDX);
            break;

        case OP_NIL_CHECK: {
            rd = I_VAL(insn, 12, 12);
            rs = I_VAL(insn, 0, 12);
            emit_alu_mem_imm32(b, ALU_CMP, RBX, TAG_DISP(rs), TAG_NIL);
            size_t jne = emit_pos(b);
            emit_jcc_rel32(b, CC_NE, 0);
            emit_raise_at(j, JIT_RAISE_NIL_UNWRAP, code->cs.start_pc + pc);
            emit_patch_i32(b, jne + 2, (int32_t)(emit_pos(b) - (jne + 6)));
            if (rd != rs) {
                emit_mov_reg_mem(b, RAX, RBX, TAG_DISP(rs));
                emit_mov_reg_mem(b, RDX, RBX, VAL_DISP(rs));
                emit_mov_mem_reg(b, RBX, TAG_DISP(rd), RAX);
                emit_mov_mem_reg(b, RBX, VAL_DISP(rd), RDX);
            }
            break;
        }

        case OP_LOAD_FUNC: {
            /* the function table is resolved when the module is loaded */
            rd = I_VAL(insn, 16, 8);
            int idx = I_VAL(insn, 0, 16);
            if (idx >= vector_size(&m->funcs)) goto unsupported;
            Object *fn = VECTOR_ITEMS(&m->funcs, Object *)[idx];
            if (!fn) goto unsupported;
            emit_store_tag(j, rd, TAG_OBJECT);
            emit_mov_reg_imm64(b, RAX, (uint64_t)(uintptr_t)fn);
            emit_mov_mem_reg(b, RBX, VAL_DISP(rd), RAX);
            break;
        }

        case OP_IS:
        case OP_AS: {
            rd = I_VAL(insn, 16, 8);
            rs = I_VAL(insn, 8, 8);
            TypeObject **types = VECTOR_ITEMS(&m->types, TypeObject *);
            emit_reg_addr(j, RDI, rs);
            emit_mov_reg_imm64(b, RSI, (uint64_t)(uintptr_t)types[I_VAL(insn, 0, 8)]);
            emit_mov_reg_imm32(b, RDX, op == OP_AS);
            emit_sub_rsp_imm32(b, 8);
            emit_call_abs(j, (void *)kl_jit_is_as);
            emit_add_rsp_imm32(b, 8);
            emit_store_ret(j, rd);
            break;
        }

        case OP_NEW: {
            rd = I_VAL(insn, 16, 8);
            int idx = I_VAL(insn, 0, 16);
            TypeObject **types = VECTOR_ITEMS(&m->types, TypeObject *);
            emit_new(j, rd, types[idx]);
            break;
        }

        case OP_BUILD_INTERN:
            rd = I_VAL(insn, 16, 8);
            off = I_VAL(insn, 8, 8);   /* tag */
            imm = I_VAL(insn, 0, 8);   /* count */
            emit_build_intern(j, rd, off, imm);
            break;

        case OP_GLOBAL_GET:
        case OP_GLOBAL_SET: {
            rs = I_VAL(insn, 12, 12);
            int idx = I_VAL(insn, 0, 12);
            TValue *addr;
            if (idx < m->num_globals) {
                addr = &m->values[idx];
            } else {
                GlobalObject *g = (GlobalObject *)vector_get(&m->globals, idx);
                addr = &((ModuleObject *)g->module)->values[g->index];
            }
            if (op == OP_GLOBAL_GET) emit_global_get(j, rs, addr);
            else emit_global_set(j, rs, addr);
            break;
        }

        case OP_SEQ_GET:
            rd = I_VAL(insn, 16, 8);
            rs = I_VAL(insn, 8, 8);
            rt = I_VAL(insn, 0, 8);
            emit_slot3(j, rd, rs, rt, -1, 2, SLOT_GET_ITEM, code->cs.start_pc + pc, 1);
            break;

        case OP_SEQ_SET:
            rd = I_VAL(insn, 16, 8);
            rs = I_VAL(insn, 8, 8);
            rt = I_VAL(insn, 0, 8);
            emit_slot3(j, rd, rd, rt, rs, 3, SLOT_SET_ITEM, code->cs.start_pc + pc, 0);
            break;

        case OP_SEQ_GET_IMM:
            rd = I_VAL(insn, 16, 8);
            rs = I_VAL(insn, 8, 8);
            imm = sval(I_VAL(insn, 0, 8), 8);
            emit_get_item_imm(j, rd, rs, imm, code->cs.start_pc + pc);
            break;

        case OP_SEQ_SET_IMM:
            rd = I_VAL(insn, 16, 8);
            rs = I_VAL(insn, 8, 8);
            imm = sval(I_VAL(insn, 0, 8), 8);
            emit_set_item_imm(j, rd, rs, imm, code->cs.start_pc + pc);
            break;

        case OP_SEQ_GET_SLICE:
            rd = I_VAL(insn, 16, 8);
            rs = I_VAL(insn, 8, 8);
            rt = I_VAL(insn, 0, 8);
            emit_slot3(j, rd, rs, rt, -1, 2, SLOT_GET_SLICE, code->cs.start_pc + pc, 1);
            break;

        case OP_SEQ_SET_SLICE:
            rd = I_VAL(insn, 16, 8);
            rs = I_VAL(insn, 8, 8);
            rt = I_VAL(insn, 0, 8);
            emit_slot3(j, rd, rd, rt, rs, 3, SLOT_SET_SLICE, code->cs.start_pc + pc, 0);
            break;

        case OP_LEN:
            rd = I_VAL(insn, 12, 12);
            rs = I_VAL(insn, 0, 12);
            emit_slot3(j, rd, rs, -1, -1, 1, SLOT_LEN, code->cs.start_pc + pc, 1);
            break;

        case OP_CONTAINS:
            rd = I_VAL(insn, 16, 8);
            rs = I_VAL(insn, 8, 8);
            rt = I_VAL(insn, 0, 8);
            emit_slot3(j, rd, rs, rt, -1, 2, SLOT_CONTAINS, code->cs.start_pc + pc, 1);
            break;

        case OP_STR:
            rd = I_VAL(insn, 12, 12);
            rs = I_VAL(insn, 0, 12);
            emit_slot3(j, rd, rs, -1, -1, 1, SLOT_STR, code->cs.start_pc + pc, 1);
            break;

        case OP_HASH:
            rd = I_VAL(insn, 12, 12);
            rs = I_VAL(insn, 0, 12);
            emit_hash(j, rd, rs, code->cs.start_pc + pc);
            break;

        case OP_FMT:
            rs = I_VAL(insn, 12, 12);
            rt = I_VAL(insn, 0, 12);
            emit_fmt(j, rs, rt, code->cs.start_pc + pc);
            break;

        case OP_PRINT:
            imm = I_VAL(insn, 0, 8);
            emit_print(j, imm, code->cs.start_pc + pc);
            break;

        case OP_REF_EQ_NULL:
            rd = I_VAL(insn, 12, 12);
            rs = I_VAL(insn, 0, 12);
            emit_ref_null(j, rd, rs, CC_E);
            break;

        case OP_REF_NE_NULL:
            rd = I_VAL(insn, 12, 12);
            rs = I_VAL(insn, 0, 12);
            emit_ref_null(j, rd, rs, CC_NE);
            break;

        case OP_JMP_REF_EQ_NULL:
            rs = I_VAL(insn, 16, 8);
            off = sval(I_VAL(insn, 0, 16), 16);
            emit_jmp_ref_null(j, rs, CC_E, pc + 1 + off);
            break;

        case OP_JMP_REF_NE_NULL:
            rs = I_VAL(insn, 16, 8);
            off = sval(I_VAL(insn, 0, 16), 16);
            emit_jmp_ref_null(j, rs, CC_NE, pc + 1 + off);
            break;

        case OP_MOVE_TRUE:
            rd = I_VAL(insn, 16, 8);
            rs = I_VAL(insn, 8, 8);
            rt = I_VAL(insn, 0, 8);
            emit_move_true(j, rd, rs, rt);
            break;

        default: {
            int is_cmp;
            int slot = num_slot(op, &is_cmp);
            if (slot >= 0) {
                /* OP_NUM_*: FORMAT_RRR (rd@16, rs@8, rt@0), runtime dispatch. */
                rd = I_VAL(insn, 16, 8);
                rs = I_VAL(insn, 8, 8);
                rt = I_VAL(insn, 0, 8);
                emit_num_binop(j, rd, rs, rt, slot, is_cmp, code->cs.start_pc + pc);
                break;
            }
            int alu = arith_alu(op);
            int cc = cmp_cc(op);
            int uns = is_unsigned_op(op);
            int32_t rtag = uns ? TAG_UINT64 : TAG_INT64;
            if (alu >= 0) {
                /* FORMAT_RRR (rd@16, rs@8, rt@0) or RRImm (rd@16, rs@8, imm@0).
                 * uint immediates are zero-extended (I_VAL), int sign-extended. */
                rd = I_VAL(insn, 16, 8);
                rs = I_VAL(insn, 8, 8);
                if (is_imm_op(op)) {
                    imm = uns ? (int)I_VAL(insn, 0, 8) : sval(I_VAL(insn, 0, 8), 8);
                    emit_alu_rrimm(j, alu, rd, rs, imm, rtag);
                } else {
                    rt = I_VAL(insn, 0, 8);
                    emit_alu_rrr(j, alu, rd, rs, rt, rtag);
                }
            } else if (cc >= 0 && is_jmp_op(op)) {
                /* FORMAT_RROff (rs@16, rt@8, off@0) or RImmOff (rs@16, imm@8, off@0). */
                rs = I_VAL(insn, 16, 8);
                off = sval(I_VAL(insn, 0, 8), 8);
                if (is_imm_op(op)) {
                    imm = uns ? (int)I_VAL(insn, 8, 8) : sval(I_VAL(insn, 8, 8), 8);
                    emit_jmp_cmp_rrimm(j, cc, rs, imm, pc + 1 + off);
                } else {
                    rt = I_VAL(insn, 8, 8);
                    emit_jmp_cmp_rrr(j, cc, rs, rt, pc + 1 + off);
                }
            } else if (cc >= 0) {
                /* Comparison producing a BOOL: RRR (rd@16, rs@8, rt@0) or RRImm. */
                rd = I_VAL(insn, 16, 8);
                rs = I_VAL(insn, 8, 8);
                if (is_imm_op(op)) {
                    imm = uns ? (int)I_VAL(insn, 0, 8) : sval(I_VAL(insn, 0, 8), 8);
                    emit_int_cmp_rrimm(j, cc, rd, rs, imm);
                } else {
                    rt = I_VAL(insn, 0, 8);
                    emit_int_cmp_rrr(j, cc, rd, rs, rt);
                }
            } else {
                goto unsupported;
            }
            break;
        }
    }
    return 1;

unsupported:
    /* bail out, keep the interpreter */
    j->failed = 1;
    j->fail_pc = pc;
    j->fail_insn = insn;
    return 1;
}

/* True when every OP_CALL in the function targets the function itself (flag=0).
 * Pure self-recursion never enters kl_call_code, so ks->stack_top is never read
 * during the recursion and direct native self-calls are safe. Any non-self call
 * (or an unsupported flag) disqualifies the function: it then uses the runtime
 * helper for all calls, which keeps stack_top consistent. */
static int is_self_only(ModuleObject *m, CodeObject *code, const uint32_t *insns, int n)
{
    Object **entry_table = VECTOR_ITEMS(&m->funcs, Object *);
    for (int pc = 0; pc < n; pc++) {
        uint32_t insn = insns[pc];
        /* a tail call to another function rewrites the caller's frame, which a
           direct self call does not have */
        if (I_OP(insn) == OP_TAIL_CALL && I_VAL(insn, 20, 4) != 0) return 0;
        if (I_OP(insn) != OP_CALL) continue;
        if (I_VAL(insn, 20, 4) != 0) return 0;
        int32_t entry_index = (int32_t)insns[pc + 1];
        if (entry_table[entry_index] != (Object *)code) return 0;
        pc++; /* skip the payload word */
    }
    return 1;
}

/* ---- compilation entry point ---- */

/* Does op keep the register cache (DESIGN §15.3)? A cache-aware op reads its
 * operands through rc_* and refreshes the copy of what it writes, without any
 * call in between. Every other op runs with all copies dropped (it reads and
 * writes memory only, and may call helpers that clobber the pool registers). */
static int rc_aware(OpCode op, uint32_t insn)
{
    switch (op) {
        case OP_MOVE:
        case OP_LOAD_INT_IMM:
        case OP_LOAD_UINT_IMM:
        case OP_LOADK:
        case OP_LOAD_TAG:
        case OP_INT_NEG:
        case OP_INT_NOT:
        case OP_INT_SHL_IMM:
        case OP_INT_SHR_IMM:
        case OP_UINT_SHL_IMM:
        case OP_UINT_SHR_IMM:
        case OP_INT_DIV:
        case OP_INT_MOD:
        case OP_INT_DIV_IMM:
        case OP_INT_MOD_IMM:
        case OP_UINT_DIV:
        case OP_UINT_MOD:
        case OP_UINT_DIV_IMM:
        case OP_UINT_MOD_IMM:
        case OP_FLOAT_ADD:
        case OP_FLOAT_SUB:
        case OP_FLOAT_MUL:
        case OP_FLOAT_DIV:
        case OP_FLOAT_NEG:
        case OP_FLOAT_EQ:
        case OP_FLOAT_NE:
        case OP_FLOAT_LT:
        case OP_FLOAT_LE:
        case OP_FLOAT_GT:
        case OP_FLOAT_GE:
        case OP_JMP_FLOAT_EQ:
        case OP_JMP_FLOAT_NE:
        case OP_JMP_FLOAT_LT:
        case OP_JMP_FLOAT_LE:
        case OP_JMP_FLOAT_GT:
        case OP_JMP_FLOAT_GE:
        case OP_LAND:
        case OP_LOR:
        case OP_LNOT:
        case OP_RET:
        case OP_RET_INT_IMM:
        case OP_RET_UINT_IMM:
        case OP_RET_TAG:
        case OP_RET_CONST:
        case OP_RET_VOID:
        case OP_JMP:
        case OP_JMP_TRUE:
        case OP_JMP_FALSE:
        case OP_JMP_REF_EQ_NULL:
        case OP_JMP_REF_NE_NULL:
            return 1;
        case OP_TAIL_CALL:
            return I_VAL(insn, 20, 4) == 0; /* self: jump to pc 0 */
        default: {
            int is_cmp;
            if (num_slot(op, &is_cmp) >= 0) return 0;
            /* int/uint ALU (incl. _IMM), compares and fused compare-jumps */
            return arith_alu(op) >= 0 || cmp_cc(op) >= 0;
        }
    }
}

static int is_terminator(OpCode op)
{
    switch (op) {
        case OP_JMP:
        case OP_RET:
        case OP_RET_INT_IMM:
        case OP_RET_UINT_IMM:
        case OP_RET_TAG:
        case OP_RET_CONST:
        case OP_RET_VOID:
        case OP_TAIL_CALL:
            return 1;
        default:
            return 0;
    }
}

/* One emission pass over the body. Also the dataflow step: jumps record the
 * cache state on their edge (emit_jump), fallthroughs here; a jump target
 * starts from rc_in[pc], the meet computed by the previous pass. */
static void jit_emit_body(JitCtx *j, ModuleObject *m, CodeObject *code, uint32_t *insns,
                          int n, int self_only)
{
    j->buf.len = 0;
    j->nfixups = 0;
    j->failed = 0;
    for (int pc = 0; pc < n; pc++) {
        j->rc_meet[pc] = ~0u;
        j->rc_pred[pc] = 0;
        j->rc_jt_new[pc] = 0;
    }
    emit_prologue(j);
    rc_emit_preload(j);
    rc_edge(j, 0, j->rc_valid); /* entry */
    int prev_term = 0;
    for (int pc = 0; pc < n && !j->failed; pc++) {
        uint32_t insn = insns[pc];
        OpCode op = (OpCode)I_OP(insn);
        j->cur_pc = pc;
        if (pc > 0 && !prev_term) rc_edge(j, pc, j->rc_valid);
        if (j->rc_jt[pc])
            j->rc_valid = j->rc_in[pc];
        else if (pc > 0 && prev_term)
            j->rc_valid = 0; /* not reachable by fallthrough or jump */
        j->pc_map[pc] = emit_pos(&j->buf);
        int aware = rc_aware(op, insn);
        if (!aware) j->rc_valid = 0;
        pc += translate_insn(j, insn, pc, m, code, self_only) - 1;
        if (!aware) j->rc_valid = 0;
        prev_term = is_terminator(op);
    }
}

/* Choose the promoted VM registers from the pass-0 votes (DESIGN §15.4): each
 * use weighs 8^loop depth (depth = number of back edges spanning the pc, capped
 * at 3); a register is float-class when its float votes outweigh the int ones.
 * The best RC_NGPR int and RC_NXMM float registers with a use score >= 8 (one
 * use in a loop, or 8 straight-line uses) win. */
static void rc_select(JitCtx *j, int nregs)
{
    if (nregs <= 0) return;
    int64_t *score = calloc((size_t)nregs * 3, sizeof(int64_t)); /* use, int, float */
    for (int v = 0; v < j->nvotes; v++) {
        RcVote *vt = &j->votes[v];
        if (vt->vm < 0 || vt->vm >= nregs) continue;
        int depth = 0;
        for (int e = 0; e < j->nbacks; e += 2)
            if (j->backs[e] <= vt->pc && vt->pc <= j->backs[e + 1]) depth++;
        if (depth > 3) depth = 3;
        int64_t w = (int64_t)1 << (3 * depth);
        if (!vt->def) score[vt->vm * 3] += w;
        if (vt->cls == 0) score[vt->vm * 3 + 1] += w;
        if (vt->cls == 1) score[vt->vm * 3 + 2] += w;
    }
    int ng = 0, nx = 0;
    for (;;) {
        int best = -1, best_x = 0;
        for (int v = 0; v < nregs; v++) {
            if (score[v * 3] < 8) continue;
            int x = score[v * 3 + 2] > score[v * 3 + 1];
            if (x ? nx >= RC_NXMM : ng >= RC_NGPR) continue;
            if (best < 0 || score[v * 3] > score[best * 3]) {
                best = v;
                best_x = x;
            }
        }
        if (best < 0) break;
        RcSlot *r = &j->rc[j->rc_n++];
        r->vm = best;
        r->xmm = best_x;
        r->phys = best_x ? rc_xmm_pool[nx++] : rc_gpr_pool[ng++];
        score[best * 3] = 0;
    }
    free(score);
}

static void rc_print(JitCtx *j, const char *name, int passes)
{
    static const char *gpr[] = { "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
                                 "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15" };
    fprintf(stderr, "[jit] regcache @%s:", name);
    for (int s = 0; s < j->rc_n; s++) {
        if (j->rc[s].xmm)
            fprintf(stderr, " r%d=xmm%d", j->rc[s].vm, j->rc[s].phys);
        else
            fprintf(stderr, " r%d=%s", j->rc[s].vm, gpr[j->rc[s].phys]);
    }
    fprintf(stderr, " (%d passes)\n", passes);
}

JitEntry *kl_jit_compile(CodeObject *code)
{
    if (!kl_jit_enabled()) return NULL;

    const char *name = code->cs.name ? code->cs.name : "?";

    /* A free function's owner is its ModuleObject; a method's owner is its
     * TypeObject, and the method runs in the type's module (as _new_frame sets
     * cf->module): its bytecode and function table are that module's. */
    Object *owner = code->owner;
    if (owner && !IS_MODULE(owner) && IS_TYPE(owner, &type_type))
        owner = ((TypeObject *)owner)->module;
    if (!owner || !IS_MODULE(owner)) {
        jit_bail(name, -1, 0, "no module", 1);
        return NULL;
    }
    ModuleObject *m = (ModuleObject *)owner;

    /* Only the module's OWN functions: funcs[0..num_funcs). resolve_import
     * appends imported functions at index >= num_funcs, so they are filtered
     * here silently and fast-passed via jit_tried, neither compiling nor
     * cluttering the trace. */
    if (!kl_mo_is_local_func((Object *)m, (Object *)code)) {
        jit_bail(name, -1, 0, "not a local function", 1);
        return NULL;
    }

    if (!m->codes) {
        jit_bail(name, -1, 0, "module has no code", 0);
        return NULL;
    }

    int n = code->cs.num_insns;
    int start = code->cs.start_pc;
    if (n <= 0 || (uint32_t)start + (uint32_t)n > m->num_codes) {
        jit_bail(name, -1, 0, "empty code", 0);
        return NULL;
    }
    uint32_t *insns = m->codes + start;

    JitCtx j;
    memset(&j, 0, sizeof(j));
    j.num_insns = n;
    j.pc_map = calloc(n, sizeof(size_t));
    j.cap_fixups = n;
    j.fixups = calloc(n, sizeof(Fixup));
    emit_init(&j.buf);
    j.rc_in = calloc(n, sizeof(uint32_t));
    j.rc_meet = calloc(n, sizeof(uint32_t));
    j.rc_pred = calloc(n, 1);
    j.rc_jt = calloc(n, 1);
    j.rc_jt_new = calloc(n, 1);

    int self_only = is_self_only(m, code, insns, n);
    j.self_only = self_only;
    j.nlocals = code->cs.nlocals;

    /* Pass 0: no cache; collects jump targets, back edges and operand votes. */
    j.rc_collect = jit_regcache();
    jit_emit_body(&j, m, code, insns, n, self_only);
    j.rc_collect = 0;

    JitEntry *entry = NULL;
    if (j.failed) {
        jit_bail(name, j.fail_pc, j.fail_insn, "unsupported opcode", 0);
        goto done;
    }

    /* Register cache fixpoint (DESIGN §15.5): start optimistic (every copy
     * valid at every jump target) and re-emit until the entry states the pass
     * used equal the meets it observed. States only shrink, so this ends. */
    int rc_passes = 0;
    if (jit_regcache()) rc_select(&j, code->cs.nlocals > 0 ? code->cs.nlocals : 0);
    if (j.rc_n) {
        j.rc_check = jit_check_cache();
        memcpy(j.rc_jt, j.rc_jt_new, n);
        j.rc_preload = j.rc_jt[0];
        for (int pc = 0; pc < n; pc++) j.rc_in[pc] = RC_ALL(&j);
        int stable = 0;
        while (!stable && rc_passes < 64) {
            jit_emit_body(&j, m, code, insns, n, self_only);
            rc_passes++;
            if (j.failed) break;
            stable = 1;
            for (int pc = 0; pc < n; pc++) {
                if (!j.rc_jt[pc]) continue;
                uint32_t in = j.rc_pred[pc] ? j.rc_meet[pc] : 0;
                if (in != j.rc_in[pc]) {
                    j.rc_in[pc] = in;
                    stable = 0;
                }
            }
        }
        if (!stable || j.failed) {
            /* cannot happen (monotone), but never ship an unverified state */
            j.rc_n = 0;
            j.rc_preload = 0;
            memset(j.rc_jt, 0, n);
            jit_emit_body(&j, m, code, insns, n, self_only);
            if (j.failed) {
                jit_bail(name, j.fail_pc, j.fail_insn, "unsupported opcode", 0);
                goto done;
            }
        }
    }

    /* Error epilogue: entered from any helper call that returned TAG_ERROR.
     * Write error_value to *out and return, mirroring the interpreter's error
     * path (the frame's traceback entry was already recorded by the helper). */
    size_t err_off = emit_pos(&j.buf);
    emit_load_out(&j);
    emit_mov_mem_imm32(&j.buf, RCX, 0, TAG_ERROR);
    emit_mov_mem_imm32(&j.buf, RCX, 8, -1);
    emit_epilogue(&j);

    /* Patch jump displacements now that every pc has a native offset. */
    for (int i = 0; i < j.nfixups; i++) {
        int t = j.fixups[i].target_pc;
        size_t target_off;
        if (t == JIT_ERR_TARGET) {
            target_off = err_off;
        } else if (t >= 0 && t < n) {
            target_off = j.pc_map[t];
        } else {
            j.failed = 1;
            jit_bail(name, -1, 0, "jump target out of range", 0);
            goto done;
        }
        int32_t rel = (int32_t)(target_off - (j.fixups[i].patch_off + 4));
        emit_patch_i32(&j.buf, j.fixups[i].patch_off, rel);
    }

    size_t sz = j.buf.len;
    void *mem = jit_code_alloc(j.buf.data, sz);
    if (!mem) {
        jit_bail(name, -1, 0, "code arena allocation failed", 0);
        goto done;
    }

    entry = calloc(1, sizeof(JitEntry));
    entry->fn = (void (*)(TValue *, TValue *, struct _KoalaState *))mem;
    entry->mem = mem;
    entry->mem_size = sz;

    if (jit_stats()) {
        jit_stats_init();
        jit_stats_data.compiled++;
    }
    if (jit_trace() && j.rc_n) rc_print(&j, name, rc_passes);
    if (jit_trace()) jit_print_code(name, insns, n, j.pc_map, j.buf.data, j.buf.len, err_off, stderr);

done:
    free(j.pc_map);
    free(j.fixups);
    free(j.rc_in);
    free(j.rc_meet);
    free(j.rc_pred);
    free(j.rc_jt);
    free(j.rc_jt_new);
    free(j.votes);
    free(j.backs);
    emit_free(&j.buf);
    return entry;
}

void kl_jit_free(JitEntry *e)
{
    if (!e) return;
    /* The code stays in the arena (never reclaimed). */
    free(e);
}

#ifdef __cplusplus
}
#endif

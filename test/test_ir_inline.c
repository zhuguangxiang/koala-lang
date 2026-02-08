/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "atom.h"
#include "ir.h"
#include "log.h"
#include "passes.h"

#ifdef __cplusplus
extern "C" {
#endif

void klr_remove_load_pass(KlrFunc *func, void *ctx);
void klr_remove_store_pass(KlrFunc *func, void *ctx);
void klr_constant_folding_pass(KlrFunc *func, void *ctx);
void klr_constant_propagation_pass(KlrFunc *func, void *ctx);
void klr_remove_unused_pass(KlrFunc *func, void *ctx);
void klr_insn_remap(KlrFunc *func);
void klr_remove_only_jump_block(KlrFunc *func);
void klr_alloc_registers(KlrFunc *func);
void klr_simple_alloc_registers(KlrFunc *func);

static KlrFunc *foo;
static KlrFunc *bar;

/*
func foo(a int, b int) int {
    var c = 100
    c = a + b
    return c
}
*/
void build_foo(KlrModule *m)
{
    TypeSpec *param_types[] = {
        int64_type_spec(),
        int64_type_spec(),
        NULL,
    };

    KlrValue *func = klr_add_func(m, int64_type_spec(), param_types, "foo");
    foo = (KlrFunc *)func;
    KlrValue *pa = klr_get_param(func, 0);
    klr_set_name(pa, "a");

    KlrValue *pb = klr_get_param(func, 1);
    klr_set_name(pb, "b");

    KlrBasicBlock *bb = klr_append_block(func, "entry");
    KlrBuilder bldr;
    klr_builder_end(&bldr, bb);

    // var c = 100
    KlrValue *cvar = klr_add_local(&bldr, int64_type_spec(), "c");
    klr_build_store(&bldr, cvar, klr_const_int(100, int64_type_spec()));

    // c = a + b
    KlrValue *a = klr_build_load(&bldr, pa, "");
    KlrValue *b = klr_build_load(&bldr, pb, "");
    KlrValue *add = klr_build_add(&bldr, a, b, "add");
    klr_build_store(&bldr, cvar, add);

    // return c
    KlrValue *ret = klr_build_load(&bldr, cvar, "");
    klr_build_ret(&bldr, cvar);

    klr_print_func((KlrFunc *)func, stdout);
    // klr_print_cfg((KlrFunc *)func, stdout);

    printf("remove load instructions\n");
    klr_remove_load_pass((KlrFunc *)func, NULL);
    klr_print_func((KlrFunc *)func, stdout);

    printf("remove store instructions\n");
    klr_remove_store_pass((KlrFunc *)func, NULL);
    klr_print_func((KlrFunc *)func, stdout);
}

/*
func bar(n int) int {
    return foo(n, n)
}
*/
void build_bar(KlrModule *m)
{
    TypeSpec *param_types[] = {
        int64_type_spec(),
        NULL,
    };

    KlrValue *func = klr_add_func(m, int64_type_spec(), param_types, "bar");
    bar = (KlrFunc *)func;
    KlrValue *param = klr_get_param(func, 0);
    klr_set_name(param, "n");

    KlrBasicBlock *entry = klr_append_block(func, "entry");

    KlrBuilder bldr;
    klr_builder_end(&bldr, entry);

    /* entry basic block */
    KlrValue *val = klr_build_load(&bldr, param, "");
    KlrValue *args1[] = { val, val };
    KlrValue *ret = klr_build_call(&bldr, func, args1, 2, "");
    klr_build_ret(&bldr, ret);

    klr_print_func((KlrFunc *)func, stdout);
    klr_print_cfg((KlrFunc *)func, stdout);

    printf("do inline\n");
    klr_print_func((KlrFunc *)func, stdout);
}

typedef struct _InlineContext {
    KlrFunc *caller;
    KlrFunc *callee;
    KlrInsn *call_insn;
} InlineContext;

struct mapping {
    KlrValue *callee_val;
    KlrValue *caller_val;
} mappings[32];

static int mapping_size = 0;
static KlrValue *ret_val;

static KlrValue *get_caller_val(KlrValue *callee_val)
{
    for (int i = 0; i < mapping_size; i++) {
        if (mappings[i].callee_val == callee_val) {
            return mappings[i].caller_val;
        }
    }

    if (callee_val->kind == KLR_VALUE_CONST) {
        return callee_val;
    }

    UNREACHABLE();
}

static void do_inline(KlrModule *m)
{
    KlrBasicBlock *bb;
    KlrInsn *insn, *nxt_insn;
    basic_block_foreach(bb, bar) {
        insn_foreach_safe(insn, nxt_insn, bb) {
            if (insn->code == OP_CALL) {
                break;
            }
        }
    }

    KlrBuilder bldr;
    klr_builder_at(&bldr, insn);

    KlrValue *p = klr_get_param((KlrValue *)foo, 0);
    mappings[mapping_size].callee_val = p;
    mappings[mapping_size].caller_val = insn->opers[1].use.ref;
    mapping_size++;

    p = klr_get_param((KlrValue *)foo, 1);
    mappings[mapping_size].callee_val = p;
    mappings[mapping_size].caller_val = insn->opers[2].use.ref;
    mapping_size++;

    KlrBasicBlock *bb2;
    KlrInsn *insn2, *nxt_insn2;
    basic_block_foreach(bb2, foo) {
        KlrLocal *local;
        list_foreach(local, bb_link, &bb2->local_list) {
            KlrLocal *new_local =
                (KlrLocal *)klr_add_local(&bldr, local->ts, local->name);
            mappings[mapping_size].callee_val = (KlrValue *)local;
            mappings[mapping_size].caller_val = (KlrValue *)new_local;
            mapping_size++;
        }

        insn_foreach_safe(insn2, nxt_insn2, bb2) {
            if (insn2->code == OP_BINARY_ADD) {
                KlrValue *_add =
                    klr_build_add(&bldr, get_caller_val(insn2->opers[0].use.ref),
                                  get_caller_val(insn2->opers[1].use.ref), "add");
                // printf("add: %s, %s\n", insn2->opers[0].use.ref->name,
                // insn2->opers[1].use.ref->name);
                mappings[mapping_size].callee_val = (KlrValue *)insn2;
                mappings[mapping_size].caller_val = _add;
                mapping_size++;
            } else if (insn2->code == OP_RETURN) {
                ret_val = get_caller_val(insn2->opers[0].use.ref);
            } else if (insn2->code == OP_IR_STORE) {
                klr_build_store(&bldr, get_caller_val(insn2->opers[0].use.ref),
                                get_caller_val(insn2->opers[1].use.ref));
            } else {
                NYI();
            }
        }
    }

    KlrUse *use, *nxt_use;
    use_foreach_safe(use, nxt_use, insn) {
        KlrInsn *v = (KlrInsn *)use->insn;
        if (v->code == OP_RETURN) {
            list_remove(&v->opers[0].use.use_link);
            v->opers[0].use.ref = ret_val;
        } else {
            UNREACHABLE();
        }
    }

    klr_delete_insn(insn);
    klr_remove_load_pass((KlrFunc *)bar, NULL);
    klr_remove_store_pass((KlrFunc *)bar, NULL);
    klr_remove_unused_pass((KlrFunc *)bar, NULL);

    printf("after inline\n");
    klr_print_func((KlrFunc *)bar, stdout);

    printf("linear scan register allocation\n");
    klr_alloc_registers((KlrFunc *)bar);
    klr_print_func((KlrFunc *)bar, stdout);

    printf("remap instructions\n");
    klr_insn_remap((KlrFunc *)bar);
    klr_print_func((KlrFunc *)bar, stdout);
}

int main(int argc, char *argv[])
{
    init_atom();
    init_log(LOG_TRACE, NULL, 0);
    typespec_init();
    KlrModule *m = klr_create_module("example");
    build_foo(m);
    build_bar(m);
    do_inline(m);
    fini_log();
    fini_atom();
    return 0;
}

#ifdef __cplusplus
}
#endif

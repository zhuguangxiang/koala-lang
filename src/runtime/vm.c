/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "vm.h"
#include <unistd.h>
#include "atom.h"
#include "klc.h"
#include "log.h"
#include "mm.h"
#include "modobj.h"

#ifdef __cplusplus
extern "C" {
#endif

void init_tag_mappings(void);

/* max stack size */
#define MAX_STACK_SIZE (2 * 64 * 1024)

/* pthread */
__thread ThreadState *__ts;

KoalaState *kl_new_ks(void)
{
    KoalaState *ks = mm_alloc_obj(ks);
    ks->ts = __ts;
    ks->stack_base = aligned_alloc(16, sizeof(TValue) * MAX_STACK_SIZE);
    ks->stack_top = ks->stack_base;
    ks->stack_size = MAX_STACK_SIZE;
    return ks;
}

void kl_free_ks(KoalaState *ks)
{
    if (!ks) return;
    ASSERT(!ks->cf);
    free(ks->stack_base);
    mm_free(ks);
}

void koala_initialize(void)
{
    /* init logger */
    init_log(LOG_INFO, NULL, 0);

    /* init atom string table */
    init_atom();

    /* init global module table */
    kl_init_gm_stbl();

    /* init builtin & sys module */
    init_builtin_module();
    // init_sys_module(ks);

    /* initialize main thread as koala thread */
    ThreadState *ts = mm_alloc_obj(ts);
    ts->current = kl_new_ks();
    __ts = ts;

    /* initialize tag mappings */
    init_tag_mappings();
}

static void __add_const(Object *m, KlcConst *item)
{
    switch (item->type) {
        case KLC_CONST_NONE: {
            NYI();
            break;
        }
        case KLC_CONST_INT: {
            if (item->sign)
                kl_mo_add_int(m, (int64_t)item->ival, item->type_info);
            else
                kl_mo_add_uint(m, item->ival, item->type_info);
            break;
        }
        case KLC_CONST_FLT: {
            kl_mo_add_float(m, item->fval, item->type_info);
            break;
        }
        case KLC_CONST_SHORT_ASCII:
        case KLC_CONST_SHORT_UTF8:
        case KLC_CONST_ASCII:
        case KLC_CONST_UTF8: {
            kl_mo_add_str(m, item->sval);
            break;
        }
        default: {
            NYI();
            break;
        }
    }
}

void koala_run_file(char *path)
{
    KlcFile *klc = read_klc_file(path, 1);
    if (!klc) return;

    Object *m = kl_new_module(path);

    uint8_t *codes = NULL;
    uint32_t size = klc_get_bytecodes(klc, &codes);
    kl_mo_set_code(m, (uint32_t *)codes, size / 4);

    int num_rt_consts = klc->num_rt_consts;
    Vector *rt_consts = klc->objs + ITEM_RT_CONST;
    KlcConst *c;
    vector_foreach(c, rt_consts) {
        if (!c) continue;
        if (num_rt_consts <= 0) break;
        __add_const(m, c);
        --num_rt_consts;
    }

    Vector *imports = klc->objs + ITEM_IMPORT;
    KlcImport *imp;
    vector_foreach(imp, imports) {
        if (!imp) continue;
        KlcConst *ns = klc_get_rt_const(klc, imp->ns_index);
        KlcConst *sym = klc_get_rt_const(klc, imp->sym_index);
        kl_mo_add_import(m, imp->kind, ns->sval, sym->sval);
    }

    Vector *code_objs = klc->objs + ITEM_CODE;
    KlcCode *item;
    vector_foreach(item, code_objs) {
        if (!item) continue;
        KlcConst *c = klc_get_rt_const(klc, item->name_index);
        Object *_co = kl_new_code(c->sval, m);
        CodeObject *co = (CodeObject *)_co;
        co->cs.nlocals = item->nlocals;
        co->cs.max_call_args = item->max_call_args;
        co->cs.start_pc = item->start_pc;
        co->cs.num_insns = item->num_insns;
        kl_mo_add_func(m, _co);
    }

    kl_init_module(m);
    kl_resolve_import(m);
    kl_run_module(m);

    free_klc_file(klc);
}

void koala_finalize(void) { /* finalize atom string table */ fini_atom(); }

#ifdef __cplusplus
}
#endif

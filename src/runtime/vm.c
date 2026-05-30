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

static void __load_const(Object *m, KlcConst *item)
{
    switch (item->type) {
        case KLC_CONST_NONE: {
            kl_mo_add_none(m);
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

        case KLC_CONST_BOOL: {
            kl_mo_add_bool(m, (int)item->ival);
            break;
        }

        case KLC_CONST_SHORT_ASCII:
        case KLC_CONST_SHORT_UTF8:
        case KLC_CONST_ASCII:
        case KLC_CONST_UTF8: {
            kl_mo_add_str(m, item->sval);
            break;
        }

        case KLC_CONST_SHORT_TUPLE:
        case KLC_CONST_TUPLE: {
            Vector *vec = item->val;
            kl_mo_add_tuple(m, vec);
            break;
        }

        case KLC_CONST_RANGE: {
            Vector *vec = item->val;
            kl_mo_add_range(m, vec);
            break;
        }

        case KLC_CONST_SHORT_LIST:
        case KLC_CONST_LIST: {
            Vector *vec = item->val;
            kl_mo_add_list(m, vec);
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
    KlcConst *kc;
    vector_foreach(kc, rt_consts) {
        if (!kc) continue;
        if (num_rt_consts <= 0) break;
        __load_const(m, kc);
        --num_rt_consts;
    }

    Vector *imports = klc->objs + ITEM_IMPORT;
    KlcImport *imp;
    vector_foreach(imp, imports) {
        if (!imp) continue;
        KlcConst *ns = klc_get_rt_const(klc, imp->ns_index);
        KlcConst *kls = klc_get_rt_const(klc, imp->kls_index);
        KlcConst *sym = klc_get_rt_const(klc, imp->sym_index);
        char *kls_name = kls ? kls->sval : NULL;
        kl_mo_add_import(m, imp->kind, ns->sval, kls_name, sym->sval);
    }

    Vector *code_objs = klc->objs + ITEM_CODE;
    KlcCode *item;
    vector_foreach(item, code_objs) {
        if (!item) continue;
        kc = klc_get_rt_const(klc, item->name_index);
        Object *_co = kl_new_code(kc->sval, m);
        CodeObject *co = (CodeObject *)_co;
        if (item->flags & KLC_FLAGS_PUB) co->flags |= CODE_FLAG_PUB;
        if (item->flags & KLC_FLAGS_METH) co->flags |= CODE_FLAG_METH;
        co->cs.nlocals = item->nlocals;
        co->cs.max_call_args = item->max_call_args;
        co->cs.start_pc = item->start_pc;
        co->cs.num_insns = item->num_insns;
        kl_mo_add_func(m, _co);
    }

    ModuleObject *mo = (ModuleObject *)m;
    Vector *cls_objs = klc->objs + ITEM_CLASS;
    KlcKlass *cls;
    vector_foreach(cls, cls_objs) {
        if (!cls) continue;
        KlcConst *kls_kc = klc_get_const(klc, cls->name_index);
        TypeObject *tp = kl_new_type(kls_kc->sval, cls->flags);

        if (cls->flags & KLC_FLAGS_TRAIT) continue;

        KlcVar *var;
        vector_foreach(var, &cls->fields) {
            if (!var) continue;
            kc = klc_get_const(klc, var->name_index);
            Object *field = kl_new_index_field(kc->sval, 0, i__, (Object *)tp);
            vector_push_back(&tp->fields, &field);
            stbl_add_obj(&tp->members, kc->sval, field);
        }

        KlcFunc *meth;
        vector_foreach(meth, &cls->methods) {
            if (!meth) continue;
            kc = klc_get_const(klc, meth->name_index);
            Object *_co = vector_get(&mo->funcs, meth->code_index);
            ASSERT(_co && IS_CODE(_co));
            CodeObject *co = (CodeObject *)_co;
            ASSERT(co->flags & CODE_FLAG_METH);
            vector_push_back(&tp->methods, &co);
            stbl_add_obj(&tp->members, kc->sval, _co);
        }

        KlcIntfEntry *intf_entry;
        vector_foreach(intf_entry, &cls->intf_table) {
            if (!intf_entry) continue;

            IntfTable itable;
            kc = klc_get_const(klc, intf_entry->name_index);
            itable.name = atom(kc->sval);
            itable.num_funcs = vector_size(&intf_entry->methods);
            itable.num_parents = vector_size(&intf_entry->parents);
            itable.methods = mm_alloc(sizeof(Object *) * itable.num_funcs);
            itable.parents = mm_alloc(sizeof(IntfTable *) * itable.num_parents);

            uint16_t _idx = 0;
            vector_foreach(_idx, &intf_entry->methods) {
                Object *co = vector_get(&mo->funcs, _idx);
                ASSERT(co && IS_CODE(co));
                CodeObject *co_obj = (CodeObject *)co;
                ASSERT(co_obj->flags & CODE_FLAG_METH);
                ASSERT(i__ < itable.num_funcs);
                itable.methods[i__] = co;
            }

            vector_push_back(&tp->itables, &itable);
        }

        vector_foreach(intf_entry, &cls->intf_table) {
            if (!intf_entry) continue;

            IntfTable *itable = vector_get_ptr(&tp->itables, i__ - 1);

            uint16_t _idx = 0;
            vector_foreach(_idx, &intf_entry->parents) {
                IntfTable *parent = vector_get_ptr(&tp->itables, _idx);
                ASSERT(parent);
                ASSERT(i__ < itable->num_parents);
                itable->parents[i__] = parent;
            }
        }

        vector_push_back(&mo->types, &tp);
        stbl_add_obj(&mo->symbols, kls_kc->sval, (Object *)tp);
        tp->flags |= TP_FLAGS_READY;
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

/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "atom.h"
#include "klc.h"
#include "log.h"
#include "mm.h"
#include "symbol.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FIXUP_UNKNOWN,
    FIXUP_TP_BOUND,
    FIXUP_FUNC_PARAM,
    FIXUP_FUNC_RET,
    FIXUP_KLASS_BASE,
    FIXUP_GENERIC_VAR,
    FIXUP_FUNC_PROTO,
} FixupKind;

typedef struct _FixupEntry {
    FixupKind kind;
    void *owner;
    int index;
} FixupEntry;

typedef struct _LoadKlcContext {
    HashMap *stbl;
    KlcFile *klc;
    Vector fixups;
    Vector stage_2_fixups;
} LoadContext;

static inline int ts_need_fixup(TypeSpec *ts)
{
    return ts->kind == TYPE_GENERIC_REF || ts->kind == TYPE_GENERIC_VAR ||
           ts->kind == TYPE_MANGLED || ts->kind == TYPE_KLASS;
}

static void add_fixup_entry(FixupEntry *entry, TypeSpec *ts, LoadContext *ctx)
{
    ASSERT(ts_need_fixup(ts));

    vector_push_back(&ctx->fixups, entry);

    if (ts->kind == TYPE_GENERIC_REF) {
        TypeSpec *arg_ts;
        vector_foreach(arg_ts, ts->generic_ref.args) {
            ASSERT(arg_ts->kind == TYPE_GENERIC_VAR);
            FixupEntry arg_entry = {
                .kind = FIXUP_GENERIC_VAR,
                .owner = ts,
                .index = i__,
            };
            vector_push_back(&ctx->fixups, &arg_entry);
        }
    }
}

static void add_stage_2_fixup_entry(FixupEntry *entry, LoadContext *ctx)
{
    vector_push_back(&ctx->stage_2_fixups, entry);
}

static void fixup_type_spec(TypeSpec **ts_ptr, LoadContext *ctx)
{
    ASSERT(ts_ptr);
    TypeSpec *ts = *ts_ptr;
    ASSERT(ts);

    if (ts->kind == TYPE_GENERIC_REF) {
        Symbol *sym = stbl_get(ctx->stbl, ts->generic_ref.name);
        if (sym) {
            log_info("found symbol for generic_ref type: %s", ts->generic_ref.name);
            ts->sym_id = sym->id;
        } else {
            UNREACHABLE();
        }
    } else if (ts->kind == TYPE_GENERIC_VAR) {
        Symbol *owner = stbl_get(ctx->stbl, ts->generic_var.owner);
        if (owner) {
            Symbol *tp_sym = stbl_get(owner->stbl, ts->generic_var.name);
            ASSERT(tp_sym && tp_sym->kind == SYM_TYPE_PARAM);
            ts->sym_id = tp_sym->id;
            ts->generic_var.index = ((TypeParamSymbol *)tp_sym)->index;
        } else {
            UNREACHABLE();
        }
    } else if (ts->kind == TYPE_KLASS) {
        Symbol *sym = stbl_get(ctx->stbl, ts->klass_type.name);
        if (sym) {
            log_info("found symbol for klass type: %s", ts->klass_type.name);
            ts->sym_id = sym->id;
        } else {
            UNREACHABLE();
        }
    } else if (ts->kind == TYPE_MANGLED) {
        Symbol *origin = stbl_get(ctx->stbl, ts->mangled.name);
        ASSERT(origin && (origin->kind == SYM_CLASS || origin->kind == SYM_TRAIT));
        log_info("found origin symbol for mangled type: %s", ts->mangled.name);
        InstanceSymbol *inst_sym =
            find_or_add_instance(ctx->stbl, origin, ts->mangled.args);
        ASSERT(inst_sym);
        type_spec_free(ts);
        *ts_ptr = inst_sym->instance_ts;
    } else {
        UNREACHABLE();
    }
}

static void __do_fixup(Vector *fixups, LoadContext *ctx)
{
    FixupEntry *entry;
    vector_foreach_ptr(entry, fixups) {
        if (!entry) continue;
        switch (entry->kind) {
            case FIXUP_TP_BOUND: {
                TypeParamSymbol *tp = (TypeParamSymbol *)entry->owner;
                TypeSpec **ts_ptr = vector_get_ptr(&tp->bound, entry->index);
                fixup_type_spec(ts_ptr, ctx);
                break;
            }
            case FIXUP_FUNC_PARAM: {
                ArgInfo *arg = entry->owner;
                TypeSpec **ts_ptr = &arg->ts;
                fixup_type_spec(ts_ptr, ctx);
                break;
            }
            case FIXUP_FUNC_RET: {
                FuncSymbol *fn_sym = entry->owner;
                TypeSpec **ts_ptr = &fn_sym->ret;
                fixup_type_spec(ts_ptr, ctx);
                break;
            }
            case FIXUP_KLASS_BASE: {
                KlassSymbol *kls_sym = entry->owner;
                TypeSpec **ts_ptr = vector_get_ptr(&kls_sym->bases, entry->index);
                fixup_type_spec(ts_ptr, ctx);
                break;
            }
            case FIXUP_GENERIC_VAR: {
                TypeSpec *ts = entry->owner;
                TypeSpec **ts_ptr = vector_get_ptr(ts->generic_ref.args, entry->index);
                fixup_type_spec(ts_ptr, ctx);
                break;
            }
            case FIXUP_FUNC_PROTO: {
                FuncSymbol *fn_sym = entry->owner;
                // create proto type for this function
                ASSERT(fn_sym->ts == NULL);
                fn_sym->ts = func_type_spec_from_arginfo(fn_sym->params, fn_sym->ret);
                break;
            }
            default: {
                UNREACHABLE();
                break;
            }
        }
    }
}

static void do_fixup(LoadContext *ctx)
{
    __do_fixup(&ctx->fixups, ctx);
    __do_fixup(&ctx->stage_2_fixups, ctx);
}

static void load_func(KlcFunc *fn, KlassSymbol *kls_sym, LoadContext *ctx)
{
    Vector *params = vector_create_ptr();

    KlcArgument *arg;
    vector_foreach(arg, &fn->args) {
        if (!arg) continue;
        ArgInfo *arg_info = mm_alloc_obj(arg_info);
        KlcConst *name = klc_get_const(ctx->klc, arg->name_index);
        KlcConst *ty_k = klc_get_const(ctx->klc, arg->type_index);
        TypeSpec *ts = type_spec_from_str(ty_k->sval);
        KlcConst *def_val = klc_get_const(ctx->klc, arg->const_index);
        arg_info->name = name->sval;
        arg_info->ts = ts;
        arg_info->dfl_val_idx = def_val ? 1 : 0;
        vector_push_back(params, &arg_info);
        if (ts_need_fixup(ts)) {
            FixupEntry entry = {
                .kind = FIXUP_FUNC_PARAM,
                .owner = arg_info,
                .index = -1,
            };
            if (ts->kind == TYPE_MANGLED) {
                add_stage_2_fixup_entry(&entry, ctx);
            } else {
                add_fixup_entry(&entry, ts, ctx);
            }
        } else {
            // nothing
        }
    }

    KlcConst *k = klc_get_const(ctx->klc, fn->name_index);
    KlcConst *ret = klc_get_const(ctx->klc, fn->ret_type_index);
    TypeSpec *ret_ts = ret ? type_spec_from_str(ret->sval) : no_type_spec();

    HashMap *stbl = kls_sym ? kls_sym->stbl : ctx->stbl;
    Symbol *sym = stbl_add_func(stbl, k->sval, ret_ts, params, 0);
    ASSERT(sym);

    if (ts_need_fixup(ret_ts)) {
        FixupEntry entry = {
            .kind = FIXUP_FUNC_RET,
            .owner = sym,
            .index = -1,
        };
        if (ret_ts->kind == TYPE_MANGLED) {
            add_stage_2_fixup_entry(&entry, ctx);
        } else {
            add_fixup_entry(&entry, ret_ts, ctx);
        }
    } else {
        // nothing
    }

    // after all params and ret type are fixed, we can create proto type for this function
    FixupEntry proto_entry = {
        .kind = FIXUP_FUNC_PROTO,
        .owner = sym,
        .index = -1,
    };
    add_stage_2_fixup_entry(&proto_entry, ctx);

    sym->status = SYM_RESOLVED;
}

static void load_type_params(KlcKlass *kls, void *owner, LoadContext *ctx)
{
    KlassSymbol *cls_sym = (KlassSymbol *)owner;
    Vector *tps = &kls->tps;
    Vector *result = &cls_sym->tps;

    KlcTypeParam *tp;
    vector_foreach(tp, tps) {
        if (!tp) continue;

        KlcConst *name = klc_get_const(ctx->klc, tp->name_index);
        TypeParamSymbol *tp_sym = stbl_add_type_param(cls_sym->stbl, name->sval, owner);
        tp_sym->index = vector_size(result);
        tp_sym->which = tp->which;
        vector_push_back(result, &tp_sym);

        // add bounds
        uint16_t bound;
        vector_foreach(bound, &tp->bounds) {
            if (!bound) continue;
            KlcConst *bound_k = klc_get_const(ctx->klc, bound);
            TypeSpec *ts = type_spec_from_str(bound_k->sval);
            vector_push_back(&tp_sym->bound, &ts);
            if (ts->kind == TYPE_GENERIC_REF) {
                log_info("generic_ref type in type param bound: %s", ts->signature);
                FixupEntry entry = {
                    .kind = FIXUP_TP_BOUND,
                    .owner = (Symbol *)tp_sym,
                    .index = vector_size(&tp_sym->bound) - 1,
                };
                add_fixup_entry(&entry, ts, ctx);
            } else if (ts->kind == TYPE_MANGLED) {
                UNREACHABLE();
            }
        }
    }
}

static void load_bases(KlcKlass *kls, KlassSymbol *sym, LoadContext *ctx)
{
    uint16_t base;
    vector_foreach(base, &kls->bases) {
        if (!base) continue;
        KlcConst *base_k = klc_get_const(ctx->klc, base);
        TypeSpec *ts = type_spec_from_str(base_k->sval);
        vector_push_back(&sym->bases, &ts);
        if (ts->kind == TYPE_GENERIC_REF) {
            log_info("generic_ref type in klass base: %s", ts->signature);
            FixupEntry entry = {
                .kind = FIXUP_KLASS_BASE,
                .owner = sym,
                .index = vector_size(&sym->bases) - 1,
            };
            add_fixup_entry(&entry, ts, ctx);
        } else if (ts->kind == TYPE_MANGLED) {
            log_info("mangled type in klass base: %s", ts->signature);
            FixupEntry entry = {
                .kind = FIXUP_KLASS_BASE,
                .owner = sym,
                .index = vector_size(&sym->bases) - 1,
            };
            add_stage_2_fixup_entry(&entry, ctx);
        } else {
            UNREACHABLE();
        }
    }
}

static int __type_in_vec(Vector *vec, TypeSpec *ts)
{
    TypeSpec *existing_ts;
    vector_foreach(existing_ts, vec) {
        if (!existing_ts) continue;
        if (existing_ts == ts) {
            return 1;
        }
    }
    return 0;
}

#ifndef NOLOG
void print_vtbl_info(KlassSymbol *sym);
#endif

static void load_pip_lro_scm(KlcKlass *kls, KlassSymbol *sym, LoadContext *ctx)
{
    uint16_t pip;
    vector_foreach(pip, &kls->pip) {
        if (!pip) continue;
        KlcConst *base_k = klc_get_const(ctx->klc, pip);
        TypeSpec *ts = type_spec_from_str(base_k->sval);
        vector_push_back(&sym->pip, &ts);
    }

    uint16_t lro;
    vector_foreach(lro, &kls->lro) {
        if (!lro) continue;
        KlcConst *base_k = klc_get_const(ctx->klc, lro);
        TypeSpec *ts = type_spec_from_str(base_k->sval);
        vector_push_back(&sym->lro, &ts);
    }

    TypeSpec *ts;
    vector_foreach(ts, &sym->lro) {
        if (!ts) continue;

        if (!__type_in_vec(&sym->pip, ts)) {
            vector_push_back(&sym->scm, &ts);
        }
    }

#ifndef NOLOG
    print_vtbl_info(sym);
#endif
}

static void load_klass(KlcKlass *kls, LoadContext *ctx)
{
    KlcConst *k = klc_get_const(ctx->klc, kls->name_index);

    KlassSymbol *cls_sym;
    if (kls->flags & KLC_FLAGS_TRAIT) {
        cls_sym = stbl_add_klass(ctx->stbl, k->sval, SYM_FLAGS_PUBLIC, 1);
    } else {
        cls_sym = stbl_add_klass(ctx->stbl, k->sval, SYM_FLAGS_PUBLIC, 0);
    }

    // add type params
    load_type_params(kls, cls_sym, ctx);

    // add bases
    load_bases(kls, cls_sym, ctx);

    // add pip & lro & scm
    load_pip_lro_scm(kls, cls_sym, ctx);

    // add methods
    KlcFunc *fn;
    vector_foreach(fn, &kls->methods) {
        if (!fn) continue;
        load_func(fn, cls_sym, ctx);
    }

    cls_sym->status = SYM_RESOLVED;
}

static void load_funcs(LoadContext *ctx)
{
    KlcFile *klc = ctx->klc;
    Vector *consts = klc->objs + ITEM_CONST;

    KlcFunc *item;
    vector_foreach(item, klc->objs + ITEM_FUNC) {
        if (!item) continue;
        if (!(item->flags & KLC_FLAGS_PUB)) {
            continue;
        }
        load_func(item, NULL, ctx);
    }
}

static void load_klasses(LoadContext *ctx)
{
    KlcFile *klc = ctx->klc;
    Vector *consts = klc->objs + ITEM_CONST;

    KlcKlass *kls;
    vector_foreach(kls, klc->objs + ITEM_CLASS) {
        if (!kls) continue;
        if (!(kls->flags & KLC_FLAGS_PUB)) {
            continue;
        }

        load_klass(kls, ctx);
    }
}

// absolute path to klc file
static HashMap *__load(char *path)
{
    log_info("read klc file: %s", path);

    KlcFile *klc = read_klc_file(path, 0);
    if (!klc) {
        log_error("failed to read klc file: %s", path);
        return NULL;
    }

    HashMap *stbl = stbl_new();

    LoadContext ctx;
    ctx.stbl = stbl;
    ctx.klc = klc;
    vector_init(&ctx.fixups, sizeof(FixupEntry));
    vector_init(&ctx.stage_2_fixups, sizeof(FixupEntry));

    load_klasses(&ctx);
    load_funcs(&ctx);
    stbl_show(stbl);
    do_fixup(&ctx);

    free_klc_file(klc);
    vector_fini(&ctx.fixups);
    vector_fini(&ctx.stage_2_fixups);

    return stbl;
}

// path without .klc suffix
HashMap *load_module(char *path)
{
    char *koala_path = getenv("KOALA_PATH");
    if (!koala_path) {
        log_info("KOALA_PATH is not set");
        return __load(path);
    }

    log_info("KOALA_PATH: %s", koala_path);

    BUF(buf);
    HashMap *stbl = NULL;
    char *prefix = NULL;
    int count = str_sep(&koala_path, ':', &prefix);
    while (count > 0) {
        buf_write_nstr(&buf, prefix, count);
        buf_write_str(&buf, path);
        buf_write_str(&buf, ".klc");
        stbl = __load(BUF_STR(buf));
        if (stbl) {
            log_info("found module '%s' in KOALA_PATH: %s", path, prefix);
            break;
        }

        RESET_BUF(buf);
        count = str_sep(NULL, ':', &prefix);
    }

    FINI_BUF(buf);

    return stbl;
}

#ifdef __cplusplus
}
#endif

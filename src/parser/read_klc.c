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

static void add_func(HashMap *stbl, KlcFile *klc, KlcFunc *item)
{
    KlcConst *k = klc_get_const(klc, item->name_index);
    KlcConst *ret = klc_get_const(klc, item->ret_type_index);
    TypeSpec *ret_desc = ret ? type_spec_from_str(ret->sval) : NULL;

    KlcArgument *arg;
    Vector *params = vector_create_ptr();
    vector_foreach_object(arg, &item->args)
    {
        if (!arg) continue;
        ArgInfo *arg_info = mm_alloc_obj_fast(arg_info);
        KlcConst *name = klc_get_const(klc, arg->name_index);
        KlcConst *ty_k = klc_get_const(klc, arg->type_index);
        TypeSpec *ts = type_spec_from_str(ty_k->sval);
        arg_info->name = name->sval;
        arg_info->ts = ts;
        arg_info->dfl_val_idx = 0;
        vector_push_back(params, &arg_info);
    }
    stbl_add_func(stbl, k->sval, NULL, ret_desc, params, 0, NULL, NULL);
}

static void read_funcs(HashMap *stbl, KlcFile *klc)
{
    Vector *consts = klc->objs + ITEM_CONST;

    KlcFunc **item_p;
    KlcFunc *item;
    vector_foreach(item_p, klc->objs + ITEM_FUNC) {
        item = *item_p;
        if (!item) continue;

        add_func(stbl, klc, item);
    }
}

static void read_classes(HashMap *stbl, KlcFile *klc)
{
    Vector *consts = klc->objs + ITEM_CONST;

    KlcKlass *kls;
    vector_foreach_object(kls, klc->objs + ITEM_CLASS)
    {
        if (!kls) continue;

        // add class symbol
        if (!(kls->flags & KLC_FLAGS_PUB)) {
            continue;
        }

        KlcConst *k = klc_get_const(klc, kls->name_index);

        Symbol *cls_sym;
        if (kls->flags & KLC_FLAGS_TRAIT) {
            cls_sym = stbl_add_trait(stbl, k->sval, SYM_FLAGS_PUBLIC);
        } else {
            cls_sym = stbl_add_klass(stbl, k->sval, SYM_FLAGS_PUBLIC);
        }

        // read methods
        KlcFunc *fn;
        vector_foreach_object(fn, &kls->methods)
        {
            if (!fn) continue;
            add_func(cls_sym->stbl, klc, fn);
        }
    }
}

void kl_read_from_klc(HashMap *stbl, char *path)
{
    printf("read klc file: %s\n", path);
    KlcFile klc;
    init_klc_file(&klc, path);
    read_klc_file(&klc, 0);
    klc_dump(&klc);
    read_classes(stbl, &klc);
    fini_klc_file(&klc);
}

#ifdef __cplusplus
}
#endif

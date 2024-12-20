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

static void read_funcs(HashMap *stbl, KlcFile *klc)
{
    Vector *consts = klc->objs + ITEM_CONST;

    KlcFunc **item_p;
    KlcFunc *item;
    vector_foreach(item_p, klc->objs + ITEM_FUNC) {
        item = *item_p;
        if (!item) continue;

        KlcConst *k = klc_get_const(klc, item->name_index);
        KlcConst *ret = klc_get_const(klc, item->ret_type_index);
        TypeDesc *ret_desc = ret ? desc_from_str(ret->sval) : desc_no_type();
        KlcArgument **arg_p;
        KlcArgument *arg;
        Vector *params = vector_create_ptr();
        vector_foreach(arg_p, &item->args) {
            arg = *arg_p;
            ArgInfo *arg_info = mm_alloc_obj_fast(arg_info);
            KlcConst *name = klc_get_const(klc, arg->name_index);
            KlcConst *ty_k = klc_get_const(klc, arg->type_index);
            TypeDesc *desc = desc_from_str(ty_k->sval);
            arg_info->name = name->sval;
            arg_info->desc = desc;
            arg_info->dfl_val_idx = 0;
            vector_push_back(params, &arg_info);
        }
        stbl_add_func(stbl, k->sval, NULL, ret_desc, params, 0, NULL, NULL);
    }
}

void kl_read_from_klc(HashMap *stbl, char *path)
{
    printf("read klc file: %s\n", path);
    KlcFile klc;
    init_klc_file(&klc, path);
    read_klc_file(&klc, 0);
    klc_dump(&klc);
    read_funcs(stbl, &klc);
    fini_klc_file(&klc);
}

#ifdef __cplusplus
}
#endif

/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "atom.h"
#include "buffer.h"
#include "klc.h"
#include "typedesc.h"
#include "version.h"

#ifdef __cplusplus
extern "C" {
#endif

static void dump_const(KlcConst *item)
{
    switch (item->type) {
        case KLC_CONST_NONE: {
            fprintf(stdout, "none\n");
            break;
        }
        case KLC_CONST_INT: {
            fprintf(stdout, "int, %ld\n", item->ival);
            break;
        }
        case KLC_CONST_FLT: {
            fprintf(stdout, "flt, %lf\n", item->fval);
            break;
        }
        case KLC_CONST_SHORT_ASCII:
        case KLC_CONST_SHORT_UTF8:
        case KLC_CONST_ASCII:
        case KLC_CONST_UTF8: {
            fprintf(stdout, "str, \"%s\"\n", item->sval);
            break;
        }
        default: {
            break;
        }
    }
}

static void dump_consts(Vector *vec)
{
    fprintf(stdout, "constants:\n");
    KlcConst **item_p;
    KlcConst *item;
    vector_foreach(item_p, vec) {
        item = *item_p;
        if (!item) continue;
        fprintf(stdout, "  [%2d] = ", i__);
        dump_const(item);
    }
}

static void dump_vars(Vector *vec, KlcFile *klc)
{
    fprintf(stdout, "variables:\n");

    BUF(buf);
    Vector *consts = klc->objs + ITEM_CONST;
    KlcVar **item_p;
    KlcVar *item;
    vector_foreach(item_p, vec) {
        item = *item_p;
        if (!item) continue;
        fprintf(stdout, "  [%2d] = ", i__);
        KlcConst **k = vector_get(consts, item->name_index);
        fprintf(stdout, "%s", (*k)->sval);
        k = vector_get(consts, item->type_index);
        RESET_BUF(buf);
        desc_str_print((*k)->sval, &buf);
        fprintf(stdout, " : %s = ", BUF_STR(buf));
        k = vector_get(consts, item->const_index);
        if (k && *k) dump_const(*k);
    }
    FINI_BUF(buf);
}

static void dump_anns(Vector *vec, KlcFile *klc)
{
    KlcAnnot **item_p;
    KlcAnnot *item;
    vector_foreach(item_p, vec) {
        item = *item_p;
        if (!item) continue;
        KlcConst *k = klc_get_const(klc, item->name_index);
        fprintf(stdout, "@%s(", k->sval);
        k = klc_get_const(klc, item->key_index);
        fprintf(stdout, "%s)\n", k->sval);
    }
}

static void dump_funcs(Vector *vec, KlcFile *klc)
{
    fprintf(stdout, "functions:\n");

    BUF(buf);
    Vector *consts = klc->objs + ITEM_CONST;
    KlcFunc **item_p;
    KlcFunc *item;
    vector_foreach(item_p, vec) {
        item = *item_p;
        if (!item) continue;

        dump_anns(&item->anns, klc);
        if (item->flags & KLC_FLAGS_PUB) {
            fprintf(stdout, "public ");
        }

        KlcConst *k = klc_get_const(klc, item->name_index);
        fprintf(stdout, "func %s(", k->sval);
        KlcConst *ty_k;
        KlcArgument **arg_p;
        KlcArgument *arg;
        vector_foreach(arg_p, &item->args) {
            arg = *arg_p;
            if (!arg) continue;
            k = klc_get_const(klc, arg->name_index);
            ty_k = klc_get_const(klc, arg->type_index);
            RESET_BUF(buf);
            desc_str_print(ty_k->sval, &buf);
            if (i__ != 0) {
                fprintf(stdout, ", %s: %s", k->sval, BUF_STR(buf));
            } else {
                fprintf(stdout, "%s: %s", k->sval, BUF_STR(buf));
            }
        }

        fprintf(stdout, ")\n");
    }
    FINI_BUF(buf);
}

static void dump_class(Vector *vec, KlcFile *klc)
{
    fprintf(stdout, "classes:\n");
    Vector *consts = klc->objs + ITEM_CONST;

    KlcKlass **item_p;
    KlcKlass *item;
    vector_foreach(item_p, vec) {
        item = *item_p;
        if (!item) continue;
        KlcConst *k = klc_get_const(klc, item->name_index);
        fprintf(stdout, "class %s\n", k->sval);
    }
}

static void dump_relocs(Vector *vec)
{
    KlcConst **item_p;
    KlcConst *item;
    vector_foreach(item_p, vec) {
        item = *item_p;
        if (!item) continue;
    }
}

static void dump_codes(Vector *vec)
{
    KlcConst **item_p;
    KlcConst *item;
    vector_foreach(item_p, vec) {
        item = *item_p;
        if (!item) continue;
    }
}

void klc_dump(KlcFile *klc)
{
    dump_consts(klc->objs + ITEM_CONST);
    dump_vars(klc->objs + ITEM_VAR, klc);
    dump_funcs(klc->objs + ITEM_FUNC, klc);
    dump_class(klc->objs + ITEM_CLASS, klc);
    dump_relocs(klc->objs + ITEM_RELOC);
    dump_codes(klc->objs + ITEM_CODES);
}

#ifdef __cplusplus
}
#endif

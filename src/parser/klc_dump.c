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

static void dump_consts(Vector *vec)
{
    fprintf(stdout, "constants:\n");
    KlcConst **item_p;
    KlcConst *item;
    vector_foreach(item_p, vec) {
        item = *item_p;
        if (!item) continue;
        fprintf(stdout, "  [%2d] = ", i__);
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
}

static void dump_vars(Vector *vec, KlcFile *klc)
{
    fprintf(stdout, "variables:\n");

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
        fprintf(stdout, " : %s\n", (*k)->sval);
    }
}

static void dump_funcs(Vector *vec, KlcFile *klc)
{
    fprintf(stdout, "functions:\n");

    Vector *consts = klc->objs + ITEM_CONST;
    KlcFunc **item_p;
    KlcFunc *item;
    vector_foreach(item_p, vec) {
        item = *item_p;
        if (!item) continue;
        KlcConst *k = klc_get_const(klc, item->name_index);
        fprintf(stdout, "func %s(", k->sval);
        KlcArgument **arg_p;
        KlcArgument *arg;
        vector_foreach(arg_p, &item->args) {
            arg = *arg_p;
            if (!arg) continue;
            k = klc_get_const(klc, arg->name_index);
            if (i__ != 0)
                fprintf(stdout, ", %s", k->sval);
            else
                fprintf(stdout, "%s", k->sval);
        }
        fprintf(stdout, ")\n");
    }
}

static void dump_class(Vector *vec)
{
    KlcConst **item_p;
    KlcConst *item;
    vector_foreach(item_p, vec) {
        item = *item_p;
        if (!item) continue;
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
    dump_class(klc->objs + ITEM_CLASS);
    dump_relocs(klc->objs + ITEM_RELOC);
    dump_codes(klc->objs + ITEM_CODES);
}

#ifdef __cplusplus
}
#endif

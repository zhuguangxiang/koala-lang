/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <inttypes.h>
#include "atom.h"
#include "buffer.h"
#include "klc.h"
#include "opcode.h"
#include "typedesc.h"
#include "typespec.h"
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
            fprintf(stdout, "%s%d, ", item->sign ? "int" : "uint", item->len * 8);
            if (item->sign) {
                if (item->len == 1)
                    fprintf(stdout, "%d\n", (int8_t)item->ival);
                else if (item->len == 2)
                    fprintf(stdout, "%d\n", (int16_t)item->ival);
                else if (item->len == 4)
                    fprintf(stdout, "%d\n", (int32_t)item->ival);
                else
                    fprintf(stdout, "%ld\n", (int64_t)item->ival);
            } else {
                if (item->len == 1)
                    fprintf(stdout, "%u\n", (uint8_t)item->ival);
                else if (item->len == 2)
                    fprintf(stdout, "%u\n", (uint16_t)item->ival);
                else if (item->len == 4)
                    fprintf(stdout, "%u\n", (uint32_t)item->ival);
                else
                    fprintf(stdout, "%" PRIu64 "\n", (uint64_t)item->ival);
            }
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

static void dump_var(KlcVar *var, KlcFile *klc)
{
    Vector *consts = klc->objs + ITEM_CONST;

    BUF(buf);

    KlcConst **k = vector_get(consts, var->name_index);
    fprintf(stdout, "  var %s ", (*k)->sval);

    k = vector_get(consts, var->type_index);
    type_spec_str_print((*k)->sval, &buf);

    k = vector_get(consts, var->const_index);
    if (k && *k) {
        fprintf(stdout, "%s = ", BUF_STR(buf));
        dump_const(*k);
    } else {
        fprintf(stdout, "%s\n", BUF_STR(buf));
    }

    FINI_BUF(buf);
}

static void dump_vars(Vector *vec, KlcFile *klc)
{
    fprintf(stdout, "variables:\n");

    Vector *consts = klc->objs + ITEM_CONST;

    BUF(buf);

    KlcVar **item_p;
    KlcVar *item;
    vector_foreach(item_p, vec) {
        item = *item_p;
        if (!item) continue;
        dump_var(item, klc);
    }
    FINI_BUF(buf);
}

static void dump_anns(Vector *vec, KlcFile *klc, int leading_spaces)
{
    KlcAnnot **item_p;
    KlcAnnot *item;
    vector_foreach(item_p, vec) {
        item = *item_p;
        if (!item) continue;
        fprintf(stdout, "%*c", leading_spaces, ' ');
        KlcConst *k = klc_get_const(klc, item->name_index);
        fprintf(stdout, "@%s", k->sval);
        k = klc_get_const(klc, item->key_index);
        if (k) {
            fprintf(stdout, "(%s)", k->sval);
        }
        fprintf(stdout, "\n");
    }
}

static void dump_code(KlcCode *code)
{
    if (!code) return;

    fprintf(stdout, "  locals: %d\n  opcodes:\n", code->num_locals);
    uint8_t *op = (uint8_t *)code->codes;
    uint8_t *end = (uint8_t *)code->codes + code->code_size;
    while (op < end) {
        switch (*op) {
            case OP_PUSH_IMM8: {
                int8_t v = *(int8_t *)(op + 1);
                fprintf(stdout, "    push %d\n", v);
                op += 2;
                break;
            }
            case OP_PUSH_CONST: {
                uint16_t index = *(uint16_t *)(op + 1);
                fprintf(stdout, "    push-const %d\n", index);
                op += 3;
                break;
            }
            case OP_CALL: {
                int8_t num_args = *(int8_t *)(op + 1);
                uint16_t index = *(uint16_t *)(op + 2);
                fprintf(stdout, "    call %d, %d\n", num_args, index);
                op += 4;
                break;
            }
            case OP_RETURN_NONE: {
                fprintf(stdout, "    ret-void\n");
                op += 1;
                break;
            }
            default:
                break;
        }
    }
}

static void dump_func(KlcFunc *fn, KlcFile *klc, int leading_spaces)
{
    Vector *codes = klc->objs + ITEM_CODE;
    KlcCode **code_p;
    KlcCode *code;

    dump_anns(&fn->anns, klc, leading_spaces);

    fprintf(stdout, "%*c", leading_spaces, ' ');

    if (fn->flags & KLC_FLAGS_PUB) {
        fprintf(stdout, "public ");
    }

    BUF(buf);

    KlcConst *k = klc_get_const(klc, fn->name_index);
    fprintf(stdout, "func %s(", k->sval);
    KlcConst *ty_k;
    KlcArgument **arg_p;
    KlcArgument *arg;
    int i = 0;
    vector_foreach(arg_p, &fn->args) {
        arg = *arg_p;
        if (!arg) continue;
        k = klc_get_const(klc, arg->name_index);
        ty_k = klc_get_const(klc, arg->type_index);
        RESET_BUF(buf);
        type_spec_str_print(ty_k->sval, &buf);
        if (i != 0) {
            fprintf(stdout, ", %s %s", k->sval, BUF_STR(buf));
        } else {
            fprintf(stdout, "%s %s", k->sval, BUF_STR(buf));
        }
        i++;
    }

    fprintf(stdout, ") ");

    RESET_BUF(buf);

    ty_k = klc_get_const(klc, fn->ret_type_index);
    if (ty_k) {
        type_spec_str_print(ty_k->sval, &buf);
        if (fn->flags & KLC_FLAGS_TRAIT) {
            fprintf(stdout, "%s", BUF_STR(buf));
        } else {
            fprintf(stdout, "%s ", BUF_STR(buf));
        }
    }

    FINI_BUF(buf);
    if (fn->flags & KLC_FLAGS_TRAIT) {
        fprintf(stdout, "\n");
        return;
    }

    code_p = vector_get(codes, fn->code_index);
    code = *code_p;
    if (!code) {
        fprintf(stdout, "{}\n");
    } else {
        fprintf(stdout, "{\n");
        dump_code(code);
        fprintf(stdout, "}\n");
    }
}

static void dump_funcs(Vector *vec, KlcFile *klc)
{
    fprintf(stdout, "functions:\n");

    KlcFunc **item_p;
    KlcFunc *item;
    vector_foreach(item_p, vec) {
        item = *item_p;
        if (!item) continue;
        dump_func(item, klc, 2);
    }
}

static void dump_class(Vector *vec, KlcFile *klc)
{
    fprintf(stdout, "classes:\n");

    KlcKlass **item_p;
    KlcKlass *item;
    vector_foreach(item_p, vec) {
        item = *item_p;
        if (!item) continue;

        fprintf(stdout, "%*c", 2, ' ');

        KlcConst *k = klc_get_const(klc, item->name_index);
        if (item->flags & KLC_FLAGS_PUB) {
            fprintf(stdout, "public ");
        }
        if (item->flags & KLC_FLAGS_FINAL) {
            fprintf(stdout, "final ");
        }

        if (item->flags & KLC_FLAGS_TRAIT) {
            fprintf(stdout, "trait %s", k->sval);
        } else {
            fprintf(stdout, "class %s", k->sval);
        }

        if (vector_size(&item->tps) > 1) {
            fprintf(stdout, "[");
            KlcTypeParam *tp;
            int index = 0;
            vector_foreach_object(tp, &item->tps)
            {
                if (!tp) continue;
                KlcConst *name = klc_get_const(klc, tp->name_index);
                if (index != 0)
                    fprintf(stdout, ", %s", name->sval);
                else
                    fprintf(stdout, "%s", name->sval);

                if (vector_size(&tp->bounds) > 1) {
                    fprintf(stdout, ": ");
                    BUF(buf);
                    uint16_t bitem;
                    int bindex = 0;
                    vector_foreach_object(bitem, &tp->bounds)
                    {
                        if (bitem == 0) continue;

                        KlcConst *bname = klc_get_const(klc, bitem);
                        type_spec_str_print(bname->sval, &buf);
                        if (bindex != 0)
                            fprintf(stdout, " & %s", BUF_STR(buf));
                        else
                            fprintf(stdout, "%s", BUF_STR(buf));
                        RESET_BUF(buf);
                        bindex++;
                    }
                    FINI_BUF(buf);
                }

                index++;
            }
            fprintf(stdout, "]");
        }

        fprintf(stdout, " {\n");

        KlcVar **field_p;
        KlcVar *field;
        vector_foreach(field_p, &item->fields) {
            field = *field_p;
            if (!field) continue;
            dump_var(field, klc);
        }

        KlcFunc **fn_p;
        KlcFunc *fn;
        vector_foreach(fn_p, &item->methods) {
            fn = *fn_p;
            if (!fn) continue;
            dump_func(fn, klc, 4);
        }

        fprintf(stdout, "  }\n");
    }
}

static void dump_relocs(Vector *vec, KlcFile *klc)
{
    fprintf(stdout, "relocs:\n");

    Vector *consts = klc->objs + ITEM_CONST;

    KlcReloc **item_p;
    KlcReloc *item;
    vector_foreach(item_p, vec) {
        item = *item_p;
        if (!item) continue;
        fprintf(stdout, "  [%2d] = ", i__);
        KlcConst **k = vector_get(consts, item->ns_index);
        KlcConst **k2 = vector_get(consts, item->sym_index);
        fprintf(stdout, "%s:%s\n", *k ? (*k)->sval : "", (*k2)->sval);
    }
}

void klc_dump(KlcFile *klc)
{
    dump_consts(klc->objs + ITEM_CONST);
    dump_vars(klc->objs + ITEM_VAR, klc);
    dump_funcs(klc->objs + ITEM_FUNC, klc);
    dump_class(klc->objs + ITEM_CLASS, klc);
    dump_relocs(klc->objs + ITEM_RELOC, klc);
}

#ifdef __cplusplus
}
#endif

/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <inttypes.h>
#include "atom.h"
#include "buffer.h"
#include "klc.h"
#include "log.h"
#include "opcode.h"
#include "typespec.h"
#include "version.h"

#ifdef __cplusplus
extern "C" {
#endif

static void dump_header(uint8_t *magic, uint32_t version)
{
    int major = (version >> 16) & 0xFFFFu;
    int minor = (version >> 8) & 0xFFu;
    int patch = version & 0xFFu;
    fprintf(stdout, "\nversion: %" PRIu32 " (%d.%d.%d)\n", version, major, minor, patch);
}

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

static void dump_const_value(KlcConst *item)
{
    switch (item->type) {
        case KLC_CONST_NONE: {
            fprintf(stdout, "null");
            break;
        }
        case KLC_CONST_INT: {
            if (item->sign) {
                if (item->len == 1)
                    fprintf(stdout, "%d", (int8_t)item->ival);
                else if (item->len == 2)
                    fprintf(stdout, "%d", (int16_t)item->ival);
                else if (item->len == 4)
                    fprintf(stdout, "%d", (int32_t)item->ival);
                else
                    fprintf(stdout, "%ld", (int64_t)item->ival);
            } else {
                if (item->len == 1)
                    fprintf(stdout, "%u", (uint8_t)item->ival);
                else if (item->len == 2)
                    fprintf(stdout, "%u", (uint16_t)item->ival);
                else if (item->len == 4)
                    fprintf(stdout, "%u", (uint32_t)item->ival);
                else
                    fprintf(stdout, "%" PRIu64, (uint64_t)item->ival);
            }
            break;
        }
        case KLC_CONST_FLT: {
            fprintf(stdout, "%lf", item->fval);
            break;
        }
        case KLC_CONST_SHORT_ASCII:
        case KLC_CONST_SHORT_UTF8:
        case KLC_CONST_ASCII:
        case KLC_CONST_UTF8: {
            fprintf(stdout, "\"%s\"", item->sval);
            break;
        }
        default: {
            break;
        }
    }
}

static void dump_consts(Vector *vec)
{
    fprintf(stdout, "\nconstants:\n");
    KlcConst *item;
    vector_foreach(item, vec) {
        if (!item) continue;
        fprintf(stdout, "  [%2d] = ", i__);
        dump_const(item);
    }
}

static void dump_var(KlcVar *var, KlcFile *klc)
{
    Vector *consts = klc->objs + ITEM_CONST;

    BUF(buf);

    KlcConst *k = vector_get(consts, var->name_index);
    fprintf(stdout, "  var %s ", k->sval);

    k = vector_get(consts, var->type_index);
    type_spec_str_print(k->sval, &buf);

    k = vector_get(consts, var->const_index);
    if (k) {
        fprintf(stdout, "%s = ", BUF_STR(buf));
        dump_const(k);
    } else {
        fprintf(stdout, "%s\n", BUF_STR(buf));
    }

    FINI_BUF(buf);
}

static void dump_vars(Vector *vec, KlcFile *klc)
{
    fprintf(stdout, "\nvariables:\n");

    Vector *consts = klc->objs + ITEM_CONST;

    BUF(buf);

    KlcVar *item;
    vector_foreach(item, vec) {
        if (!item) continue;
        dump_var(item, klc);
    }
    FINI_BUF(buf);
}

static void dump_anns(Vector *vec, KlcFile *klc, int leading_spaces)
{
    KlcAnnot *item;
    vector_foreach(item, vec) {
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

    fprintf(stdout, "  locals: %d\n  codes:\n", code->nlocals);
}

static void dump_func(KlcFunc *fn, KlcFile *klc, int leading_spaces)
{
    Vector *codes = klc->objs + ITEM_CODE;
    KlcCode *code;

    dump_anns(&fn->anns, klc, leading_spaces);

    fprintf(stdout, "%*c", leading_spaces, ' ');

    if (fn->flags & KLC_FLAGS_PUB) {
        fprintf(stdout, "public ");
    }

    BUF(buf);

    KlcConst *k = klc_get_const(klc, fn->name_index);
    fprintf(stdout, "func %s(", k->sval);
    KlcConst *def_val;
    KlcConst *ty_k;

    KlcArgument *arg;
    int i = 0;
    vector_foreach(arg, &fn->args) {
        if (!arg) continue;
        k = klc_get_const(klc, arg->name_index);
        def_val = klc_get_const(klc, arg->const_index);
        if (def_val) {
            if (i != 0) {
                fprintf(stdout, ", %s", k->sval);
            } else {
                fprintf(stdout, "%s", k->sval);
            }
            fprintf(stdout, " = ");
            dump_const_value(def_val);
        } else {
            ty_k = klc_get_const(klc, arg->type_index);
            RESET_BUF(buf);
            type_spec_str_print(ty_k->sval, &buf);
            if (i != 0) {
                fprintf(stdout, ", %s %s", k->sval, BUF_STR(buf));
            } else {
                fprintf(stdout, "%s %s", k->sval, BUF_STR(buf));
            }
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

    code = vector_get(codes, fn->code_index);
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
    fprintf(stdout, "\nfunctions:\n");

    KlcFunc *item;
    vector_foreach(item, vec) {
        if (!item) continue;
        dump_func(item, klc, 2);
    }
}

static void dump_class(Vector *vec, KlcFile *klc)
{
    fprintf(stdout, "\nclasses:\n");

    KlcKlass *item;
    vector_foreach(item, vec) {
        if (!item) continue;

        fprintf(stdout, "%*c", 2, ' ');

        KlcConst *k = klc_get_const(klc, item->name_index);
        if (item->flags & KLC_FLAGS_PUB) {
            fprintf(stdout, "public ");
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
            vector_foreach(tp, &item->tps) {
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
                    vector_foreach(bitem, &tp->bounds) {
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

        if (vector_size(&item->bases) > 1) {
            fprintf(stdout, " : ");
            uint16_t bitem;
            int index = 0;
            BUF(buf);
            vector_foreach(bitem, &item->bases) {
                if (bitem == 0) continue;

                KlcConst *bname = klc_get_const(klc, bitem);
                type_spec_str_print(bname->sval, &buf);
                if (index != 0)
                    fprintf(stdout, " & %s", BUF_STR(buf));
                else
                    fprintf(stdout, "%s", BUF_STR(buf));
                RESET_BUF(buf);
                index++;
            }
            FINI_BUF(buf);
        }

        fprintf(stdout, " {\n");

        KlcVar *field;
        vector_foreach(field, &item->fields) {
            if (!field) continue;
            dump_var(field, klc);
        }

        KlcFunc *fn;
        vector_foreach(fn, &item->methods) {
            if (!fn) continue;
            dump_func(fn, klc, 4);
        }

        fprintf(stdout, "  }\n");
    }
}

static void dump_rt_consts(Vector *vec, KlcFile *klc)
{
    fprintf(stdout, "\nruntime constants:\n");

    int num_rt_consts = klc->num_rt_consts;
    KlcConst *item;
    vector_foreach(item, vec) {
        if (!item) continue;
        if (num_rt_consts <= 0) break;
        fprintf(stdout, "  [%2d] = ", i__);
        dump_const(item);
        --num_rt_consts;
    }
}

static void dump_imports(Vector *vec, KlcFile *klc)
{
    fprintf(stdout, "\nimports:\n");

    KlcImport *item;
    vector_foreach(item, vec) {
        if (!item) continue;
        fprintf(stdout, "  [%2d] = ", i__);
        KlcConst *k = klc_get_rt_const(klc, item->ns_index);
        KlcConst *k2 = klc_get_rt_const(klc, item->sym_index);
        fprintf(stdout, "%s::%s\n", k->sval, k2->sval);
    }
}

static void dump_bytecodes(Vector *vec, Vector *code_vec, KlcFile *klc)
{
    fprintf(stdout, "\ncodes:\n\n");

    uint8_t *codes = NULL;

    KlcByteCode *item;
    vector_foreach(item, vec) {
        if (!item) continue;
        codes = item->codes;
        break;
    }

    ASSERT(codes);

    KlcCode *code;
    vector_foreach(code, code_vec) {
        if (!code) continue;
        KlcConst *k = klc_get_rt_const(klc, code->name_index);
        fprintf(stdout,
                "[%d]@%s(nlocals=%d, max_call_args=%d, start_pc=%d, num_insns=%d)\n",
                i__ - 1, k->sval, code->nlocals, code->max_call_args, code->start_pc,
                code->num_insns);
        bytecode_print(codes, code->start_pc, code->num_insns);
        printf("\n");
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s <klc file>\n", argv[0]);
        return -1;
    }

    init_log(LOG_INFO, NULL, 0);
    init_atom();

    KlcFile *klc = read_klc_file(argv[1], 0);
    if (!klc) {
        fprintf(stderr, "failed to read klc file: %s\n", argv[1]);
        return -1;
    }

    dump_header(klc->magic, klc->version);

    dump_consts(klc->objs + ITEM_CONST);
    dump_vars(klc->objs + ITEM_VAR, klc);
    dump_funcs(klc->objs + ITEM_FUNC, klc);
    dump_class(klc->objs + ITEM_CLASS, klc);

    dump_rt_consts(klc->objs + ITEM_RT_CONST, klc);
    dump_imports(klc->objs + ITEM_IMPORT, klc);
    dump_bytecodes(klc->objs + ITEM_BYTECODE, klc->objs + ITEM_CODE, klc);

    free_klc_file(klc);
    fini_atom();

    return 0;
}

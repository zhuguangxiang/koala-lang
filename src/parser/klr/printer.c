/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "ir.h"

#ifdef __cplusplus
extern "C" {
#endif

char *klr_block_name(KlrBasicBlock *bb)
{
    if (bb->name[0]) return bb->name;
    sprintf(bb->print_name, "bb%d", bb->tag);
    return bb->print_name;
}

static void print_type(TypeSpec *ty, FILE *fp)
{
    BUF(buf);
    type_spec_print(ty, &buf);
    fprintf(fp, " %s", BUF_STR(buf));
    FINI_BUF(buf);
}

#define print_value_type(val, fp) print_type((val)->ts, fp)

void klr_print_name_or_tag(KlrValue *val, FILE *fp)
{
    if (val->kind == KLR_VALUE_NONE) {
        fprintf(fp, "undef");
        return;
    }

    if (val->name[0]) {
        if (val->kind == KLR_VALUE_GLOBAL) {
            fprintf(fp, "@%s", val->name);
        } else {
            fprintf(fp, "%%%s", val->name);
        }
    } else {
        fprintf(fp, "%%%d", val->tag);
    }
}

static void print_const(KlrConst *v, FILE *fp);

static void print_const_item(KlrValue *item, FILE *fp)
{
    if (item->kind == KLR_VALUE_CONST) {
        print_const((KlrConst *)item, fp);
    } else {
        klr_print_name_or_tag(item, fp);
    }
}

static void print_const(KlrConst *v, FILE *fp)
{
    int kind = v->which;
    switch (kind) {
        case CONST_INT:
            fprintf(fp, "%ld", v->ival);
            break;
        case CONST_FLT:
            fprintf(fp, "%lf", v->fval);
            break;
        case CONST_BOOL:
            fprintf(fp, "%s", v->bval ? "true" : "false");
            break;
        case CONST_STR:
            fprintf(fp, "'%s'", v->sval);
            break;
        case CONST_LIST: {
            fprintf(fp, "list[");
            for (int i = 0; i < v->len; i++) {
                KlrValue *item = v->list.items[i];
                print_const_item(item, fp);
                if (i < v->len - 1) fprintf(fp, ", ");
            }
            fprintf(fp, "]");
            break;
        }
        case CONST_TUPLE: {
            fprintf(fp, "tuple(");
            for (int i = 0; i < v->len; i++) {
                KlrValue *item = v->list.items[i];
                print_const_item(item, fp);
                if (i < v->len - 1) fprintf(fp, ", ");
            }
            fprintf(fp, ")");
            break;
        }
        default:
            UNREACHABLE();
            break;
    }
}

static void print_operand(KlrOper *oper, FILE *fp)
{
    KlrOperKind kind = oper->kind;
    if (kind == KLR_OPER_NONE) {
        /* FIXME:
         * number of phi parameters is the same with predecessors number?
         */
        assert(0);
        return;
    }

    if (kind == KLR_OPER_PHI) {
        fprintf(fp, "[ ");
        KlrValue *val = oper->phi.use.ref;
        if (val->kind == KLR_VALUE_CONST) {
            print_const((KlrConst *)val, fp);
        } else {
            klr_print_name_or_tag(val, fp);
        }

        if (val->ts) {
            print_value_type(val, fp);
        }

        fprintf(fp, ", ");
        klr_print_name_or_tag((KlrValue *)oper->phi.bb, fp);
        fprintf(fp, " ]");
    } else {
        KlrValue *val = oper->use.ref;
        if (kind == KLR_OPER_CONST) {
            print_const((KlrConst *)val, fp);
        } else {
            klr_print_name_or_tag(val, fp);
        }
        print_value_type(val, fp);
    }
}

static void print_binary(KlrInsn *insn, char *op, FILE *fp)
{
    klr_print_name_or_tag((KlrValue *)insn, fp);
    fprintf(fp, " = %s ", op);
    print_operand(&insn->opers[0], fp);
    fprintf(fp, ", ");
    print_operand(&insn->opers[1], fp);
}

static void print_ret(KlrInsn *insn, FILE *fp)
{
    fprintf(fp, "ret ");
    print_operand(&insn->opers[0], fp);
}

static void print_ret_void(KlrInsn *insn, FILE *fp) { fprintf(fp, "ret void"); }

static void print_phi(KlrInsn *insn, FILE *fp)
{
    klr_print_name_or_tag((KlrValue *)insn, fp);
    fprintf(fp, " = phi ");
    for (int i = 0; i < insn->num_opers; i++) {
        print_operand(&insn->opers[i], fp);
        if (i < insn->num_opers - 1) fprintf(fp, ", ");
    }
}

static void print_unary(KlrInsn *insn, char *op, FILE *fp)
{
    klr_print_name_or_tag((KlrValue *)insn, fp);
    fprintf(fp, " = %s ", op);
    print_operand(&insn->opers[0], fp);
}

static void print_move(KlrInsn *insn, FILE *fp)
{
    fprintf(fp, "move ");
    print_operand(&insn->opers[0], fp);
    fprintf(fp, ", ");
    print_operand(&insn->opers[1], fp);
}

static void print_push(KlrInsn *insn, FILE *fp)
{
    fprintf(fp, "push ");
    print_operand(&insn->opers[0], fp);
}

static void print_cmp(const char *name, KlrInsn *insn, FILE *fp)
{
    klr_print_name_or_tag((KlrValue *)insn, fp);
    fprintf(fp, " = %s ", name);
    print_operand(&insn->opers[0], fp);
    fprintf(fp, ", ");
    print_operand(&insn->opers[1], fp);
}

static void print_jmp(KlrInsn *insn, FILE *fp)
{
    fprintf(fp, "jmp ");

    KlrValue *val = insn->opers[0].use.ref;
    if (val->name[0])
        fprintf(fp, "label %%%s", val->name);
    else
        fprintf(fp, "label %%bb%d", val->tag);

    if (insn->flags & KLR_INSN_FLAGS_LOOP) fprintf(fp, ", !klr.loop\n");
}

static void print_jmp_cond(const char *name, KlrInsn *insn, FILE *fp)
{
    fprintf(fp, "%s ", name);
    print_operand(&insn->opers[0], fp);
    fprintf(fp, ", ");

    KlrValue *_then = insn->opers[1].use.ref;
    if (_then->name[0])
        fprintf(fp, "label %%%s", _then->name);
    else
        fprintf(fp, "label %%bb%d", _then->tag);

    KlrValue *_else = insn->opers[2].use.ref;
    if (_else->name[0])
        fprintf(fp, ", label %%%s", _else->name);
    else
        fprintf(fp, ", label %%bb%d", _else->tag);
}

static void print_call(KlrInsn *insn, FILE *fp)
{
    KlrValue *fn = insn->opers[0].use.ref;

    if (fn->ts->kind == TYPE_NO_TYPE) {
        fprintf(fp, "call ");
    } else {
        klr_print_name_or_tag((KlrValue *)insn, fp);
        fprintf(fp, " = call ");
    }

    fprintf(fp, "@%s", fn->name);

    if (fn->kind == KLR_VALUE_EXT_FUNC) {
        fprintf(fp, " [ext = true, path = '%s']", ((KlrExtFunc *)fn)->path);
    }

    if (insn->flags & KLR_INSN_FLAGS_CONST) {
        fprintf(fp, " [const]");
    }

    if (insn->num_opers > 1) fprintf(fp, ", ");

    KlrOper *oper;
    for (int i = 1; i < insn->num_opers; i++) {
        if (i != 1) fprintf(fp, ", ");
        oper = &insn->opers[i];
        print_operand(oper, fp);
    }
}

static void print_const_insn(KlrInsn *insn, FILE *fp)
{
    klr_print_name_or_tag((KlrValue *)insn, fp);
    fprintf(fp, " = const ");
    print_operand(&insn->opers[0], fp);
}

static void print_get_global(KlrInsn *insn, FILE *fp)
{
    klr_print_name_or_tag((KlrValue *)insn, fp);
    fprintf(fp, " = get_global ");
    print_operand(&insn->opers[0], fp);
}

static void print_set_global(KlrInsn *insn, FILE *fp)
{
    fprintf(fp, "set_global ");
    print_operand(&insn->opers[0], fp);
    fprintf(fp, ", ");
    print_operand(&insn->opers[1], fp);
}

static void print_local_insn(KlrValue *local, FILE *fp)
{
    klr_print_name_or_tag(local, fp);
    fprintf(fp, " = local");
    KlrInsn *insn = (KlrInsn *)local;
    if (insn->flags & KLR_INSN_FLAGS_CONST) fprintf(fp, " [immutable]");
    print_value_type(local, fp);
}

void klr_print_insn(KlrInsn *insn, FILE *fp)
{
    switch (insn->code) {
        case OP_IR_LOCAL:
            print_local_insn((KlrValue *)insn, fp);
            break;

        case OP_IR_PHI:
            print_phi(insn, fp);
            break;

        case OP_IR_JMP_COND:
            print_jmp_cond("branch", insn, fp);
            break;

        case OP_MOVE:
            print_move(insn, fp);
            break;

        case OP_PUSH:
            print_push(insn, fp);
            break;

        case OP_CONST:
            print_const_insn(insn, fp);
            break;

        case OP_JMP_INT_CMP_LT_IMM:
            print_jmp_cond("jmp_icmplt_imm", insn, fp);
            break;

        case OP_BINARY_ADD:
            print_binary(insn, "add", fp);
            break;

        case OP_BINARY_SUB:
            print_binary(insn, "sub", fp);
            break;

        case OP_BINARY_MUL:
            print_binary(insn, "mul", fp);
            break;

        case OP_BINARY_DIV:
            print_binary(insn, "div", fp);
            break;

        case OP_BINARY_MOD:
            print_binary(insn, "mod", fp);
            break;

        case OP_BINARY_AND:
            print_binary(insn, "and", fp);
            break;

        case OP_BINARY_OR:
            print_binary(insn, "or", fp);
            break;

        case OP_BINARY_XOR:
            print_binary(insn, "xor", fp);
            break;

        case OP_UNARY_NOT:
            print_unary(insn, "not", fp);
            break;

        case OP_BINARY_SHL:
            print_binary(insn, "shl", fp);
            break;

        case OP_BINARY_SHR:
            print_binary(insn, "shr", fp);
            break;

        case OP_CALL:
            print_call(insn, fp);
            break;

        case OP_BINARY_CMP_EQ:
            print_cmp("cmpeq", insn, fp);
            break;

        case OP_BINARY_CMP_NE:
            print_cmp("cmpneq", insn, fp);
            break;

        case OP_BINARY_CMP_LT:
            print_cmp("cmplt", insn, fp);
            break;

        case OP_BINARY_CMP_GT:
            print_cmp("cmpgt", insn, fp);
            break;

        case OP_BINARY_CMP_LE:
            print_cmp("cmple", insn, fp);
            break;

        case OP_BINARY_CMP_GE:
            print_cmp("cmpge", insn, fp);
            break;

        case OP_JMP:
            print_jmp(insn, fp);
            break;

        case OP_JMP_TRUE:
            print_jmp_cond("jmp_true", insn, fp);
            break;

        case OP_RETURN:
            print_ret(insn, fp);
            break;

        case OP_RETURN_NONE:
            print_ret_void(insn, fp);
            break;

        case OP_GET_GLOBAL:
            print_get_global(insn, fp);
            break;

        case OP_SET_GLOBAL:
            print_set_global(insn, fp);
            break;

        case OP_LAND:
            print_binary(insn, "land", fp);
            break;

        case OP_LOR:
            print_binary(insn, "lor", fp);
            break;

        case OP_LNOT:
            print_unary(insn, "lnot", fp);
            break;

        default:
            printf("%d\n", insn->code);
            UNREACHABLE();
            break;
    }
}

static void print_preds(KlrBasicBlock *bb, int spaces, FILE *fp)
{
    fprintf(fp, "%*s = ", spaces, ";; preds");

    KlrBasicBlock *pred;
    int i = 0;
    bb_pred_foreach(pred, bb) {
        if (i++ == 0) {
            if (pred->name[0]) {
                fprintf(fp, "%%%s", pred->name);
            } else {
                fprintf(fp, "%%bb%d", pred->tag);
            }
        } else {
            if (pred->name[0]) {
                fprintf(fp, ", %%%s", pred->name);
            } else {
                fprintf(fp, ", %%bb%d", pred->tag);
            }
        }
    }
}

static void print_block(KlrBasicBlock *bb, FILE *fp)
{
    int used = 0;

    if (bb->name[0])
        used = fprintf(fp, "  %%%s:", bb->name);
    else
        used = fprintf(fp, "  %%bb%d:", bb->tag);

    // print predecessors
    if (edge_in_empty(bb)) {
        fprintf(fp, "%*s", 50 - used + 12, ";; No preds!");
    } else {
        KlrFunc *fn = bb->func;
        KlrEdge *edge = edge_in_first(bb);
        if (edge->src != fn->sbb) print_preds(bb, 50 - used + 8, fp);
    }

    KlrInsn *insn;
    insn_foreach(insn, bb) {
        fprintf(fp, "\n        ");
        klr_print_insn(insn, fp);
    }
}

static void update_tags(KlrFunc *fn)
{
    fn->tag = 0;
    fn->bb_tag = 0;

    KlrParam *param;
    vector_foreach(param, &fn->params) {
        if (!param->name[0]) param->tag = fn->tag++;
    }

    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        if (!bb->name[0]) bb->tag = fn->bb_tag++;
        KlrInsn *insn;
        insn_foreach(insn, bb) {
            if (!insn_has_value(insn)) continue;
            if (!insn->name[0]) insn->tag = fn->tag++;
        }
    }
}

static void print_bb_edges(KlrBasicBlock *bb, FILE *fp)
{
    if (bb->name[0])
        fprintf(fp, "%%%s:\n", bb->name);
    else
        fprintf(fp, "%%bb%d:\n", bb->tag);

    fprintf(fp, "\tpreds: ");
    if (edge_in_empty(bb)) {
        fprintf(fp, "\tno\n");
    } else {
        int i = 0;
        KlrBasicBlock *pred;
        bb_pred_foreach(pred, bb) {
            if (i++ == 0) {
                if (pred->name[0]) {
                    fprintf(fp, "\t%%%s", pred->name);
                } else {
                    fprintf(fp, "\t%%bb%d", pred->tag);
                }
            } else {
                if (pred->name[0]) {
                    fprintf(fp, ", %%%s\n", pred->name);
                } else {
                    fprintf(fp, ", %%bb%d\n", pred->tag);
                }
            }
        }

        if (i == 1) fprintf(fp, "\n");
    }

    fprintf(fp, "\tsuccs: ");
    if (edge_out_empty(bb)) {
        fprintf(fp, "\tno\n");
    } else {
        int i = 0;
        KlrBasicBlock *dst;
        bb_succ_foreach(dst, bb) {
            if (i++ == 0) {
                if (dst->name[0]) {
                    fprintf(fp, "\t%%%s", dst->name);
                } else {
                    fprintf(fp, "\t%%bb%d", dst->tag);
                }
            } else {
                if (dst->name[0]) {
                    fprintf(fp, ", %%%s\n", dst->name);
                } else {
                    fprintf(fp, ", %%bb%d\n", dst->tag);
                }
            }
        }
        if (i == 1) fprintf(fp, "\n");
    }

    fprintf(fp, "\n");
}

void klr_print_cfg(KlrFunc *func, FILE *fp)
{
    fprintf(fp, "\nbasic blocks:\n\n");
    print_bb_edges(func->sbb, fp);
    KlrBasicBlock *bb;
    basic_block_foreach(bb, func) {
        print_bb_edges(bb, fp);
    }

    print_bb_edges(func->ebb, fp);
}

void klr_print_func(KlrFunc *func, FILE *fp)
{
    update_tags(func);

    fprintf(fp, "  func @%s", func->name);

    fprintf(fp, "(");
    KlrParam *param;
    vector_foreach(param, &func->params) {
        if (i__ == 0) {
            fprintf(fp, "param ");
        } else {
            fprintf(fp, ", param ");
        }
        klr_print_name_or_tag((KlrValue *)param, fp);
        print_value_type(param, fp);
    }

    fprintf(fp, ")");

    TypeSpec *ret = func->ts;
    if (ret) {
        print_type(ret, fp);
    }

    fprintf(fp, " {");

    // print basic blocks directly(not cfg)
    KlrBasicBlock *bb;
    basic_block_foreach(bb, func) {
        fprintf(fp, "\n");
        print_block(bb, fp);
        fprintf(fp, "\n");
    }

    /* append a basic block to the end of a function */
    fprintf(fp, "  }\n");
}

void klr_print_module(KlrModule *m, FILE *fp)
{
    fprintf(fp, "module @%s {\n", m->name);

    KlrGlobal *g;
    vector_foreach(g, &m->globals) {
        if (!g) continue;
        fprintf(fp, "  global @%s", g->name);
        if (g->mutable)
            fprintf(fp, " [mutable]");
        else
            fprintf(fp, " [immutable]");
        print_value_type(g, fp);
        fprintf(fp, "\n");
    }

    // KlrExtSym *ext;
    // vector_foreach(ext, &m->ext_syms) {
    //     if (ext->kind == KLR_VALUE_EXT_FUNC) {
    //         fprintf(fp, "  ext func @%s.%s", ext->path, ext->name);
    //         print_type(ext->ts, fp);
    //         fprintf(fp, "\n");
    //     } else if (ext->kind == KLR_VALUE_EXT_GLOBAL) {
    //         fprintf(fp, "  ext global @%s.%s", ext->path, ext->name);
    //         print_type(ext->ts, fp);
    //         fprintf(fp, "\n");
    //     } else {
    //         fprintf(fp, "  ext sym @%s.%s", ext->path, ext->name);
    //     }
    // }

    KlrFunc *fn;
    vector_foreach(fn, &m->functions) {
        klr_print_func(fn, fp);
    }

    fprintf(fp, "}\n");
}

#ifdef __cplusplus
}
#endif

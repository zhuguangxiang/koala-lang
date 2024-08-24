/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "ir.h"

#ifdef __cplusplus
extern "C" {
#endif

char *klr_block_name(KlrBasicBlock *bb)
{
    if (bb->name[0]) return bb->name;
    static char sbuf[32];
    sprintf(sbuf, "bb%d", bb->tag);
    return sbuf;
}

static void print_type(TypeDesc *ty, FILE *fp)
{
    BUF(buf);
    desc_to_str(ty, &buf);
    fprintf(fp, " %s", BUF_STR(buf));
    FINI_BUF(buf);
}

#define print_value_type(val, fp) print_type((val)->desc, fp)

void klr_print_name_or_tag(KlrValue *val, FILE *fp)
{
    if (val->kind == KLR_VALUE_NONE) {
        fprintf(fp, "undef");
        return;
    }

    if (val->name[0]) {
        fprintf(fp, "%%%s", val->name);
    } else {
        fprintf(fp, "%%%d", val->tag);
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
            fprintf(fp, "%s", v->bval ? "True" : "False");
            break;
        case CONST_STR:
            fprintf(fp, "%s", v->sval);
            break;
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

        if (val->desc) {
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

static void print_store(KlrInsn *insn, FILE *fp)
{
    fprintf(fp, "store ");
    print_operand(&insn->opers[0], fp);
    fprintf(fp, ", ");
    print_operand(&insn->opers[1], fp);
}

static void print_load(KlrInsn *insn, FILE *fp)
{
    klr_print_name_or_tag((KlrValue *)insn, fp);
    fprintf(fp, " = load ");
    print_operand(&insn->opers[0], fp);
}

static void print_phi(KlrInsn *insn, FILE *fp)
{
    klr_print_name_or_tag((KlrValue *)insn, fp);
    fprintf(fp, " = phi ");
    for (int i = 0; i < insn->num_opers; i++) {
        print_operand(&insn->opers[i], fp);
        if (i < insn->num_opers - 1) fprintf(fp, ", ");
    }
}

static void print_unary_operand(const char *name, KlrInsn *insn, FILE *fp)
{
    fprintf(fp, "%s ", name);
    print_operand(&insn->opers[0], fp);
}

static void print_binary_operands(const char *name, KlrInsn *insn, FILE *fp)
{
    fprintf(fp, "%s ", name);
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

    if (insn->opers[1].kind != KLR_OPER_NONE) {
        print_operand(&insn->opers[1], fp);
        fprintf(fp, ", ");
    }

    KlrValue *_then = insn->opers[2].use.ref;
    if (_then->name[0])
        fprintf(fp, "label %%%s", _then->name);
    else
        fprintf(fp, "label %%bb%d", _then->tag);

    KlrValue *_else = insn->opers[3].use.ref;
    if (_else->name[0])
        fprintf(fp, ", label %%%s", _else->name);
    else
        fprintf(fp, ", label %%bb%d", _else->tag);
}

static void print_call(KlrInsn *insn, FILE *fp)
{
    klr_print_name_or_tag((KlrValue *)insn, fp);
    fprintf(fp, " = call ");
    KlrValue *fn = insn->opers[0].use.ref;
    fprintf(fp, "@%s", fn->name);
    if (insn->num_opers > 0) fprintf(fp, ", ");
    KlrOper *oper;
    for (int i = 1; i < insn->num_opers; i++) {
        if (i != 1) fprintf(fp, ", ");
        oper = &insn->opers[i];
        print_operand(oper, fp);
    }
}

void klr_print_insn(KlrInsn *insn, FILE *fp)
{
    switch (insn->code) {
        case OP_IR_LOAD:
            print_load(insn, fp);
            break;

        case OP_IR_STORE:
            print_store(insn, fp);
            break;

        case OP_IR_PHI:
            print_phi(insn, fp);
            break;

        case OP_IR_JMP_COND:
            print_jmp_cond("branch", insn, fp);
            break;

        case OP_MOVE:
            print_binary_operands("move", insn, fp);
            break;

        case OP_PUSH:
            print_push(insn, fp);
            break;

        case OP_CONST_INT_M1:
            print_unary_operand("const_int_m1", insn, fp);
            break;

        case OP_CONST_INT_0:
            print_unary_operand("const_int_0", insn, fp);
            break;

        case OP_CONST_INT_IMM8:
            print_binary_operands("const_int_imm8", insn, fp);
            break;

        case OP_CONST_LOAD:
            print_binary_operands("const_load", insn, fp);
            break;

        case OP_JMP_INT_CMP_LT_IMM8:
            print_jmp_cond("jmp_icmplt_imm8", insn, fp);
            break;

        case OP_BINARY_ADD:
            print_binary(insn, "add", fp);
            break;

        case OP_BINARY_SUB:
            print_binary(insn, "sub", fp);
            break;

        case OP_CALL:
            print_call(insn, fp);
            break;

        case OP_BINARY_CMP_LT:
            print_cmp("cmplt", insn, fp);
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

        default:
            printf("%d\n", insn->code);
            UNREACHABLE();
            break;
    }
}

static void print_local(KlrLocal *local, FILE *fp)
{
    fprintf(fp, "\n        ");
    fprintf(fp, "local ");
    klr_print_name_or_tag((KlrValue *)local, fp);
    print_value_type(local, fp);
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

    KlrFunc *func = bb->func;
    // first block, print locals
    if (list_first(&func->bb_list, KlrBasicBlock, link) == bb) {
        KlrLocal **local;
        Vector *vec = &func->locals;
        vector_foreach(local, vec) {
            if (klr_value_used(*local)) print_local((*local), fp);
        }
    }

    KlrInsn *insn;
    insn_foreach(insn, bb) {
        fprintf(fp, "\n        ");
        klr_print_insn(insn, fp);
    }
}

static int need_update_tag(KlrInsn *insn) { return insn_has_value(insn); }

static void update_tags(KlrFunc *fn)
{
    fn->tag = 0;
    fn->bb_tag = 0;

    KlrParam **param;
    vector_foreach(param, &fn->params) {
        if (!(*param)->name[0]) (*param)->tag = fn->tag++;
    }

    KlrLocal **local;
    Vector *vec = &fn->locals;
    vector_foreach(local, vec) {
        if (!(*local)->name[0]) (*local)->tag = fn->tag++;
    }

    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        if (!bb->name[0]) bb->tag = fn->bb_tag++;
        KlrInsn *insn;
        insn_foreach(insn, bb) {
            if (!need_update_tag(insn)) continue;
            if (!insn->name[0]) insn->tag = fn->tag++;
        }
    }
}

void klr_print_var_use(KlrFunc *func, FILE *fp)
{
    KlrLocal *local;
    Vector *vec = &func->locals;
    KlrUse *use;
    int j = 0;
    vector_foreach(local, vec) {
        fprintf(fp, "local: %s uses:\n", local->name);
        j = 1;
        use_foreach(use, local) {
            fprintf(fp, "\t [%d] %d\n", j++, use->insn->code);
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
    KlrParam **param;
    vector_foreach(param, &func->params) {
        if (i__ == 0) {
            fprintf(fp, "param ");
        } else {
            fprintf(fp, ", param ");
        }
        klr_print_name_or_tag(*(KlrValue **)param, fp);
        print_value_type((*param), fp);
    }

    fprintf(fp, ")");

    TypeDesc *ret = func->desc;
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

void klr_print_module(KlrModule *m, FILE *fp) {}

#ifdef __cplusplus
}
#endif

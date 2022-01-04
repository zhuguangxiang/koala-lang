/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "klvm.h"

#ifdef __cplusplus
extern "C" {
#endif

static void print_type(TypeDesc *ty, FILE *fp)
{
    BUF(buf);
    desc_to_str(ty, &buf);
    fprintf(fp, "%s", BUF_STR(buf));
    FINI_BUF(buf);
}

static void klvm_print_var(KLVMVar *var, FILE *fp)
{
    fprintf(fp, "var @%s ", var->name);
    print_type(var->type, fp);
    fprintf(fp, "\n\n");
}

static void print_label_or_tag(KLVMValue *val, FILE *fp)
{
    if (val->kind == KLVM_VALUE_VAR || val->kind == KLVM_VALUE_FUNC) {
        fprintf(fp, "@%s", val->name);
        return;
    }

    if (val->name[0]) {
        fprintf(fp, "%%%s", val->name);
    } else {
        fprintf(fp, "%%%d", val->tag);
    }
}

static void print_const(KLVMConst *v, FILE *fp)
{
    TypeKind kind = v->type->kind;
    switch (kind) {
        case TYPE_I8_KIND:
        case TYPE_I16_KIND:
        case TYPE_I32_KIND:
            fprintf(fp, "%d", v->ival);
            break;
        case TYPE_I64_KIND:
            fprintf(fp, "%ld", v->i64val);
            break;
        default:
            break;
    }
}

static void print_use(KLVMFunc *fn, KLVMUse *use, FILE *fp)
{
    KLVMValue *val = use->parent;
    print_label_or_tag(val, fp);
    if (val->kind != KLVM_VALUE_FUNC) {
        fprintf(fp, " ");
        print_type(val->type, fp);
    }
}

static void print_operand(KLVMFunc *fn, KLVMOper *oper, FILE *fp)
{
    KLVMOperKind kind = oper->kind;
    if (kind == KLVM_OPER_CONST) {
        KLVMConst *val = oper->konst;
        print_const(val, fp);
        fprintf(fp, " ");
        print_type(val->type, fp);
    } else {
        print_use(fn, oper->use, fp);
    }
}

static void print_local(KLVMLocal *local, KLVMFunc *fn, FILE *fp)
{
    fprintf(fp, "\n  ");
    fprintf(fp, "local ");
    print_label_or_tag((KLVMValue *)local, fp);
    fprintf(fp, " ");
    print_type(local->type, fp);
}

static void print_copy(KLVMInst *inst, KLVMFunc *fn, FILE *fp)
{
    KLVMValue *val = inst->operands[0].use->parent;
    print_label_or_tag(val, fp);
    fprintf(fp, " = ");
    print_operand(fn, &inst->operands[1], fp);
}

static void print_binary(KLVMInst *inst, char *opname, KLVMFunc *fn, FILE *fp)
{
    print_label_or_tag((KLVMValue *)inst, fp);
    fprintf(fp, " = %s ", opname);
    print_operand(fn, &inst->operands[0], fp);
    fprintf(fp, ", ");
    print_operand(fn, &inst->operands[1], fp);
}

static void print_call(KLVMInst *inst, KLVMFunc *fn, FILE *fp)
{
    print_label_or_tag((KLVMValue *)inst, fp);
    fprintf(fp, " = call ");
    print_operand(fn, &inst->operands[0], fp);
    fprintf(fp, "(");
    KLVMOper *oper;
    for (int i = 1; i < inst->num_ops; i++) {
        oper = &inst->operands[i];
        print_operand(fn, oper, fp);
    }
    fprintf(fp, ") ");
    print_type(inst->type, fp);
}

static void print_ret(KLVMInst *inst, KLVMFunc *fn, FILE *fp)
{
    fprintf(fp, "ret ");
    print_operand(fn, &inst->operands[0], fp);
}

static void print_jmp(KLVMInst *inst, FILE *fp)
{
    fprintf(fp, "br ");

    KLVMValue *val = inst->operands[0].use->parent;
    if (val->name[0])
        fprintf(fp, "label %%%s", val->name);
    else
        fprintf(fp, "label %%bb%d", val->tag);
}

static void print_condjmp(KLVMInst *inst, KLVMFunc *fn, FILE *fp)
{
    if (inst->opcode == OP_JMP_LT) {
        fprintf(fp, "blt");
    } else if (inst->opcode == OP_JMP_LE) {
        fprintf(fp, "ble");
    } else {
        fprintf(fp, "br");
    }

    assert(inst->num_ops == 4);

    fprintf(fp, " ");
    print_operand(fn, &inst->operands[0], fp);
    fprintf(fp, ", ");
    print_operand(fn, &inst->operands[1], fp);

    KLVMValue *val = inst->operands[2].use->parent;
    if (val->name[0])
        fprintf(fp, ", label %%%s", val->name);
    else
        fprintf(fp, ", label %%bb%d", val->tag);

    val = inst->operands[3].use->parent;
    if (val->name[0])
        fprintf(fp, ", label %%%s", val->name);
    else
        fprintf(fp, ", label %%bb%d", val->tag);
}

void klvm_print_inst(KLVMFunc *fn, KLVMInst *inst, FILE *fp)
{
    switch (inst->opcode) {
        case OP_MOVE:
        case OP_CONST_I8:
        case OP_CONST_I16:
        case OP_CONST_I32:
        case OP_CONST_F32:
        case OP_CONST_BOOL:
        case OP_CONST_CHAR:
        case OP_LOAD_CONST:
            print_copy(inst, fn, fp);
            break;
        case OP_ADD:
            print_binary(inst, "add", fn, fp);
            break;
        case OP_SUB:
            print_binary(inst, "sub", fn, fp);
            break;
        case OP_CMP_LE:
            print_binary(inst, "cmple", fn, fp);
            break;
        case OP_CMP_LT:
            print_binary(inst, "cmplt", fn, fp);
            break;
        case OP_CMP_GE:
            print_binary(inst, "cmpge", fn, fp);
            break;
        case OP_CALL:
            print_call(inst, fn, fp);
            break;
        case OP_RET:
            print_ret(inst, fn, fp);
            break;
        case OP_JMP:
            print_jmp(inst, fp);
            break;
        case OP_JMP_EQ:
        case OP_JMP_NE:
        case OP_JMP_GT:
        case OP_JMP_GE:
        case OP_JMP_LT:
        case OP_JMP_LE:
            print_condjmp(inst, fn, fp);
            break;
        default:
            assert(0);
            break;
    }
}

static void print_preds(KLVMBasicBlock *bb, int spaces, FILE *fp)
{
    fprintf(fp, "%*s = ", spaces, ";; preds");

    KLVMBasicBlock *src;
    KLVMEdge *edge;
    int i = 0;
    edge_in_foreach(edge, bb, {
        src = edge->src;
        if (i++ == 0) {
            if (src->name[0]) {
                fprintf(fp, "%%%s", src->name);
            } else {
                fprintf(fp, "%%bb%d", src->tag);
            }
        } else {
            if (src->name[0]) {
                fprintf(fp, ", %%%s", src->name);
            } else {
                fprintf(fp, ", %%bb%d", src->tag);
            }
        }
    });
}

static void print_block(KLVMBasicBlock *bb, FILE *fp)
{
    int used = 0;

    if (bb->name[0])
        used = fprintf(fp, "%%%s:", bb->name);
    else
        used = fprintf(fp, "%%bb%d:", bb->tag);

    // print predecessors
    if (edge_in_empty(bb)) {
        fprintf(fp, "%*s", 50 - used + 12, ";; No preds!");
    } else {
        KLVMFunc *fn = bb->func;
        KLVMEdge *edge = edge_in_first(bb);
        if (edge->src != fn->sbb) print_preds(bb, 50 - used + 8, fp);
    }

    // print locals
    KLVMLocal *local;
    local_foreach(local, bb, { print_local(local, bb->func, fp); });

    KLVMInst *inst;
    inst_foreach(inst, bb, {
        fprintf(fp, "\n  ");
        klvm_print_inst(bb->func, inst, fp);
    });
}

static void update_tags(KLVMFunc *fn)
{
    fn->val_tag = 0;
    fn->bb_tag = 0;

    KLVMArgument **arg;
    vector_foreach(arg, &fn->args, {
        if (!(*arg)->name[0]) (*arg)->tag = fn->val_tag++;
    });

    KLVMBasicBlock *bb;
    KLVMLocal *local;
    KLVMInst *inst;
    basic_block_foreach(bb, fn, {
        if (!bb->name[0]) bb->tag = fn->bb_tag++;

        local_foreach(local, bb, {
            if (!local->name[0]) local->tag = fn->val_tag++;
        });

        inst_foreach(inst, bb, {
            if (inst->type && !inst->name[0]) inst->tag = fn->val_tag++;
        });
    });
}

static void print_func(KLVMFunc *fn, FILE *fp)
{
    update_tags(fn);

    fprintf(fp, "func @%s", fn->name);

    fprintf(fp, "(");
    KLVMArgument **arg;
    vector_foreach(arg, &fn->args, {
        if (i != 0) fprintf(fp, ", ");
        print_label_or_tag((KLVMValue *)(*arg), fp);
        fprintf(fp, " ");
        print_type((*arg)->type, fp);
    });

    TypeDesc *ret = proto_ret(fn->type);
    if (ret) {
        fprintf(fp, ") ");
        print_type(ret, fp);
    } else {
        fprintf(fp, ")");
    }

    fprintf(fp, " {");

    // print basic blocks directly(not cfg)
    KLVMBasicBlock *bb;
    basic_block_foreach(bb, fn, {
        fprintf(fp, "\n");
        print_block(bb, fp);
        fprintf(fp, "\n");
    });

    fprintf(fp, "}\n\n");
}

void klvm_print_module(KLVMModule *m, FILE *fp)
{
    fprintf(fp, "\n@__name__ := \"%s\"\n\n", m->name);

    KLVMVar **var;
    vector_foreach(var, &m->variables, { klvm_print_var(*var, fp); });

    KLVMFunc **fn;
    vector_foreach(fn, &m->functions, { print_func(*fn, fp); });
}

void klvm_print_liveness(KLVMFunc *fn, FILE *fp)
{
    fprintf(fp, "Liveness `@%s`:\n", fn->name);
    fprintf(fp, "  Instructions: %d\n", fn->num_insts);
    fprintf(fp, "  registers: %d\n", fn->vregs);
    KLVMBasicBlock *bb;
    basic_block_foreach(bb, fn, {
        fprintf(fp, "\n");
        if (bb->name[0])
            fprintf(fp, "  BB of `%%%s`:\n", bb->name);
        else
            fprintf(fp, "  BB of `%%bb%d`:\n", bb->tag);
        fprintf(fp, "    Instructions: %d\n", bb->num_insts);
        fprintf(fp, "    Range: [%d, %d)\n", bb->start, bb->end);

        fprintf(fp, "    Uses: ");
        bits_show(bb->use_set, fp);

        fprintf(fp, "\n    Defs: ");
        bits_show(bb->def_set, fp);

        fprintf(fp, "\n    Live-Ins: ");
        bits_show(bb->live_in_set, fp);

        fprintf(fp, "\n    Live-Outs: ");
        bits_show(bb->live_out_set, fp);

        fprintf(fp, "\n");
    });
    fprintf(fp, "\n");
}

#ifdef __cplusplus
}
#endif

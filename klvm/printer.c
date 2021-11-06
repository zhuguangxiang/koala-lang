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

// https://cs.lmu.edu/~ray/notes/ir/

static void print_type(TypeDesc *ty, FILE *fp)
{
    BUF(buf);
    desc_to_str(ty, &buf);
    fprintf(fp, "%s", BUF_STR(buf));
    FINI_BUF(buf);
}

static void klvm_print_var(KLVMVar *var, FILE *fp)
{
    fprintf(fp, "var %s", var->name);
    print_type(var->type, fp);
}

static inline int val_tag(KLVMFunc *fn, KLVMValDef *def)
{
    return def->tag;
}

static inline int bb_tag(KLVMBasicBlock *bb)
{
    /*
    if (bb->tag < 0) {
        bb->tag = bb->fn->bb_tag_index++;
    }
    return bb->tag;
    */
    return 0;
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

static void print_label_or_tag(KLVMValDef *def, KLVMFunc *fn, FILE *fp)
{
    if (def->label[0])
        fprintf(fp, "%%%s", def->label);
    else {
        fprintf(fp, "%%%d", val_tag(fn, def));
    }
}

static void print_reg(KLVMFunc *fn, KLVMUse *reg, FILE *fp)
{
    KLVMValDef *val = reg->interval->owner;
    print_label_or_tag(val, fn, fp);
    fprintf(fp, " ");
    print_type(val->type, fp);
}

static void print_operand(KLVMFunc *fn, KLVMOper *oper, FILE *fp)
{
    KLVMOperKind kind = oper->kind;
    switch (kind) {
        case KLVM_OPER_CONST: {
            KLVMConst *val = oper->konst;
            print_const(val, fp);
            fprintf(fp, " ");
            print_type(val->type, fp);
            break;
        }
        case KLVM_OPER_VAR: {
            assert(0);
            break;
        }
        case KLVM_OPER_REG: {
            print_reg(fn, oper->reg, fp);
            break;
        }
        case KLVM_OPER_FUNC: {
            fprintf(fp, "@%s", oper->func->name);
            break;
        }
        case KLVM_OPER_BLOCK: {
            break;
        }
        default: {
            panic("invalid operand kind");
            break;
        }
    }
}

static void print_copy(KLVMInst *inst, KLVMFunc *fn, FILE *fp)
{
    KLVMUse *reg = inst->operands[0].reg;
    KLVMValDef *def = reg->interval->owner;
    print_label_or_tag(def, fn, fp);
    fprintf(fp, " = ");
    print_operand(fn, &inst->operands[1], fp);
}

static void print_binary(KLVMInst *inst, char *opname, KLVMFunc *fn, FILE *fp)
{
    KLVMUse *reg = inst->operands[0].reg;
    KLVMValDef *def = reg->interval->owner;
    print_label_or_tag(def, fn, fp);
    fprintf(fp, " = %s ", opname);
    print_operand(fn, &inst->operands[1], fp);
    fprintf(fp, ", ");
    print_operand(fn, &inst->operands[2], fp);
}

static void print_call(KLVMInst *inst, KLVMFunc *fn, FILE *fp)
{
    KLVMUse *reg = inst->operands[0].reg;
    KLVMValDef *def = reg->interval->owner;
    print_label_or_tag(def, fn, fp);
    print_type(def->type, fp);
    fprintf(fp, " = call ");
    print_operand(fn, &inst->operands[0], fp);
    fprintf(fp, "(");
    KLVMOper *oper;
    for (int i = 1; i < inst->num_ops; i++) {
        oper = &inst->operands[i];
        print_operand(fn, oper, fp);
    }
    fprintf(fp, ")");
}

static void print_ret(KLVMInst *inst, KLVMFunc *fn, FILE *fp)
{
    fprintf(fp, "ret ");
    print_operand(fn, &inst->operands[0], fp);
}

static void print_jmp(KLVMInst *inst, FILE *fp)
{
    fprintf(fp, "jmp ");

    KLVMBasicBlock *bb = inst->operands[0].block;
    if (bb->label[0])
        fprintf(fp, "label %%%s", bb->label);
    else
        fprintf(fp, "label %%bb%d", bb_tag(bb));
}

static void print_condjmp(KLVMInst *inst, KLVMFunc *fn, FILE *fp)
{
    fprintf(fp, "br ");
    print_operand(fn, &inst->operands[0], fp);

    KLVMBasicBlock *bb;

    bb = inst->operands[1].block;
    if (bb->label[0])
        fprintf(fp, ", label %%%s", bb->label);
    else
        fprintf(fp, ", label %%bb%d", bb_tag(bb));

    bb = inst->operands[2].block;
    if (bb->label[0])
        fprintf(fp, ", label %%%s", bb->label);
    else
        fprintf(fp, ", label %%bb%d", bb_tag(bb));
}

void klvm_print_inst(KLVMFunc *fn, KLVMInst *inst, FILE *fp)
{
    switch (inst->opcode) {
        case KLVM_OP_COPY:
            print_copy(inst, fn, fp);
            break;
        case KLVM_OP_ADD:
            print_binary(inst, "add", fn, fp);
            break;
        case KLVM_OP_SUB:
            print_binary(inst, "sub", fn, fp);
            break;
        case KLVM_OP_CMP_LE:
            print_binary(inst, "cmple", fn, fp);
            break;
        case KLVM_OP_CALL:
            print_call(inst, fn, fp);
            break;
        case KLVM_OP_RET:
            print_ret(inst, fn, fp);
            break;
        case KLVM_OP_JMP:
            print_jmp(inst, fp);
            break;
        case KLVM_OP_COND_JMP:
            print_condjmp(inst, fn, fp);
            break;
        default:
            break;
    }
}

static void print_preds(KLVMBasicBlock *bb, int spaces, FILE *fp)
{
    fprintf(fp, "%*s = %%", spaces, ";; preds");

    KLVMBasicBlock *src;
    KLVMEdge *edge;
    int i = 0;
    edge_in_foreach(edge, &bb->in_edges, {
        src = edge->src;
        if (i++ == 0)
            fprintf(fp, "%s", src->label);
        else
            fprintf(fp, ", %s", src->label);
    });
}

static void print_block(KLVMBasicBlock *bb, FILE *fp)
{
    int cnt;
    if (bb->label[0]) {
        cnt = fprintf(fp, "%s:", bb->label);
    } else {
        cnt = fprintf(fp, "bb%d:", bb_tag(bb));
    }

    // print predecessors
    if (list_empty(&bb->in_edges)) {
        fprintf(fp, "%*s", 48 - cnt, ";; No preds!");
    } else {
        KLVMFunc *fn = bb->func;
        KLVMEdge *edge = list_first(&bb->in_edges, KLVMEdge, in_link);
        if (edge->src != fn->sbb) print_preds(bb, 64 - cnt, fp);
    }

    KLVMInst *inst;
    inst_foreach(inst, &bb->inst_list, {
        fprintf(fp, "\n  ");
        klvm_print_inst(bb->func, inst, fp);
    });
}

static void print_func(KLVMFunc *fn, FILE *fp)
{
    fprintf(fp, "func %s", fn->name);

    fprintf(fp, "(");
    KLVMValDef *def;
    for (int i = 0; i < fn->num_params; i++) {
        vector_get(&fn->locals, i, &def);
        if (i != 0) fprintf(fp, ", ");
        if (def->label[0])
            fprintf(fp, "%s ", def->label);
        else
            fprintf(fp, "%%%d ", val_tag(fn, def));
        print_type(def->type, fp);
    }

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
    basic_block_foreach(bb, &fn->bb_list, {
        fprintf(fp, "\n");
        print_block(bb, fp);
    });

    fprintf(fp, "\n}\n");
}

void klvm_print_module(KLVMModule *m, FILE *fp)
{
    fprintf(fp, "\n__name__ := \"%s\"\n\n", m->name);

    KLVMVar **var;
    vector_foreach(var, &m->variables, {
        klvm_print_var(*var, fp);
        fprintf(fp, "\n");
    });

    KLVMFunc **fn;
    vector_foreach(fn, &m->functions, {
        print_func(*fn, fp);
        fprintf(fp, "\n");
    });
}

#ifdef __cplusplus
}
#endif

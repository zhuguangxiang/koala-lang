/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "ir.h"

#ifdef __cplusplus
extern "C" {
#endif

static void print_type(TypeSpec *ty, FILE *fp)
{
    BUF(buf);
    type_spec_print(ty, &buf);
    fprintf(fp, " %s", BUF_STR(buf));
    FINI_BUF(buf);
}

#define print_value_type(val, fp) print_type((val)->ts, fp)

static void print_const(KlrConst *v, FILE *fp);

static void print_const_item(KlrValue *item, FILE *fp)
{
    if (item->kind == KLR_VALUE_CONST) {
        print_const((KlrConst *)item, fp);
    } else {
        klr_print_value_name(item, fp);
    }
}

static void print_const(KlrConst *v, FILE *fp)
{
    int kind = v->which;
    switch (kind) {
        case CONST_NONE:
            fprintf(fp, "none");
            break;
        case CONST_INT:
            fprintf(fp, "%ld", v->ival);
            break;
        case CONST_UINT:
            fprintf(fp, "%lu", (uint64_t)v->ival);
            break;
        case CONST_FLT:
            fprintf(fp, "%lf", v->fval);
            break;
        case CONST_BOOL:
            fprintf(fp, "%s", v->bval ? "true" : "false");
            break;
        case CONST_STR:
            fprintf(fp, "'");
            BUF(buf);
            escape_str(v->sval, &buf);
            fprintf(fp, "%s'", BUF_STR(buf));
            FINI_BUF(buf);
            break;
        case CONST_LIST: {
            fprintf(fp, "list[");
            int len = vector_size(v->list);
            KlrValue *item;
            vector_foreach(item, v->list) {
                print_const_item(item, fp);
                if (i__ < len - 1) fprintf(fp, ", ");
            }
            fprintf(fp, "]");
            break;
        }
        case CONST_TUPLE: {
            fprintf(fp, "tuple(");
            int len = vector_size(v->list);
            KlrValue *item;
            vector_foreach(item, v->list) {
                print_const_item(item, fp);
                if (i__ < len - 1) fprintf(fp, ", ");
            }
            fprintf(fp, ")");
            break;
        }
        case CONST_RANGE: {
            fprintf(fp, "range(");
            int len = vector_size(v->list);
            KlrValue *item;
            vector_foreach(item, v->list) {
                print_const_item(item, fp);
                if (i__ < len - 1) fprintf(fp, ", ");
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
    KlrValue *val = oper_value(oper);
    if (klr_is_const(val)) {
        print_const((KlrConst *)val, fp);
    } else {
        klr_print_value_name(val, fp);
    }

    if (!klr_is_const_none(val)) {
        print_value_type(val, fp);
    }
}

static void print_binary(KlrInsn *insn, char *op, FILE *fp)
{
    klr_print_value_name((KlrValue *)insn, fp);
    fprintf(fp, " = %s ", op);
    print_operand(&insn->opers[0], fp);
    fprintf(fp, ", ");
    print_operand(&insn->opers[1], fp);
}

static void print_ret(char *name, KlrInsn *insn, FILE *fp)
{
    fprintf(fp, "%s ", name);
    print_operand(&insn->opers[0], fp);
}

static void print_ret_void(KlrInsn *insn, FILE *fp) { fprintf(fp, "ret void"); }

static void print_ir_cast(char *name, KlrInsn *insn, FILE *fp)
{
    klr_print_value_name((KlrValue *)insn, fp);
    fprintf(fp, " = %s ", name);
    print_operand(&insn->opers[0], fp);
    fprintf(fp, " to");
    print_type(insn->ts, fp);
}

static void print_phi(KlrInsn *insn, FILE *fp)
{
    klr_print_value_name((KlrValue *)insn, fp);
    fprintf(fp, " = phi ");
    for (int i = 0; i < insn->num_opers; i++) {
        print_operand(&insn->opers[i], fp);
        if (i < insn->num_opers - 1) fprintf(fp, ", ");
    }
}

static void print_unary(KlrInsn *insn, char *op, FILE *fp)
{
    klr_print_value_name((KlrValue *)insn, fp);
    fprintf(fp, " = %s ", op);
    print_operand(&insn->opers[0], fp);
}

static void print_no_value_insn(const char *name, KlrInsn *insn, FILE *fp)
{
    fprintf(fp, "%s ", name);
    print_operand(&insn->opers[0], fp);
    fprintf(fp, ", ");
    print_operand(&insn->opers[1], fp);
}

static inline void print_cmp(char *name, KlrInsn *insn, FILE *fp) { print_binary(insn, name, fp); }

static void print_jmp(KlrInsn *insn, FILE *fp)
{
    fprintf(fp, "jmp ");
    KlrValue *val = insn_oper_value(insn, 0);
    fprintf(fp, "label %%bb%d", val->tag);
}

static void print_select(KlrInsn *insn, FILE *fp)
{
    klr_print_value_name((KlrValue *)insn, fp);
    fprintf(fp, " = select ");

    print_operand(&insn->opers[0], fp);
    fprintf(fp, ", ");
    print_operand(&insn->opers[1], fp);
    fprintf(fp, ", ");
    print_operand(&insn->opers[2], fp);
}

static void print_jmp_cond(const char *name, KlrInsn *insn, FILE *fp)
{
    fprintf(fp, "%s ", name);
    print_operand(&insn->opers[0], fp);
    fprintf(fp, ", ");

    KlrValue *_then = insn_oper_value(insn, 2);
    fprintf(fp, "label %%bb%d", _then->tag);

    KlrValue *_else = insn_oper_value(insn, 3);
    fprintf(fp, ", label %%bb%d", _else->tag);
}

static void print_jmp_cond_fused(const char *name, KlrInsn *insn, FILE *fp)
{
    fprintf(fp, "%s ", name);
    print_operand(&insn->opers[0], fp);
    fprintf(fp, ", ");

    print_operand(&insn->opers[1], fp);
    fprintf(fp, ", ");

    KlrValue *_then = insn_oper_value(insn, 2);
    fprintf(fp, "label %%bb%d", _then->tag);

    KlrValue *_else = insn_oper_value(insn, 3);
    fprintf(fp, ", label %%bb%d", _else->tag);
}

static void print_ir_call(KlrInsn *insn, FILE *fp)
{
    KlrValue *fn = insn_oper_value(insn, 0);

    if (fn->ts->kind == TYPE_NO_TYPE) {
        fprintf(fp, "call ");
    } else {
        klr_print_value_name((KlrValue *)insn, fp);
        fprintf(fp, " = call ");
    }

    fprintf(fp, "@%s", fn->name);

    if (fn->kind == KLR_VALUE_EXT_FUNC) {
        fprintf(fp, " [path = '%s']", ((KlrExtSym *)fn)->path);
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

static void print_call(const char *name, KlrInsn *insn, FILE *fp)
{
    KlrValue *fn = insn_oper_value(insn, 0);

    if (fn->ts->kind == TYPE_NO_TYPE) {
        fprintf(fp, "%s ", name);
    } else {
        klr_print_value_name((KlrValue *)insn, fp);
        fprintf(fp, " = %s ", name);
    }

    fprintf(fp, "@%s", fn->name);

    if (fn->kind == KLR_VALUE_EXT_FUNC) {
        fprintf(fp, " [path = '%s']", ((KlrExtSym *)fn)->path);
    }

    if (insn->flags & KLR_INSN_FLAGS_CONST) {
        fprintf(fp, " [const]");
    }

    fprintf(fp, ", nargs=%d", insn->num_args);
}

static void print_get_global(KlrInsn *insn, FILE *fp)
{
    klr_print_value_name((KlrValue *)insn, fp);
    fprintf(fp, " = get_global ");
    print_operand(&insn->opers[0], fp);
}

static void print_local_insn(KlrValue *local, FILE *fp)
{
    klr_print_value_name(local, fp);
    fprintf(fp, " = local");
    KlrInsn *insn = (KlrInsn *)local;
    if (insn->flags & KLR_INSN_FLAGS_CONST) fprintf(fp, " [immutable]");
    print_value_type(local, fp);
}

static void print_attributes(KlrInsn *insn, FILE *fp)
{
    if (insn->fixedslot) {
        if (insn->vreg != -1) {
            fprintf(fp, "        [fixedslot = %d(%s) -> R%d]", insn->slotindex,
                    insn->fixedslot == 1 ? "next" : "pos", insn->vreg);
        } else {
            fprintf(fp, "        [fixedslot = %d(%s)]", insn->slotindex,
                    insn->fixedslot == 1 ? "next" : "pos");
        }
    }

    if (insn->error) {
        fprintf(fp, "        [error]");
    }
}

static void print_new(KlrInsn *insn, FILE *fp)
{
    KlrValue *ty = insn_oper_value(insn, 0);

    TypeSpec *ts = ty->ts;

    klr_print_value_name((KlrValue *)insn, fp);
    if (ts->klass_type.pkg) {
        fprintf(fp, " = new @%s::%s", ts->klass_type.pkg, ts->klass_type.name);
    } else {
        fprintf(fp, " = new @%s", ts->klass_type.name);
    }

    fprintf(fp, ", nargs=%d", insn->num_args);
}

static void print_ir_new(KlrInsn *insn, FILE *fp)
{
    KlrValue *ty = insn_oper_value(insn, 0);

    TypeSpec *ts = ty->ts;

    klr_print_value_name((KlrValue *)insn, fp);
    if (ts->klass_type.pkg) {
        fprintf(fp, " = new @%s::%s", ts->klass_type.pkg, ts->klass_type.name);
    } else {
        fprintf(fp, " = new @%s", ts->klass_type.name);
    }

    if (insn->num_opers > 1) fprintf(fp, ", ");

    KlrOper *oper;
    for (int i = 1; i < insn->num_opers; i++) {
        if (i != 1) fprintf(fp, ", ");
        oper = &insn->opers[i];
        print_operand(oper, fp);
    }
}

static void print_build_intern(KlrInsn *insn, FILE *fp)
{
    klr_print_value_name((KlrValue *)insn, fp);

    if (insn->num_args > 0) {
        fprintf(fp, " = build_intern @%s, nargs=%d", intern_tag_name[insn->intern_tag],
                insn->num_args);
    } else {
        fprintf(fp, " = build_intern @%s", intern_tag_name[insn->intern_tag]);
        if (insn->num_opers > 0) fprintf(fp, ", ");

        KlrOper *oper;
        for (int i = 0; i < insn->num_opers; i++) {
            if (i != 0) fprintf(fp, ", ");
            oper = &insn->opers[i];
            print_operand(oper, fp);
        }
    }
}

static void print_get_field(KlrInsn *insn, char *name, FILE *fp)
{
    klr_print_value_name((KlrValue *)insn, fp);
    fprintf(fp, " = %s ", name);
    print_operand(&insn->opers[0], fp);
    KlrFieldInfo *field_info = &insn->field_info;
    if (field_info->index == -1) {
        fprintf(fp, ", @%s::%s [path = '%s']", field_info->klass, field_info->name,
                field_info->path);
    } else {
        fprintf(fp, ", %s(index=%d)", field_info->name, field_info->index);
    }
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

        case OP_IR_SELECT:
            print_select(insn, fp);
            break;

        case OP_IR_JMP_COND:
            print_jmp_cond("branch", insn, fp);
            break;

        case OP_MOVE:
            print_no_value_insn("move", insn, fp);
            break;

        case OP_JMP_INT_LT:
            print_jmp_cond_fused("jmp_int_lt", insn, fp);
            break;

        case OP_JMP_INT_LT_IMM:
            print_jmp_cond_fused("jmp_int_lt_imm", insn, fp);
            break;

        case OP_JMP_INT_LE:
            print_jmp_cond_fused("jmp_int_le", insn, fp);
            break;

        case OP_JMP_INT_LE_IMM:
            print_jmp_cond_fused("jmp_int_le_imm", insn, fp);
            break;

        case OP_JMP_INT_EQ:
            print_jmp_cond_fused("jmp_int_eq", insn, fp);
            break;

        case OP_JMP_INT_EQ_IMM:
            print_jmp_cond_fused("jmp_int_eq_imm", insn, fp);
            break;

        case OP_JMP_INT_NE:
            print_jmp_cond_fused("jmp_int_ne", insn, fp);
            break;

        case OP_JMP_INT_NE_IMM:
            print_jmp_cond_fused("jmp_int_ne_imm", insn, fp);
            break;

        case OP_JMP_INT_GT:
            print_jmp_cond_fused("jmp_int_gt", insn, fp);
            break;

        case OP_JMP_INT_GT_IMM:
            print_jmp_cond_fused("jmp_int_gt_imm", insn, fp);
            break;

        case OP_JMP_INT_GE:
            print_jmp_cond_fused("jmp_int_ge", insn, fp);
            break;

        case OP_JMP_INT_GE_IMM:
            print_jmp_cond_fused("jmp_int_ge_imm", insn, fp);
            break;

        case OP_JMP_UINT_LT:
            print_jmp_cond_fused("jmp_uint_lt", insn, fp);
            break;

        case OP_JMP_UINT_LT_IMM:
            print_jmp_cond_fused("jmp_uint_lt_imm", insn, fp);
            break;

        case OP_JMP_UINT_LE:
            print_jmp_cond_fused("jmp_uint_le", insn, fp);
            break;

        case OP_JMP_UINT_LE_IMM:
            print_jmp_cond_fused("jmp_uint_le_imm", insn, fp);
            break;

        case OP_JMP_UINT_GT:
            print_jmp_cond_fused("jmp_uint_gt", insn, fp);
            break;

        case OP_JMP_UINT_GT_IMM:
            print_jmp_cond_fused("jmp_uint_gt_imm", insn, fp);
            break;

        case OP_JMP_UINT_GE:
            print_jmp_cond_fused("jmp_uint_ge", insn, fp);
            break;

        case OP_JMP_UINT_GE_IMM:
            print_jmp_cond_fused("jmp_uint_ge_imm", insn, fp);
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

        case OP_IR_CALL:
            print_ir_call(insn, fp);
            break;

        case OP_CALL:
            print_call("call", insn, fp);
            break;

        case OP_TAIL_CALL:
            print_call("tail_call", insn, fp);
            break;

        case OP_BINARY_CMPEQ:
            print_cmp("cmpeq", insn, fp);
            break;

        case OP_BINARY_CMPNE:
            print_cmp("cmpne", insn, fp);
            break;

        case OP_BINARY_CMPLT:
            print_cmp("cmplt", insn, fp);
            break;

        case OP_BINARY_CMPGT:
            print_cmp("cmpgt", insn, fp);
            break;

        case OP_BINARY_CMPLE:
            print_cmp("cmple", insn, fp);
            break;

        case OP_BINARY_CMPGE:
            print_cmp("cmpge", insn, fp);
            break;

        case OP_JMP:
            print_jmp(insn, fp);
            break;

        case OP_JMP_TRUE:
            print_jmp_cond("jmp_true", insn, fp);
            break;

        case OP_RET:
            print_ret("ret", insn, fp);
            break;

        case OP_RET_INT_IMM:
            print_ret("ret_int_imm", insn, fp);
            break;

        case OP_RET_TAG:
            print_ret("ret_tag", insn, fp);
            break;

        case OP_RET_CONST:
            print_ret("ret_const", insn, fp);
            break;

        case OP_RET_VOID:
            print_ret_void(insn, fp);
            break;

        case OP_GLOBAL_GET:
            print_get_global(insn, fp);
            break;

        case OP_GLOBAL_SET:
            print_no_value_insn("set_global", insn, fp);
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

        case OP_LOAD_INT_IMM:
            print_no_value_insn("load_int_imm", insn, fp);
            break;

        case OP_INT_ADD:
            print_binary(insn, "int.add", fp);
            break;

        case OP_INT_ADD_IMM:
            print_binary(insn, "int.add_imm", fp);
            break;

        case OP_INT_SUB:
            print_binary(insn, "int.sub", fp);
            break;

        case OP_INT_SUB_IMM:
            print_binary(insn, "int.sub_imm", fp);
            break;

        case OP_INT_MUL:
            print_binary(insn, "int.mul", fp);
            break;

        case OP_INT_MUL_IMM:
            print_binary(insn, "int.mul_imm", fp);
            break;

        case OP_INT_DIV:
            print_binary(insn, "int.div", fp);
            break;

        case OP_INT_DIV_IMM:
            print_binary(insn, "int.div_imm", fp);
            break;

        case OP_INT_MOD:
            print_binary(insn, "int.mod", fp);
            break;

        case OP_INT_MOD_IMM:
            print_binary(insn, "int.mod_imm", fp);
            break;

        case OP_INT_SHL:
            print_binary(insn, "int.shl", fp);
            break;

        case OP_INT_SHL_IMM:
            print_binary(insn, "int.shl_imm", fp);
            break;

        case OP_INT_SHR:
            print_binary(insn, "int.shr", fp);
            break;

        case OP_INT_SHR_IMM:
            print_binary(insn, "int.shr_imm", fp);
            break;

        case OP_INT_AND:
            print_binary(insn, "int.and", fp);
            break;

        case OP_INT_AND_IMM:
            print_binary(insn, "int.and_imm", fp);
            break;

        case OP_INT_OR:
            print_binary(insn, "int.or", fp);
            break;

        case OP_INT_OR_IMM:
            print_binary(insn, "int.or_imm", fp);
            break;

        case OP_INT_XOR:
            print_binary(insn, "int.xor", fp);
            break;

        case OP_INT_XOR_IMM:
            print_binary(insn, "int.xor_imm", fp);
            break;

        case OP_LOADK:
            print_no_value_insn("loadk", insn, fp);
            break;

        case OP_LOAD_TAG:
            print_no_value_insn("load_tag", insn, fp);
            break;

        case OP_INT_CMPEQ:
            print_cmp("int.cmp_eq", insn, fp);
            break;

        case OP_INT_CMPEQ_IMM:
            print_cmp("int.cmp_eq_imm", insn, fp);
            break;

        case OP_INT_CMPNE:
            print_cmp("int.cmp_ne", insn, fp);
            break;

        case OP_INT_CMPNE_IMM:
            print_cmp("int.cmp_ne_imm", insn, fp);
            break;

        case OP_INT_CMPLT:
            print_cmp("int.cmp_lt", insn, fp);
            break;

        case OP_INT_CMPLT_IMM:
            print_cmp("int.cmp_lt_imm", insn, fp);
            break;

        case OP_INT_CMPLE:
            print_cmp("int.cmp_le", insn, fp);
            break;

        case OP_INT_CMPLE_IMM:
            print_cmp("int.cmp_le_imm", insn, fp);
            break;

        case OP_INT_CMPGT:
            print_cmp("int.cmp_gt", insn, fp);
            break;

        case OP_INT_CMPGT_IMM:
            print_cmp("int.cmp_gt_imm", insn, fp);
            break;

        case OP_INT_CMPGE:
            print_cmp("int.cmp_ge", insn, fp);
            break;

        case OP_INT_CMPGE_IMM:
            print_cmp("int.cmp_ge_imm", insn, fp);
            break;

        case OP_UINT_CMPEQ_IMM:
            print_cmp("uint.cmp_eq_imm", insn, fp);
            break;

        case OP_FLOAT_ADD:
            print_binary(insn, "float.add", fp);
            break;

        case OP_FLOAT_SUB:
            print_binary(insn, "float.sub", fp);
            break;

        case OP_FLOAT_MUL:
            print_binary(insn, "float.mul", fp);
            break;

        case OP_FLOAT_DIV:
            print_binary(insn, "float.div", fp);
            break;

        case OP_FLOAT_MOD:
            print_binary(insn, "float.mod", fp);
            break;

        case OP_FLOAT_CMPEQ:
            print_cmp("float.cmp_eq", insn, fp);
            break;

        case OP_FLOAT_CMPNE:
            print_cmp("float.cmp_ne", insn, fp);
            break;

        case OP_FLOAT_CMPLT:
            print_cmp("float.cmp_lt", insn, fp);
            break;

        case OP_FLOAT_CMPLE:
            print_cmp("float.cmp_le", insn, fp);
            break;

        case OP_FLOAT_CMPGT:
            print_cmp("float.cmp_gt", insn, fp);
            break;

        case OP_FLOAT_CMPGE:
            print_cmp("float.cmp_ge", insn, fp);
            break;

        case OP_JMP_FLOAT_EQ:
            print_jmp_cond_fused("jmp_float_eq", insn, fp);
            break;

        case OP_JMP_FLOAT_NE:
            print_jmp_cond_fused("jmp_float_ne", insn, fp);
            break;

        case OP_JMP_FLOAT_LT:
            print_jmp_cond_fused("jmp_float_lt", insn, fp);
            break;

        case OP_JMP_FLOAT_LE:
            print_jmp_cond_fused("jmp_float_le", insn, fp);
            break;

        case OP_JMP_FLOAT_GT:
            print_jmp_cond_fused("jmp_float_gt", insn, fp);
            break;

        case OP_JMP_FLOAT_GE:
            print_jmp_cond_fused("jmp_float_ge", insn, fp);
            break;

        case OP_UINT_ADD_IMM:
            print_binary(insn, "uint.add_imm", fp);
            break;

        case OP_UINT_SUB_IMM:
            print_binary(insn, "uint.sub_imm", fp);
            break;

        case OP_UINT_MUL_IMM:
            print_binary(insn, "uint.mul_imm", fp);
            break;

        case OP_UINT_DIV:
            print_binary(insn, "uint.div", fp);
            break;

        case OP_UINT_DIV_IMM:
            print_binary(insn, "uint.div_imm", fp);
            break;

        case OP_UINT_MOD:
            print_binary(insn, "uint.mod", fp);
            break;

        case OP_UINT_MOD_IMM:
            print_binary(insn, "uint.mod_imm", fp);
            break;

        case OP_UINT_SHR:
            print_binary(insn, "uint.shr", fp);
            break;

        case OP_UINT_SHL_IMM:
            print_binary(insn, "uint.shl_imm", fp);
            break;

        case OP_UINT_SHR_IMM:
            print_binary(insn, "uint.shr_imm", fp);
            break;

        case OP_UINT_AND_IMM:
            print_binary(insn, "uint.and_imm", fp);
            break;

        case OP_UINT_OR_IMM:
            print_binary(insn, "uint.or_imm", fp);
            break;

        case OP_UINT_XOR_IMM:
            print_binary(insn, "uint.xor_imm", fp);
            break;

        case OP_LOAD_UINT_IMM:
            print_no_value_insn("load_uint_imm", insn, fp);
            break;

        case OP_RET_UINT_IMM:
            print_ret("ret_uint_imm", insn, fp);
            break;

        case OP_IR_CAST:
            print_ir_cast("cast", insn, fp);
            break;

        case OP_INT_CAST:
            print_ir_cast("int_cast", insn, fp);
            break;

        case OP_FLOAT_CAST:
            print_ir_cast("float_cast", insn, fp);
            break;

        case OP_REF_EQ:
            print_binary(insn, "ref.eq", fp);
            break;

        case OP_REF_NE:
            print_binary(insn, "ref.ne", fp);
            break;

        case OP_REF_EQ_NULL:
            print_unary(insn, "ref.eq_null", fp);
            break;

        case OP_REF_NE_NULL:
            print_unary(insn, "ref.ne_null", fp);
            break;

        case OP_JMP_REF_EQ:
            print_jmp_cond_fused("jmp_ref_eq", insn, fp);
            break;

        case OP_JMP_REF_NE:
            print_jmp_cond_fused("jmp_ref_ne", insn, fp);
            break;

        case OP_JMP_REF_EQ_NULL:
            print_jmp_cond_fused("jmp_ref_eq_null", insn, fp);
            break;

        case OP_JMP_REF_NE_NULL:
            print_jmp_cond_fused("jmp_ref_ne_null", insn, fp);
            break;

        case OP_NEW:
            print_new(insn, fp);
            break;

        case OP_IR_NEW:
            print_ir_new(insn, fp);
            break;

        case OP_BUILD_INTERN:
            print_build_intern(insn, fp);
            break;

        case OP_GET_FIELD:
            print_get_field(insn, "get_field", fp);
            break;

        case OP_GET_FIELD_EXT:
            print_get_field(insn, "get_field_ext", fp);
            break;

        default:
            printf("%s\n", op_name(insn->code));
            break;
    }

    print_attributes(insn, fp);
}

static void print_preds(KlrBasicBlock *bb, int spaces, FILE *fp)
{
    fprintf(fp, "%*s = ", spaces, ";; preds");

    KlrBasicBlock *pred;
    int i = 0;
    bb_pred_foreach(pred, bb) {
        if (i++ == 0) {
            fprintf(fp, "%%bb%d", pred->tag);
        } else {
            fprintf(fp, ", %%bb%d", pred->tag);
        }
    }
}

static void print_block(KlrBasicBlock *bb, FILE *fp)
{
    int used = 0;

    if (bb->name[0]) {
        used = fprintf(fp, "  %%bb%d(%s):", bb->tag, bb->name);
    } else {
        used = fprintf(fp, "  %%bb%d:", bb->tag);
    }

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

void update_tags(KlrFunc *fn)
{
    fn->tag = 0;
    fn->bb_tag = 0;

    KlrParam *param;
    vector_foreach(param, &fn->params) {
        if (!param->name[0]) param->tag = fn->tag++;
    }

    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        bb->tag = fn->bb_tag++;
        KlrInsn *insn;
        insn_foreach(insn, bb) {
            if (!ir_has_value(insn)) continue;
            if (!insn->name[0]) insn->tag = fn->tag++;
        }
    }
}

void klr_print_func(KlrFunc *func, FILE *fp)
{
    update_tags(func);

    fprintf(fp, "  func @%s", func->name);

    fprintf(fp, "(");
    KlrParam *param;
    vector_foreach(param, &func->params) {
        if (i__ != 0) {
            fprintf(fp, ", ");
        }

        klr_print_value_name((KlrValue *)param, fp);
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
    func_foreach(fn, m) {
        klr_print_func(fn, fp);
    }

    fprintf(fp, "}\n");
}

#ifdef __cplusplus
}
#endif

/*----------------------------------------------------------------------------*\
|* This file is part of the koala project, under the MIT License.             *|
|* Copyright (c) 2021-2021 James <zhuguangxiang@gmail.com>                    *|
\*----------------------------------------------------------------------------*/

#include "klvm.h"

#ifdef __cplusplus
extern "C" {
#endif

// clang-format off

#define NEW_LINE() fprintf(fp, "\n")
#define INDENT() fprintf(fp, "  ")
#define ASSIGN() fprintf(fp, " = ")
#define COMMA()  fprintf(fp, ", ")

// clang-format on

static void klvm_print_var(klvm_var_t *var, FILE *fp)
{
    if (var->attr & KLVM_ATTR_LOCAL) {
        printf("error: invalid variable attributes\n");
        abort();
    }

    if (var->attr & KLVM_ATTR_PUB) fprintf(fp, "pub ");

    if (var->attr & KLVM_ATTR_FINAL)
        fprintf(fp, "const ");
    else
        fprintf(fp, "var ");

    fprintf(fp, "%s %s", var->name, klvm_type_string(var->type));
}

static void klvm_print_local(klvm_var_t *var, FILE *fp)
{
    if (var->attr != KLVM_ATTR_LOCAL) {
        printf("error: invalid local attributes\n");
        abort();
    }

    fprintf(fp, "local %s %s", var->name, klvm_type_string(var->type));
}

static inline int val_tag(klvm_func_t *fn, klvm_value_t *val)
{
    if (val->tag < 0) { val->tag = fn->tag_index++; }
    return val->tag;
}

static inline int bb_tag(klvm_block_t *bb)
{
    if (bb->tag < 0) {
        klvm_func_t *fn = (klvm_func_t *)bb->fn;
        bb->tag = fn->bb_tag_index++;
    }
    return bb->tag;
}

static void klvm_print_const(klvm_const_t *v, FILE *fp)
{
    klvm_type_kind_t kind = v->type->kind;
    switch (kind) {
        case KLVM_TYPE_INT8:
            fprintf(fp, "%d", v->i8val);
            break;
        case KLVM_TYPE_INT16:
            fprintf(fp, "%d", v->i16val);
            break;
        case KLVM_TYPE_INT32:
            fprintf(fp, "%d", v->i32val);
            break;
        case KLVM_TYPE_INT64:
            fprintf(fp, "%ld", v->i64val);
            break;
        default:
            break;
    }
}

static void klvm_print_type(klvm_type_t *ty, FILE *fp)
{
    fprintf(fp, " %s", klvm_type_string(ty));
}

static void klvm_print_name(klvm_value_t *val, klvm_func_t *fn, FILE *fp)
{
    if (val->name && val->name[0])
        fprintf(fp, "%%%s", val->name);
    else {
        fprintf(fp, "%%%d", val_tag(fn, val));
    }
}

static void klvm_print_value(klvm_func_t *fn, klvm_value_t *val, FILE *fp)
{
    klvm_value_kind_t kind = val->kind;
    switch (kind) {
        case KLVM_VALUE_CONST: {
            klvm_print_const((klvm_const_t *)val, fp);
            klvm_print_type(val->type, fp);
            break;
        }
        case KLVM_VALUE_VAR: {
            klvm_var_t *var = (klvm_var_t *)val;
            if (var->attr & KLVM_ATTR_LOCAL)
                klvm_print_name(val, fn, fp);
            else
                fprintf(fp, "@%s", var->name);
            klvm_print_type(val->type, fp);
            break;
        }
        case KLVM_VALUE_INST: {
            klvm_print_name(val, fn, fp);
            klvm_print_type(val->type, fp);
            break;
        }
        case KLVM_VALUE_FUNC: {
            klvm_func_t *_fn = (klvm_func_t *)val;
            fprintf(fp, "@%s", _fn->name);
            break;
        }
        default:
            printf("error: unsupported value\n");
            abort();
            break;
    }
}

static void klvm_print_copy(klvm_copy_t *inst, klvm_func_t *fn, FILE *fp)
{
    klvm_print_value(fn, inst->lhs, fp);
    ASSIGN();
    klvm_print_value(fn, inst->rhs, fp);
}

static void klvm_print_binary(
    klvm_binary_t *inst, char *opname, klvm_func_t *fn, FILE *fp)
{
    klvm_print_name((klvm_value_t *)inst, fn, fp);

    klvm_print_type(inst->type, fp);

    ASSIGN();

    fprintf(fp, "%s ", opname);
    klvm_print_value(fn, inst->lhs, fp);
    COMMA();
    klvm_print_value(fn, inst->rhs, fp);
}

static void klvm_print_call(klvm_call_t *inst, klvm_func_t *fn, FILE *fp)
{
    klvm_print_name((klvm_value_t *)inst, fn, fp);

    klvm_print_type(inst->type, fp);

    ASSIGN();

    fprintf(fp, "call ");

    klvm_print_value(fn, inst->fn, fp);

    fprintf(fp, "(");

    klvm_value_t **arg;
    vector_foreach(arg, &inst->args)
    {
        if (i != 0) COMMA();
        klvm_print_value(fn, *arg, fp);
    }

    fprintf(fp, ")");
}

static void klvm_print_ret(klvm_ret_t *inst, klvm_func_t *fn, FILE *fp)
{
    fprintf(fp, "ret ");
    klvm_print_value(fn, inst->ret, fp);
}

static void klvm_print_jmp(klvm_jmp_t *inst, FILE *fp)
{
    fprintf(fp, "jmp ");

    klvm_block_t *bb = inst->dst;
    if (bb->label && bb->label[0])
        fprintf(fp, "label %%%s", bb->label);
    else
        fprintf(fp, "label %%bb%d", bb_tag(bb));
}

static void klvm_print_branch(klvm_branch_t *inst, klvm_func_t *fn, FILE *fp)
{
    fprintf(fp, "br ");
    klvm_print_value(fn, inst->cond, fp);

    klvm_block_t *bb;

    bb = inst->_then;
    if (bb->label && bb->label[0])
        fprintf(fp, ", label %%%s", bb->label);
    else
        fprintf(fp, ", label %%bb%d", bb_tag(bb));

    bb = inst->_else;
    if (bb->label && bb->label[0])
        fprintf(fp, ", label %%%s", bb->label);
    else
        fprintf(fp, ", label %%bb%d", bb_tag(bb));
}

static void klvm_print_inst(klvm_func_t *fn, klvm_inst_t *inst, FILE *fp)
{
    switch (inst->op) {
        case KLVM_INST_COPY:
            klvm_print_copy((klvm_copy_t *)inst, fn, fp);
            break;
        case KLVM_INST_ADD:
            klvm_print_binary((klvm_binary_t *)inst, "add", fn, fp);
            break;
        case KLVM_INST_SUB:
            klvm_print_binary((klvm_binary_t *)inst, "sub", fn, fp);
            break;
        case KLVM_INST_CMP_LE:
            klvm_print_binary((klvm_binary_t *)inst, "cmple", fn, fp);
            break;
        case KLVM_INST_CALL:
            klvm_print_call((klvm_call_t *)inst, fn, fp);
            break;
        case KLVM_INST_RET:
            klvm_print_ret((klvm_ret_t *)inst, fn, fp);
            break;
        case KLVM_INST_JMP:
            klvm_print_jmp((klvm_jmp_t *)inst, fp);
            break;
        case KLVM_INST_BRANCH:
            klvm_print_branch((klvm_branch_t *)inst, fn, fp);
            break;
        default:
            break;
    }
}

static void klvm_print_preds(klvm_block_t *bb, int spaces, FILE *fp)
{
    fprintf(fp, "%*s = %%", spaces, ";; preds");

    klvm_block_t *src;
    list_t *l = &bb->in_edges;
    list_node_t *n;
    klvm_edge_t *edge;
    int i = 0;
    list_foreach(l, n)
    {
        edge = list_entry(n, klvm_edge_t, in_link);
        src = edge->src;
        if (i++ == 0)
            fprintf(fp, "%s", src->label);
        else
            fprintf(fp, ", %s", src->label);
    }
}

static void klvm_print_block(klvm_block_t *bb, FILE *fp)
{
    int cnt;
    if (bb->label && bb->label[0]) { cnt = fprintf(fp, "%s:", bb->label); }
    else {
        cnt = fprintf(fp, "bb%d:", bb_tag(bb));
    }

    // print predecessors
    if (list_is_empty(&bb->in_edges)) {
        fprintf(fp, "%*s", 64 - cnt, ";; No predecessors!");
    }
    else {
        klvm_func_t *fn = (klvm_func_t *)bb->fn;
        list_node_t *n = list_first(&bb->in_edges);
        klvm_edge_t *edge = list_entry(n, klvm_edge_t, in_link);
        if (edge->src != &fn->sbb) klvm_print_preds(bb, 64 - cnt, fp);
    }

    klvm_inst_t *inst;
    list_node_t *n;
    list_foreach(&bb->insts, n)
    {
        inst = list_entry(n, klvm_inst_t, node);
        NEW_LINE();
        INDENT();
        klvm_print_inst((klvm_func_t *)bb->fn, inst, fp);
    }
}

static void klvm_print_blocks(klvm_func_t *fn, FILE *fp)
{

    // print basic blocks directly(not cfg)

    klvm_block_t *bb;
    list_node_t *n;
    list_foreach(&fn->bblist, n)
    {
        bb = (klvm_block_t *)n;
        NEW_LINE();
        klvm_print_block(bb, fp);
    }
}

static void klvm_print_locals(klvm_func_t *fn, FILE *fp)
{
    // print local variables
    klvm_var_t *var;
    int num_locals = vector_size(&fn->locals);
    for (int i = fn->num_params; i < num_locals; i++) {
        vector_get(&fn->locals, i, &var);
        NEW_LINE();
        INDENT();
        klvm_print_local(var, fp);
    }
}

static void klvm_print_func(klvm_func_t *fn, FILE *fp)
{
    fprintf(fp, "func %s", fn->name);

    fprintf(fp, "(");
    klvm_value_t *para;
    for (int i = 0; i < fn->num_params; i++) {
        vector_get(&fn->locals, i, &para);
        if (i != 0) fprintf(fp, ", ");
        if (para->name && para->name[0])
            fprintf(fp, "%s", para->name);
        else
            fprintf(fp, "%%%d", val_tag(fn, para));
        fprintf(fp, " %s", klvm_type_string(para->type));
    }
    fprintf(fp, ")");

    klvm_type_t *ret = klvm_proto_return(fn->type);
    if (ret) fprintf(fp, " %s", klvm_type_string(ret));

    fprintf(fp, " {");

    klvm_print_locals(fn, fp);
    klvm_print_blocks(fn, fp);

    NEW_LINE();
    fprintf(fp, "}");
    NEW_LINE();
}

void klvm_print_module(klvm_module_t *m, FILE *fp)
{
    fprintf(fp, "\n__name__ := \"%s\"\n\n", m->name);

    klvm_var_t **var;
    vector_foreach(var, &m->vars)
    {
        klvm_print_var(*var, fp);
        NEW_LINE();
    }

    NEW_LINE();

    klvm_func_t *_init_ = (klvm_func_t *)m->_init_;
    if (_init_) {
        klvm_print_func(_init_, fp);
        NEW_LINE();
    }

    klvm_func_t **fn;
    vector_foreach(fn, &m->funcs)
    {
        klvm_print_func(*fn, fp);
        NEW_LINE();
    }
}

#ifdef __cplusplus
}
#endif

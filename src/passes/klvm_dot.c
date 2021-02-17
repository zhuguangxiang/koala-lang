/*----------------------------------------------------------------------------*\
|* This file is part of the koala project, under the MIT License.             *|
|* Copyright (c) 2021-2021 James <zhuguangxiang@gmail.com>                    *|
\*----------------------------------------------------------------------------*/

#include "klvm.h"

#ifdef __cplusplus
extern "C" {
#endif

static void show_block(klvm_block_t *bb)
{
    int oedges = list_length(&bb->out_edges);
    printf("bb: %s, out edges: %d\n", bb->label, oedges);

    int i = 0;
    klvm_edge_t *edge;
    list_node_t *n;
    list_foreach(&bb->out_edges, n)
    {
        edge = list_entry(n, klvm_edge_t, out_link);
        printf("out-bb-%d: %s\n", i++, edge->dst->label);
    }

    list_foreach(&bb->out_edges, n)
    {
        edge = list_entry(n, klvm_edge_t, out_link);
        show_block(edge->dst);
    }
}

static void dot_text_pass(klvm_func_t *fn, void *data)
{
    printf("func: %s\n", fn->name);
    klvm_block_t *sbb = &fn->sbb;
    show_block(sbb);
}

static int has_branch(klvm_block_t *bb)
{
    klvm_inst_t *inst;
    list_node_t *n;
    n = list_last(&bb->insts);
    if (!n) return 0;
    inst = list_entry(n, klvm_inst_t, node);
    return (inst->op == KLVM_INST_BRANCH) ? 1 : 0;
}

static int is_ebb(klvm_block_t *bb)
{
    klvm_func_t *fn = (klvm_func_t *)bb->fn;
    return bb == &fn->ebb;
}

static void dot_print_edge(klvm_block_t *bb, FILE *fp)
{
    int branch = has_branch(bb);
    klvm_block_t *out;
    klvm_edge_t *edge;
    list_node_t *n;
    int i = 0;
    list_foreach(&bb->out_edges, n)
    {
        edge = list_entry(n, klvm_edge_t, out_link);
        out = edge->dst;
        if (branch) {
            if (is_ebb(out)) {
                fprintf(fp, "  %s:%c -> %s:h\n", bb->label,
                    (i == 0) ? 't' : 'f', out->label);
            }
            else {
                fprintf(fp, "  %s:%c -> %s\n", bb->label, (i == 0) ? 't' : 'f',
                    out->label);
            }
        }
        else {
            if (is_ebb(out)) {
                fprintf(fp, "  %s -> %s\n", bb->label, out->label);
            }
            else {
                fprintf(fp, "  %s -> %s:h\n", bb->label, out->label);
            }
        }
        dot_print_edge(out, fp);
        i++;
    }
}

static void dot_print_block(klvm_block_t *bb, klvm_func_t *fn, FILE *fp)
{
    klvm_inst_t *inst;
    list_node_t *n;
    list_foreach(&bb->insts, n)
    {
        inst = list_entry(n, klvm_inst_t, node);
        fprintf(fp, "\\l");
        klvm_print_inst((klvm_func_t *)bb->fn, inst, fp);
    }
}

static void dot_graph_pass(klvm_func_t *fn, void *data)
{
    char buf[512];
    snprintf(buf, 511, "./%s.dot", fn->name);

    FILE *fp = fopen(buf, "w");
    fprintf(fp, "digraph %s {\n", fn->name);

    fprintf(fp, "  node[shape=record margin=0.1 fontsize=12]\n");

    klvm_block_t *bb;
    klvm_inst_t *inst;
    list_node_t *n;
    list_node_t *n2;
    list_foreach(&fn->bblist, n)
    {
        bb = list_entry(n, klvm_block_t, bbnode);
        fprintf(fp, "  %s", bb->label);
        n2 = list_last(&bb->insts);
        inst = list_entry(n2, klvm_inst_t, node);
        if (inst->op == KLVM_INST_BRANCH) {
            fprintf(fp, "[label=\"{<h>%s:", bb->label);
            dot_print_block(bb, fn, fp);
            fprintf(fp, "|{<t>T|<f>F}}\"]\n");
        }
        else {
            fprintf(fp, "[label=\"{<h>%s:", bb->label);
            dot_print_block(bb, fn, fp);
            fprintf(fp, "\\l}\"]\n");
        }
    }

    dot_print_edge(&fn->sbb, fp);

    fprintf(fp, "}\n");
    fclose(fp);

    snprintf(buf, 511, "dot -Tpdf %s.dot -o %s.pdf", fn->name, fn->name);
    system(buf);
}

void dot_pass_register(klvm_module_t *m)
{
    klvm_pass_t pass = { dot_text_pass, NULL };
    klvm_register_pass(m, &pass);
    pass = (klvm_pass_t) { dot_graph_pass, NULL };
    klvm_register_pass(m, &pass);
}

#ifdef __cplusplus
}
#endif

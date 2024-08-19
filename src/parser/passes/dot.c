/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "ir.h"

#ifdef __cplusplus
extern "C" {
#endif

static void show_block(KlrBasicBlock *bb)
{
    printf("bb: %s, out edges: %d\n", bb->name, bb->num_outedges);

    int i = 0;
    KlrEdge *edge;
    edge_out_foreach(edge, bb) {
        printf("out-bb-%d: %s\n", i++, edge->dst->name);
    }

    edge_out_foreach(edge, bb) {
        show_block(edge->dst);
    }
}

static void text_block_show_pass(KlrFunc *fn, void *data)
{
    printf("func: %s\n", fn->name);
    KlrBasicBlock *sbb = fn->sbb;
    show_block(sbb);
}

static int is_ebb(KlrBasicBlock *bb)
{
    KlrFunc *fn = (KlrFunc *)bb->func;
    return bb == fn->ebb;
}

static void dot_print_edge(KlrBasicBlock *bb, FILE *fp)
{
    int branch = bb->has_branch;
    KlrBasicBlock *out;
    KlrEdge *edge;
    int i = 0;
    edge_out_foreach(edge, bb) {
        out = edge->dst;
        if (branch) {
            if (is_ebb(out)) {
                fprintf(fp, "  %s:%c -> %s:h\n", bb->name, (i == 0) ? 't' : 'f',
                        out->name);
            } else {
                fprintf(fp, "  %s:%c -> %s\n", bb->name, (i == 0) ? 't' : 'f', out->name);
            }
        } else {
            if (is_ebb(out)) {
                fprintf(fp, "  %s -> %s\n", bb->name, out->name);
            } else {
                fprintf(fp, "  %s -> %s:h\n", bb->name, out->name);
            }
        }
        dot_print_edge(out, fp);
        i++;
    }
}

static void dot_print_block(KlrBasicBlock *bb, FILE *fp)
{
    KlrInsn *insn;
    insn_foreach(insn, bb) {
        fprintf(fp, "\\l\\ \\ \\ ");
        klr_print_insn(insn, fp);
    }
}

static void dot_graph_pass(KlrFunc *fn, void *data)
{
    char buf[512];
    snprintf(buf, 511, "./%s.dot", fn->name);

    FILE *fp = fopen(buf, "w");
    fprintf(fp, "digraph %s {\n", fn->name);

    fprintf(fp,
            "  node[shape=record margin=0.1 fontsize=10, fontname=\"Ubuntu Mono\"]\n");

    KlrBasicBlock *bb;
    KlrInsn *insn;
    basic_block_foreach(bb, fn) {
        fprintf(fp, "  %s", bb->name);
        insn = insn_last(bb);
        if (bb->has_branch) {
            fprintf(fp, "[label=\"{<h>%s:", bb->name);
            dot_print_block(bb, fp);
            fprintf(fp, "|{<t>T|<f>F}}\"]\n");
        } else {
            fprintf(fp, "[label=\"{<h>%s:", bb->name);
            dot_print_block(bb, fp);
            fprintf(fp, "\\l}\"]\n");
        }
    }

    dot_print_edge(fn->sbb, fp);

    fprintf(fp, "}\n");
    fclose(fp);

    snprintf(buf, 511, "dot -Tpdf %s.dot -o %s.pdf", fn->name, fn->name);
    system(buf);
}

void register_dot_passes(KlrPassGroup *grp)
{
    klr_add_pass(grp, "text_printer", text_block_show_pass, NULL);
    klr_add_pass(grp, "dot_graph", dot_graph_pass, NULL);
}

#ifdef __cplusplus
}
#endif

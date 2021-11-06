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

typedef struct _KLVMPass KLVMPass;

struct _KLVMPass {
    List link;
    KLVMPassFunc callback;
    void *arg;
};

void klvm_fini_passes(List *list)
{
    KLVMPass *pass, *nxt;
    list_foreach_safe(pass, nxt, link, list, {
        list_remove(&pass->link);
        mm_free(pass);
    });
}

void klvm_register_pass(List *list, KLVMPassFunc fn, void *arg)
{
    KLVMPass *pass = mm_alloc_obj(pass);
    init_list(&pass->link);
    pass->callback = fn;
    pass->arg = arg;
    list_push_back(list, &pass->link);
}

static inline void _run_pass(List *list, KLVMFunc *fn)
{
    KLVMPass *pass;
    list_foreach(pass, link, list, { pass->callback(fn, pass->arg); });
}

void klvm_run_passes(List *list, KLVMModule *m)
{
    KLVMFunc **item;
    vector_foreach(item, &m->functions, { _run_pass(list, *item); });
}

/* Pass: Check unreachable block */

static int __is_visited(KLVMBasicBlock *bb, Vector *visited)
{
    KLVMBasicBlock **item;
    vector_foreach(item, visited, {
        if (*item == bb) return 1;
    });
    return 0;
}

static void __visit_block(KLVMBasicBlock *bb, Vector *visited)
{
    KLVMEdge *edge;
    edge_out_foreach(edge, &bb->out_edges, {
        vector_push_back(visited, &edge->dst);
        __visit_block(edge->dst, visited);
    });
}

static void __unreach_block_pass(KLVMFunc *fn, void *arg)
{
    Vector visited;
    vector_init_ptr(&visited);

    vector_push_back(&visited, &fn->sbb);
    __visit_block(fn->sbb, &visited);

    /* check unreachable blocks */
    KLVMBasicBlock *bb;
    basic_block_foreach(bb, &fn->bb_list, {
        if (!__is_visited(bb, &visited)) {
            printf("warn: bb: %s is unreachable\n", bb->label);
        }
    });
}

void klvm_add_unreachblock_pass(List *list)
{
    klvm_register_pass(list, __unreach_block_pass, null);
}

/* Pass: Generate graphviz dot file and pdf */

static inline int has_branch(KLVMBasicBlock *bb)
{
    KLVMInst *inst = list_last(&bb->inst_list, KLVMInst, link);
    if (!inst) return 0;
    return 0; //(inst->op == KLVM_INSN_COND_JMP) ? 1 : 0;
}

static inline int is_ebb(KLVMBasicBlock *bb)
{
    return bb == bb->func->ebb;
}

static void __dot_print_edge(KLVMBasicBlock *bb, FILE *fp)
{
    int branch = has_branch(bb);
    KLVMBasicBlock *out;
    KLVMEdge *edge;
    int i = 0;
    edge_out_foreach(edge, &bb->out_edges, {
        out = edge->dst;
        if (branch) {
            if (is_ebb(out)) {
                fprintf(fp, "  %s:%c -> %s:h\n", bb->label,
                        (i == 0) ? 't' : 'f', out->label);
            } else {
                fprintf(fp, "  %s:%c -> %s\n", bb->label, (i == 0) ? 't' : 'f',
                        out->label);
            }
        } else {
            if (is_ebb(out)) {
                fprintf(fp, "  %s -> %s\n", bb->label, out->label);
            } else {
                fprintf(fp, "  %s -> %s:h\n", bb->label, out->label);
            }
        }
        __dot_print_edge(out, fp);
        i++;
    });
}

static void __dot_print_block(KLVMBasicBlock *bb, FILE *fp)
{
    KLVMInst *inst;
    inst_foreach(inst, &bb->inst_list, {
        fprintf(fp, "\\l");
        klvm_print_inst((KLVMFunc *)bb->func, inst, fp);
    });
}

static void __dot_pass(KLVMFunc *fn, void *arg)
{
    char buf[512];
    snprintf(buf, 511, "./%s.dot", fn->name);

    FILE *fp = fopen(buf, "w");
    fprintf(fp, "digraph %s {\n", fn->name);

    fprintf(fp, "  node[shape=record margin=0.1 fontsize=12]\n");

    KLVMBasicBlock *bb;
    KLVMInst *inst;
    basic_block_foreach(bb, &fn->bb_list, {
        fprintf(fp, "  %s", bb->label);
        inst = list_last(&bb->inst_list, KLVMInst, link);
        if (inst && inst->opcode == KLVM_OP_COND_JMP) {
            fprintf(fp, "[label=\"{<h>%s:", bb->label);
            __dot_print_block(bb, fp);
            fprintf(fp, "|{<t>T|<f>F}}\"]\n");
        } else {
            fprintf(fp, "[label=\"{<h>%s:", bb->label);
            __dot_print_block(bb, fp);
            fprintf(fp, "\\l}\"]\n");
        }
    });

    __dot_print_edge(fn->sbb, fp);

    fprintf(fp, "}\n");
    fclose(fp);

    snprintf(buf, 511, "dot -Tpdf %s.dot -o %s.pdf", fn->name, fn->name);
    system(buf);
}

void klvm_add_dot_pass(List *list)
{
    klvm_register_pass(list, __dot_pass, null);
}

#ifdef __cplusplus
}
#endif

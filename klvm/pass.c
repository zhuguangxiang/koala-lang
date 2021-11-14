/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "klvm.h"
#include "opcode.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _KLVMPass KLVMPass;

struct _KLVMPass {
    List link;
    KLVMPassFunc callback;
    void *arg;
};

void klvm_init_passes(KLVMPassGroup *grp)
{
    init_list(&grp->list);
}

void klvm_fini_passes(KLVMPassGroup *grp)
{
    KLVMPass *pass, *nxt;
    list_foreach_safe(pass, nxt, link, &grp->list, {
        list_remove(&pass->link);
        mm_free(pass);
    });
}

void klvm_add_pass(KLVMPassGroup *grp, KLVMPassFunc fn, void *arg)
{
    KLVMPass *pass = mm_alloc_obj(pass);
    init_list(&pass->link);
    pass->callback = fn;
    pass->arg = arg;
    list_push_back(&grp->list, &pass->link);
}

static inline void _run_pass(KLVMPassGroup *grp, KLVMFunc *fn)
{
    KLVMPass *pass;
    list_foreach(pass, link, &grp->list, { pass->callback(fn, pass->arg); });
}

void klvm_run_passes(KLVMPassGroup *grp, KLVMModule *m)
{
    KLVMFunc **item;
    vector_foreach(item, &m->functions, { _run_pass(grp, *item); });
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
    edge_out_foreach(edge, bb, {
        if (__is_visited(edge->dst, visited)) continue;
        vector_push_back(visited, &edge->dst);
        __visit_block(edge->dst, visited);
    });
}

void klvm_check_unused_block_pass(KLVMFunc *fn, void *arg)
{
    Vector visited;
    vector_init_ptr(&visited);

    vector_push_back(&visited, &fn->sbb);
    __visit_block(fn->sbb, &visited);

    /* check unreachable blocks */
    KLVMBasicBlock *bb;
    basic_block_foreach(bb, fn, {
        if (!__is_visited(bb, &visited)) {
            printf("warn: bb: %s is unreachable\n", bb->name);
        }
    });
}

/* Pass: Generate graphviz dot file and pdf */

static inline int has_branch(KLVMBasicBlock *bb)
{
    KLVMInst *inst = inst_last(bb);
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
    edge_out_foreach(edge, bb, {
        out = edge->dst;
        if (branch) {
            if (is_ebb(out)) {
                fprintf(fp, "  %s:%c -> %s:h\n", bb->name, (i == 0) ? 't' : 'f',
                        out->name);
            } else {
                fprintf(fp, "  %s:%c -> %s\n", bb->name, (i == 0) ? 't' : 'f',
                        out->name);
            }
        } else {
            if (is_ebb(out)) {
                fprintf(fp, "  %s -> %s\n", bb->name, out->name);
            } else {
                fprintf(fp, "  %s -> %s:h\n", bb->name, out->name);
            }
        }
        __dot_print_edge(out, fp);
        i++;
    });
}

static void __dot_print_block(KLVMBasicBlock *bb, FILE *fp)
{
    KLVMInst *inst;
    inst_foreach(inst, bb, {
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
    basic_block_foreach(bb, fn, {
        fprintf(fp, "  %s", bb->name);
        inst = inst_last(bb);
        if (inst && inst->opcode == KLVM_OP_COND_JMP) {
            fprintf(fp, "[label=\"{<h>%s:", bb->name);
            __dot_print_block(bb, fp);
            fprintf(fp, "|{<t>T|<f>F}}\"]\n");
        } else {
            fprintf(fp, "[label=\"{<h>%s:", bb->name);
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

void klvm_add_dot_pass(KLVMPassGroup *grp)
{
    klvm_add_pass(grp, __dot_pass, null);
}

void klvm_check_unused_value_pass(KLVMFunc *fn, void *arg)
{
    KLVMBasicBlock *bb;
    KLVMLocal *local;
    basic_block_foreach(bb, fn, {
        local_foreach(local, bb, {
            if (list_empty(&local->use_list)) {
                if (local->name[0]) {
                    printf("warn: unused '%%%s'\n", local->name);
                } else {
                    printf("warn: unused '%%%d'\n", 0);
                }
            }
        });
    });
}

void klvm_print_liveness_pass(KLVMFunc *fn, void *arg)
{
    klvm_analyze_liveness(fn);

    klvm_print_liveness(fn, stdout);
}

#ifdef __cplusplus
}
#endif

/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2020-2021 James <zhuguangxiang@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "klvm/klvm_insn.h"

#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------*\
|* Pass: Check unreachable block                                            *|
\*--------------------------------------------------------------------------*/

static int __is_visited(KLVMBasicBlockRef bb, VectorRef visited)
{
    KLVMBasicBlockRef *item;
    VectorForEach(item, visited, {
        if (*item == bb) return 1;
    });
    return 0;
}

static void __visit_block(KLVMBasicBlockRef bb, VectorRef visited)
{
    KLVMEdgeRef edge;
    EdgeOutForEach(edge, &bb->out_edges, {
        VectorPushBack(visited, &edge->dst);
        __visit_block(edge->dst, visited);
    });
}

static void __unreach_block_pass(KLVMFuncRef fn, void *arg)
{
    Vector visited;
    VectorInitPtr(&visited);

    VectorPushBack(&visited, fn->sbb);
    __visit_block(fn->sbb, &visited);

    /* check unreachable blocks */
    KLVMBasicBlockRef bb;
    BasicBlockForEach(bb, &fn->bb_list, {
        if (!__is_visited(bb, &visited)) {
            printf("warn: bb: %s is unreachable\n", bb->label);
        }
    });
}

void KLVMAddUnReachBlockPass(KLVMPassGroupRef grp)
{
    KLVMPass pass = { __unreach_block_pass, NULL };
    KLVMRegisterPass(grp, &pass);
}

/*--------------------------------------------------------------------------*\
|* Pass: Generate graphviz dot file and pdf                                 *|
\*--------------------------------------------------------------------------*/

static int has_branch(KLVMBasicBlockRef bb)
{
    KLVMInsnRef insn;
    insn = ListLast(&bb->insns, KLVMInsn, link);
    if (!insn) return 0;
    return (insn->op == KLVM_INSN_COND_JMP) ? 1 : 0;
}

static int is_ebb(KLVMBasicBlockRef bb)
{
    KLVMFuncRef fn = (KLVMFuncRef)bb->fn;
    return bb == fn->ebb;
}

static void __dot_print_edge(KLVMBasicBlockRef bb, FILE *fp)
{
    int branch = has_branch(bb);
    KLVMBasicBlockRef out;
    KLVMEdgeRef edge;
    int i = 0;
    EdgeOutForEach(edge, &bb->out_edges, {
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

static void __dot_print_block(KLVMBasicBlockRef bb, FILE *fp)
{
    KLVMInsnRef insn;
    InsnForEach(insn, &bb->insns, {
        fprintf(fp, "\\l");
        KLVMPrintInsn((KLVMFuncRef)bb->fn, insn, fp);
    });
}

static void __dot_pass(KLVMFuncRef fn, void *arg)
{
    char buf[512];
    snprintf(buf, 511, "./%s.dot", fn->name);

    FILE *fp = fopen(buf, "w");
    fprintf(fp, "digraph %s {\n", fn->name);

    fprintf(fp, "  node[shape=record margin=0.1 fontsize=12]\n");

    KLVMBasicBlockRef bb;
    KLVMInsnRef insn;
    BasicBlockForEach(bb, &fn->bb_list, {
        fprintf(fp, "  %s", bb->label);
        insn = ListLast(&bb->insns, KLVMInsn, link);
        if (insn->op == KLVM_INSN_COND_JMP) {
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

void KLVMAddDotPass(KLVMPassGroupRef grp)
{
    KLVMPass pass = { __dot_pass, NULL };
    KLVMRegisterPass(grp, &pass);
}

#ifdef __cplusplus
}
#endif

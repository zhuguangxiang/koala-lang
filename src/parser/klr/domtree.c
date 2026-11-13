/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "domtree.h"
#include "cmd.h"

#ifdef __cplusplus
extern "C" {
#endif

static int block_id(KlrDomTree *kdt, KlrBasicBlock *bb)
{
    int id = bb->dom_index;
    /* a block added after the tree was built */
    ASSERT(id >= 0 && id < kdt->num_blocks && kdt->blocks[id] == bb);
    return id;
}

void klr_domtree_build(KlrDomTree *kdt, KlrFunc *fn)
{
    memset(kdt, 0, sizeof(*kdt));
    kdt->fn = fn;

    int n = 2;
    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) n++;

    kdt->num_blocks = n;
    kdt->blocks = mm_alloc(sizeof(KlrBasicBlock *) * n);

    int id = 0;
    kdt->blocks[id] = fn->sbb;
    fn->sbb->dom_index = id++;
    basic_block_foreach(bb, fn) {
        kdt->blocks[id] = bb;
        bb->dom_index = id++;
    }
    kdt->blocks[id] = fn->ebb;
    fn->ebb->dom_index = id++;

    int num_edges = 0;
    for (int i = 0; i < n; i++) num_edges += kdt->blocks[i]->num_outedges;

    int *edges = mm_alloc(sizeof(int) * 2 * (num_edges + 1));
    int ne = 0;
    for (int i = 0; i < n; i++) {
        KlrEdge *edge;
        edge_out_foreach(edge, kdt->blocks[i]) {
            edges[2 * ne] = i;
            edges[2 * ne + 1] = block_id(kdt, edge->dst);
            ne++;
        }
    }

    int ret = dom_tree_build(&kdt->dt, n, 0, edges, ne);
    ASSERT(ret == 0);
    UNUSED(ret);
    mm_free(edges);
}

void klr_domtree_fini(KlrDomTree *kdt)
{
    dom_tree_fini(&kdt->dt);
    mm_free(kdt->blocks);
    memset(kdt, 0, sizeof(*kdt));
}

int klr_reachable(KlrDomTree *kdt, KlrBasicBlock *bb)
{
    return dom_reachable(&kdt->dt, block_id(kdt, bb));
}

KlrBasicBlock *klr_idom(KlrDomTree *kdt, KlrBasicBlock *bb)
{
    int id = block_id(kdt, bb);
    int idom = dom_idom(&kdt->dt, id);
    if (idom < 0 || idom == id) return NULL;
    return kdt->blocks[idom];
}

int klr_dominates(KlrDomTree *kdt, KlrBasicBlock *a, KlrBasicBlock *b)
{
    return dom_dominates(&kdt->dt, block_id(kdt, a), block_id(kdt, b));
}

int klr_is_back_edge(KlrDomTree *kdt, KlrEdge *edge)
{
    return klr_dominates(kdt, edge->dst, edge->src);
}

static char *block_name(KlrDomTree *kdt, KlrBasicBlock *bb)
{
    if (bb == kdt->fn->sbb) return "start";
    if (bb == kdt->fn->ebb) return "end";
    return klr_block_name(bb);
}

static void print_node(KlrDomTree *kdt, int id, FILE *fp)
{
    DomTree *dt = &kdt->dt;
    fprintf(fp, "%*s%s", 4 + 2 * dt->depth[id], "", block_name(kdt, kdt->blocks[id]));

    int num_df = dom_num_frontier(dt, id);
    if (num_df > 0) {
        fprintf(fp, "  df:");
        for (int i = 0; i < num_df; i++) {
            fprintf(fp, " %s", block_name(kdt, kdt->blocks[dom_frontier(dt, id)[i]]));
        }
    }
    fprintf(fp, "\n");

    for (int i = 0; i < dom_num_children(dt, id); i++) {
        print_node(kdt, dom_children(dt, id)[i], fp);
    }
}

void klr_domtree_print(KlrDomTree *kdt, FILE *fp)
{
    KlrFunc *fn = kdt->fn;
    update_tags(fn);

    if (fn->klass)
        fprintf(fp, "  func @%s:%s\n", fn->klass->name, fn->name);
    else
        fprintf(fp, "  func @%s\n", fn->name);

    print_node(kdt, kdt->dt.entry, fp);

    int first = 1;
    for (int i = 0; i < kdt->num_blocks; i++) {
        KlrBasicBlock *bb = kdt->blocks[i];
        if (klr_reachable(kdt, bb)) continue;
        fprintf(fp, first ? "    unreachable: %s" : ", %s", block_name(kdt, bb));
        first = 0;
    }
    if (!first) fprintf(fp, "\n");

    first = 1;
    for (int i = 0; i < kdt->num_blocks; i++) {
        KlrBasicBlock *bb = kdt->blocks[i];
        if (!klr_reachable(kdt, bb)) continue;
        KlrEdge *edge;
        edge_out_foreach(edge, bb) {
            if (!klr_is_back_edge(kdt, edge)) continue;
            fprintf(fp, first ? "    back edges: %s -> " : ", %s -> ", block_name(kdt, bb));
            fprintf(fp, "%s", block_name(kdt, edge->dst));
            first = 0;
        }
    }
    if (!first) fprintf(fp, "\n");
}

void klr_func_free_domtree(KlrFunc *fn)
{
    if (!fn->domtree) return;
    klr_domtree_fini(fn->domtree);
    mm_free(fn->domtree);
    fn->domtree = NULL;
}

static void build_func(KlrFunc *fn)
{
    klr_func_free_domtree(fn);
    fn->domtree = mm_alloc_obj(fn->domtree);
    klr_domtree_build(fn->domtree, fn);
}

static void dump_func(KlrFunc *fn)
{
    if (!fn->domtree) return;
    fprintf(stdout, "--- Dominator Tree ---\n");
    klr_domtree_print(fn->domtree, stdout);
}

/* functions of the module, then methods of its classes */
static void foreach_func(KlrModule *m, void (*cb)(KlrFunc *))
{
    if (!m || m->errors > 0) return;

    KlrFunc *fn;
    func_foreach(fn, m) {
        if (fn) cb(fn);
    }

    KlrKlass *kls;
    vector_foreach(kls, &m->klasses) {
        func_foreach(fn, kls) {
            cb(fn);
        }
    }
}

void kl_domtree(KlrModule *m)
{
    foreach_func(m, build_func);
    if (dump_domtree_enabled()) foreach_func(m, dump_func);
}

#ifdef __cplusplus
}
#endif

/*----------------------------------------------------------------------------*\
|* This file is part of the koala project, under the MIT License.             *|
|* Copyright (c) 2021-2021 James <zhuguangxiang@gmail.com>                    *|
\*----------------------------------------------------------------------------*/

#include "klvm.h"

#ifdef __cplusplus
extern "C" {
#endif

static void visit_block(klvm_block_t *bb)
{
    klvm_edge_t *edge;
    list_node_t *n;
    list_foreach(&bb->out_edges, n)
    {
        edge = list_entry(n, klvm_edge_t, out_link);
        edge->dst->visited = 1;
        visit_block(edge->dst);
    }
}

static void check_block(klvm_func_t *fn)
{
    vector_t removed;
    vector_init(&removed, sizeof(void *));

    klvm_block_t *bb;
    list_node_t *n;
    list_foreach(&fn->bblist, n)
    {
        bb = list_entry(n, klvm_block_t, bbnode);
        if (!bb->visited) {
            printf("warn: bb: %s is unreachable\n", bb->label);
            vector_push_back(&removed, &bb);
        }
        bb->visited = 0;
    }

    klvm_block_t **pbb;
    vector_foreach(pbb, &removed)
    {
        klvm_delete_block(*pbb);
    }

    vector_fini(&removed);
}

static void block_pass(klvm_func_t *fn, void *data)
{
    printf("func: %s\n", fn->name);
    klvm_block_t *sbb = &fn->sbb;
    visit_block(sbb);
    check_block(fn);
}

void block_pass_register(klvm_module_t *m)
{
    klvm_pass_t pass = { block_pass, NULL };
    klvm_register_pass(m, &pass);
}

#ifdef __cplusplus
}
#endif

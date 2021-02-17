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

static void dot_pass(klvm_func_t *fn, void *data)
{
    printf("func: %s\n", fn->name);
    klvm_block_t *sbb = &fn->sbb;
    show_block(sbb);
}

void dot_pass_register(klvm_module_t *m)
{
    klvm_pass_t pass = { dot_pass, NULL };
    klvm_register_pass(m, &pass);
}

#ifdef __cplusplus
}
#endif

/*----------------------------------------------------------------------------*\
|* This file is part of the koala project, under the MIT License.             *|
|* Copyright (c) 2021-2021 James <zhuguangxiang@gmail.com>                    *|
\*----------------------------------------------------------------------------*/

#include "klvm.h"

#ifdef __cplusplus
extern "C" {
#endif

void klvm_link_edge(klvm_block_t *src, klvm_block_t *dst)
{
    klvm_edge_t *edge = malloc(sizeof(*edge));
    edge->src = src;
    edge->dst = dst;
    list_node_init(&edge->link);
    list_node_init(&edge->in_link);
    list_node_init(&edge->out_link);
    klvm_func_t *fn = (klvm_func_t *)src->fn;
    list_push_back(&fn->edgelist, &edge->link);
    list_push_back(&src->out_edges, &edge->out_link);
    list_push_back(&dst->in_edges, &edge->in_link);
}

static void _init_block(klvm_block_t *bb, short dummy, char *label)
{
    list_node_init(&bb->bbnode);
    list_init(&bb->insts);
    list_init(&bb->in_edges);
    list_init(&bb->out_edges);
    bb->label = label;
    bb->dummy = dummy;
    bb->tag = -1;
}

static klvm_func_t *_new_func(klvm_type_t *ty, char *name)
{
    if (!name) {
        printf("error: function must have name\n");
        abort();
    }

    klvm_func_t *fn = malloc(sizeof(*fn));
    KLVM_INIT_VALUE_HEAD(fn, KLVM_VALUE_FUNC, ty, name);
    vector_init(&fn->locals, sizeof(void *));
    list_init(&fn->bblist);

    // add parameters
    vector_t *params = klvm_proto_params(ty);
    fn->num_params = vector_size(params);
    klvm_type_t **item;
    vector_foreach(item, params) klvm_new_local((klvm_value_t *)fn, *item, "");

    // initial 'start' and 'end' block
    _init_block(&fn->sbb, 1, "start");
    fn->sbb.fn = (klvm_value_t *)fn;
    _init_block(&fn->ebb, 1, "end");
    fn->ebb.fn = (klvm_value_t *)fn;

    // initial edge list
    list_init(&fn->edgelist);

    return fn;
}

klvm_module_t *klvm_create_module(char *name)
{
    klvm_module_t *m = malloc(sizeof(*m));
    m->name = name;
    vector_init(&m->vars, sizeof(void *));
    vector_init(&m->funcs, sizeof(void *));
    list_init(&m->passes);
    return m;
}

klvm_value_t *klvm_init_func(klvm_module_t *m)
{
    klvm_value_t *fn = m->_init_;
    if (!fn) {
        klvm_type_t *ty = klvm_type_proto(NULL, NULL);
        fn = (klvm_value_t *)_new_func(ty, "__init__");
        m->_init_ = fn;
    }
    return fn;
}

void klvm_destroy_module(klvm_module_t *m)
{
    free(m);
}

static klvm_const_t *_new_const(void)
{
    klvm_const_t *k = malloc(sizeof(*k));
    k->kind = KLVM_VALUE_CONST;
    return k;
}

klvm_value_t *klvm_const_int32(int32_t ival)
{
    klvm_const_t *k = _new_const();
    k->type = &klvm_type_int32;
    k->i32val = ival;
    return (klvm_value_t *)k;
}

static klvm_var_t *_new_var(klvm_type_t *ty, char *name)
{
    if (!name) {
        printf("error: variable must have name\n");
        abort();
    }

    klvm_var_t *var = malloc(sizeof(*var));
    KLVM_INIT_VALUE_HEAD(var, KLVM_VALUE_VAR, ty, name);
    return var;
}

klvm_value_t *klvm_new_var(klvm_module_t *m, klvm_type_t *ty, char *name)
{
    klvm_var_t *var = _new_var(ty, name);
    vector_push_back(&m->vars, &var);
    return (klvm_value_t *)var;
}

klvm_value_t *klvm_new_func(klvm_module_t *m, klvm_type_t *ty, char *name)
{
    klvm_func_t *fn = _new_func(ty, name);
    // add to module
    vector_push_back(&m->funcs, &fn);
    return (klvm_value_t *)fn;
}

klvm_value_t *klvm_new_local(klvm_value_t *fn, klvm_type_t *ty, char *name)
{
    klvm_var_t *var = _new_var(ty, name);
    var->attr = KLVM_ATTR_LOCAL;
    vector_push_back(&((klvm_func_t *)fn)->locals, &var);
    return (klvm_value_t *)var;
}

klvm_value_t *klvm_get_param(klvm_value_t *fn, int index)
{
    klvm_func_t *_fn = (klvm_func_t *)fn;
    int num_params = _fn->num_params;
    if (index < 0 || index >= num_params) {
        printf("error: index %d out of range(0 ..< %d)\n", index, num_params);
        abort();
    }

    klvm_value_t *para = NULL;
    vector_get(&_fn->locals, index, &para);
    return para;
}

static void _append_block(klvm_func_t *fn, klvm_block_t *bb)
{
    bb->fn = (klvm_value_t *)fn;

    if (list_is_empty(&fn->bblist)) {
        // first basic block, create an edge <start, bb>
        klvm_link_edge(&fn->sbb, bb);
    }

    list_push_back(&fn->bblist, &bb->bbnode);
    fn->num_bbs++;
}

klvm_block_t *klvm_append_block(klvm_value_t *fn, char *name)
{
    klvm_block_t *bb = malloc(sizeof(*bb));
    _init_block(bb, 0, name);
    _append_block((klvm_func_t *)fn, bb);
    return bb;
}

void klvm_delete_block(klvm_block_t *bb)
{
    list_remove(&bb->bbnode);
    free(bb);
}

typedef struct klvm_pass_info {
    list_node_t node;
    klvm_pass_run_t run;
    void *data;
} klvm_pass_info;

void klvm_register_pass(klvm_module_t *m, klvm_pass_t *pass)
{
    klvm_pass_info *pi = malloc(sizeof(*pi));
    list_node_init(&pi->node);
    pi->run = pass->run;
    pi->data = pass->data;
    list_push_back(&m->passes, &pi->node);
}

void klvm_run_passes(klvm_module_t *m)
{
    klvm_func_t **fn;
    list_node_t *n;
    klvm_pass_info *pass;
    vector_foreach(fn, &m->funcs)
    {
        list_foreach(&m->passes, n)
        {
            pass = (klvm_pass_info *)n;
            pass->run(*fn, pass->data);
        }
    }
}

#ifdef __cplusplus
}
#endif

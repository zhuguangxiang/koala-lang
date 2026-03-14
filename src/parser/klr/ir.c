/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "ir.h"
#include "log.h"
#include "mm.h"

#ifdef __cplusplus
extern "C" {
#endif

KlrValue *klr_const_int(uint64_t val, TypeSpec *ts, KlrModule *m)
{
    KlrConst key = { .which = CONST_INT, .ival = val };
    hashmap_entry_init(&key.hnode, mem_hash(&val, sizeof(val)));
    void *entry = hashmap_get(&m->consts, &key.hnode);
    if (entry) {
        return (KlrValue *)CONTAINER_OF(entry, KlrConst, hnode);
    }

    KlrConst *lit = mm_alloc_obj(lit);
    INIT_KLR_VALUE(lit, KLR_VALUE_CONST, ts, "");
    lit->which = CONST_INT;
    lit->ival = val;
    hashmap_entry_init(&lit->hnode, mem_hash(&val, sizeof(val)));
    hashmap_put(&m->consts, &lit->hnode);
    return (KlrValue *)lit;
}

KlrValue *klr_const_float(double val, TypeSpec *ts, KlrModule *m)
{
    KlrConst key = { .which = CONST_FLT, .fval = val };
    hashmap_entry_init(&key.hnode, mem_hash(&val, sizeof(val)));
    void *entry = hashmap_get(&m->consts, &key.hnode);
    if (entry) {
        return (KlrValue *)CONTAINER_OF(entry, KlrConst, hnode);
    }

    KlrConst *lit = mm_alloc_obj(lit);
    INIT_KLR_VALUE(lit, KLR_VALUE_CONST, ts, "");
    lit->which = CONST_FLT;
    lit->fval = val;
    hashmap_entry_init(&lit->hnode, mem_hash(&val, sizeof(val)));
    hashmap_put(&m->consts, &lit->hnode);
    return (KlrValue *)lit;
}

KlrValue *klr_const_bool(int v, KlrModule *m)
{
    KlrConst key = { .which = CONST_BOOL, .bval = v };
    hashmap_entry_init(&key.hnode, mem_hash(&v, sizeof(v)));
    void *entry = hashmap_get(&m->consts, &key.hnode);
    if (entry) {
        return (KlrValue *)CONTAINER_OF(entry, KlrConst, hnode);
    }

    KlrConst *lit = mm_alloc_obj(lit);
    TypeSpec *ts = bool_type_spec();
    INIT_KLR_VALUE(lit, KLR_VALUE_CONST, ts, "");
    lit->which = CONST_BOOL;
    lit->bval = v;
    hashmap_entry_init(&lit->hnode, mem_hash(&v, sizeof(v)));
    hashmap_put(&m->consts, &lit->hnode);
    return (KlrValue *)lit;
}

KlrValue *klr_const_str(char *s, int len, KlrModule *m)
{
    KlrConst key = { .which = CONST_STR, .sval = s, .len = len };
    hashmap_entry_init(&key.hnode, mem_hash(s, len));
    void *entry = hashmap_get(&m->consts, &key.hnode);
    if (entry) {
        return (KlrValue *)CONTAINER_OF(entry, KlrConst, hnode);
    }

    KlrConst *lit = mm_alloc_obj(lit);
    TypeSpec *ts = str_type_spec();
    INIT_KLR_VALUE(lit, KLR_VALUE_CONST, ts, "");
    lit->which = CONST_STR;
    lit->len = len;
    lit->sval = s;
    hashmap_entry_init(&lit->hnode, mem_hash(s, len));
    hashmap_put(&m->consts, &lit->hnode);
    return (KlrValue *)lit;
}

KlrValue *klr_const_none(KlrModule *m)
{
    KlrConst key = { .which = CONST_NONE, .sval = "none", .len = 4 };
    hashmap_entry_init(&key.hnode, mem_hash("none", 4));
    void *entry = hashmap_get(&m->consts, &key.hnode);
    if (entry) {
        return (KlrValue *)CONTAINER_OF(entry, KlrConst, hnode);
    }

    KlrConst *lit = mm_alloc_obj(lit);
    INIT_KLR_VALUE(lit, KLR_VALUE_CONST, NULL, "");
    lit->which = CONST_NONE;
    lit->len = 4;
    lit->sval = "none";
    hashmap_entry_init(&lit->hnode, mem_hash("none", 4));
    hashmap_put(&m->consts, &lit->hnode);
    return (KlrValue *)lit;
}

KlrValue *klr_const_list(KlrValue **items, int size, TypeSpec *ts, KlrModule *m)
{
    KlrConst *lit = mm_alloc_obj(lit);
    INIT_KLR_VALUE(lit, KLR_VALUE_CONST, ts, "");
    lit->which = CONST_LIST;
    lit->len = size;
    KlrValue **copy = mm_alloc(size * sizeof(KlrValue *));
    memcpy(copy, items, size * sizeof(KlrValue *));
    lit->list.items = copy;
    return (KlrValue *)lit;
}

KlrValue *klr_const_tuple(KlrValue **items, int size, TypeSpec *ts, KlrModule *m)
{
    KlrValue *lit = klr_const_list(items, size, ts, m);
    ((KlrConst *)lit)->which = CONST_TUPLE;
    return lit;
}

typedef struct _LocalVarMapEntry {
    HashMapEntry hnode;
    KlrInsn *local;
    KlrConst *val;
} LocalVarMapEntry;

static int __local_var_eq__(void *a, void *b)
{
    LocalVarMapEntry *e1 = (LocalVarMapEntry *)a;
    LocalVarMapEntry *e2 = (LocalVarMapEntry *)b;
    return e1->local == e2->local;
}

int klr_update_local_var_const(KlrBasicBlock *bb, KlrInsn *local, KlrConst *val)
{
    ASSERT(klr_is_local((KlrValue *)local));
    ASSERT(!(local->flags & KLR_INSN_FLAGS_CONST));

    LocalVarMapEntry key = { .local = local };
    hashmap_entry_init(&key.hnode, mem_hash(&local, sizeof(local)));
    LocalVarMapEntry *entry = hashmap_get(&bb->local_var_map, &key);
    if (entry) {
        entry->val = val;
    } else {
        entry = mm_alloc_obj(entry);
        hashmap_entry_init(&entry->hnode, mem_hash(&local, sizeof(local)));
        entry->local = local;
        entry->val = val;
        hashmap_put(&bb->local_var_map, entry);
    }
    return 0;
}

int klr_clear_local_var_const(KlrBasicBlock *bb, KlrInsn *local)
{
    ASSERT(klr_is_local((KlrValue *)local));
    ASSERT(!(local->flags & KLR_INSN_FLAGS_CONST));

    LocalVarMapEntry key = { .local = local };
    hashmap_entry_init(&key.hnode, mem_hash(&local, sizeof(local)));
    LocalVarMapEntry *entry = hashmap_get(&bb->local_var_map, &key);
    if (entry) entry->val = NULL;
    return 0;
}

KlrValue *klr_get_local_var_const(KlrBasicBlock *bb, KlrInsn *local)
{
    ASSERT(klr_is_local((KlrValue *)local));

    LocalVarMapEntry key = { .local = local };
    hashmap_entry_init(&key.hnode, mem_hash(&local, sizeof(local)));
    LocalVarMapEntry *entry = hashmap_get(&bb->local_var_map, &key);
    if (entry) {
        ASSERT(!(local->flags & KLR_INSN_FLAGS_CONST));
        ASSERT(!(entry->local->flags & KLR_INSN_FLAGS_CONST));
        KlrValue *val = (KlrValue *)entry->val;
        ASSERT(klr_is_const(val));
        return val;
    }
    return NULL;
}

int klr_clear_local_var_map(KlrBasicBlock *bb)
{
    HashMap *map = &bb->local_var_map;
    HashMapIter it = { 0 };
    while (hashmap_next(map, &it)) {
        LocalVarMapEntry *e = (LocalVarMapEntry *)it.entry;
        e->val = NULL;
    }
    return 0;
}

static KlrBasicBlock *new_block(KlrFunc *fn, char *name)
{
    KlrBasicBlock *bb = mm_alloc_obj(bb);
    INIT_KLR_VALUE(bb, KLR_VALUE_BLOCK, NULL, name);

    init_list(&bb->link);
    bb->func = fn;

    init_list(&bb->local_list);
    init_list(&bb->insn_list);
    init_list(&bb->in_edges);
    init_list(&bb->out_edges);

    // init_list(&bb->phi_list);
    // vector_init(&bb->phis, PTR_SIZE);

    hashmap_init(&bb->local_var_map, __local_var_eq__);

    return bb;
}

KlrBasicBlock *klr_append_block(KlrValue *fn_val, char *label)
{
    KlrFunc *fn = (KlrFunc *)fn_val;
    KlrBasicBlock *bb = new_block(fn, label);

    if (list_empty(&fn->bb_list)) {
        /* first block, add an edge <start, bb> */
        klr_link_edge(fn->sbb, bb);
    }

    list_push_back(&fn->bb_list, &bb->link);

    return bb;
}

KlrBasicBlock *klr_add_block(KlrBasicBlock *bb, char *label)
{
    KlrFunc *fn = bb->func;
    KlrBasicBlock *_bb = new_block(fn, label);
    list_add(&bb->link, &_bb->link);
    return _bb;
}

KlrBasicBlock *klr_add_block_before(KlrBasicBlock *bb, char *label)
{
    KlrFunc *fn = bb->func;
    KlrBasicBlock *_bb = new_block(fn, label);
    list_add_before(&bb->link, &_bb->link);
    return _bb;
}

void klr_delete_block(KlrBasicBlock *bb)
{
    list_remove(&bb->link);

    KlrEdge *edge, *nxt;

    edge_out_foreach_safe(edge, nxt, bb) {
        klr_remove_edge(edge);
    }

    edge_in_foreach_safe(edge, nxt, bb) {
        klr_remove_edge(edge);
    }

    KlrInsn *insn, *nxt_insn;
    insn_foreach_safe(insn, nxt_insn, bb) {
        klr_erase_insn(insn);
    }

    mm_free(bb);
}

KlrBasicBlock *klr_last_block(KlrValue *fn_val)
{
    KlrFunc *fn = (KlrFunc *)fn_val;
    if (list_empty(&fn->bb_list)) {
        return NULL;
    }

    KlrBasicBlock *last = list_last(&fn->bb_list, KlrBasicBlock, link);
    return last;
}

void Klr_merge_block(KlrBasicBlock *dst, KlrBasicBlock *src)
{
    ASSERT(dst->func == src->func);
    KlrFunc *fn = dst->func;

    log_info("merge block '%%%s' into '%%%s'", klr_block_name(src), klr_block_name(dst));

    KlrInsn *last = insn_last(dst);
    if (last && last->code == OP_JMP) {
        log_info("remove jmp insn in block '%%%s'", klr_block_name(dst));
        klr_erase_insn(last);
    }

    /* move all instructions from src to dst */
    KlrInsn *insn, *nxt;
    insn_foreach_safe(insn, nxt, src) {
        list_remove(&insn->bb_link);
        list_push_back(&dst->insn_list, &insn->bb_link);
        insn->bb = dst;
        ++dst->num_insns;
        --src->num_insns;
    }

    /* update out-edges */
    KlrEdge *edge, *nxt_edge;
    edge_out_foreach_safe(edge, nxt_edge, src) {
        klr_link_edge(dst, edge->dst);
        klr_remove_edge(edge);
    }

    /* update in-edges */
    edge_in_foreach_safe(edge, nxt_edge, src) {
        klr_remove_edge(edge);
    }
}

static inline int insn_is_terminator(KlrInsn *insn)
{
    if (!insn) return 0;

    switch (insn->code) {
        case OP_RETURN:
        case OP_RETURN_NONE:
        case OP_IR_JMP_COND:
        case OP_JMP:
            return 1;
        default:
            return 0;
    }
}

int block_has_terminator(KlrBasicBlock *bb)
{
    KlrInsn *last = insn_last(bb);
    if (!last) return 0;
    return insn_is_terminator(last);
}

void klr_add_last_return(KlrBasicBlock *bb)
{
    KlrInsn *last = insn_last(bb);

    if (last && insn_is_terminator(last)) return;

    KlrBuilder bldr;
    klr_builder_end(&bldr, bb);
    klr_build_ret_void(&bldr);
}

void klr_link_edge(KlrBasicBlock *src, KlrBasicBlock *dst)
{
    KlrEdge *edge = mm_alloc_obj_fast(edge);
    edge->src = src;
    edge->dst = dst;
    init_list(&edge->link);
    init_list(&edge->in_link);
    init_list(&edge->out_link);

    KlrFunc *fn = src->func;
    list_push_back(&fn->edge_list, &edge->link);
    list_push_back(&src->out_edges, &edge->out_link);
    list_push_back(&dst->in_edges, &edge->in_link);
    ++src->num_outedges;
    ++dst->num_inedges;
}

void klr_remove_edge(KlrEdge *edge)
{
    ASSERT(edge);
    --edge->src->num_outedges;
    --edge->dst->num_inedges;
    list_remove(&edge->link);
    list_remove(&edge->in_link);
    list_remove(&edge->out_link);
    mm_free(edge);
}

void klr_remove_all_out_edges(KlrBasicBlock *bb)
{
    KlrEdge *edge, *nxt;
    edge_out_foreach_safe(edge, nxt, bb) {
        klr_remove_edge(edge);
    }
}

static int __const_eq__(void *e1, void *e2)
{
    KlrConst *k1 = CONTAINER_OF(e1, KlrConst, hnode);
    KlrConst *k2 = CONTAINER_OF(e2, KlrConst, hnode);

    if (k1->which != k2->which) {
        return 0;
    }

    switch (k1->which) {
        case CONST_INT:
            return k1->ival == k2->ival;
        case CONST_FLT:
            return k1->fval == k2->fval;
        case CONST_BOOL:
            return k1->bval == k2->bval;
        case CONST_STR:
            return (k1->len == k2->len) && !strcmp(k1->sval, k2->sval);
        default:
            UNREACHABLE();
    }
}

KlrModule *klr_create_module(char *name)
{
    KlrModule *m = mm_alloc_obj_fast(m);
    m->name = name;
    vector_init_ptr(&m->globals);
    vector_init_ptr(&m->functions);
    vector_init_ptr(&m->ext_syms);
    vector_init_ptr(&m->klasses);
    hashmap_init(&m->consts, (HashMapEqualFunc)__const_eq__);
    m->init = NULL;
    return m;
}

void klr_destroy_module(KlrModule *m) {}

KlrValue *klr_add_func(KlrModule *m, TypeSpec *ret, char *name)
{
    KlrFunc *fn = mm_alloc_obj(fn);
    INIT_KLR_VALUE(fn, KLR_VALUE_FUNC, ret, name);

    init_list(&fn->bb_list);
    init_list(&fn->edge_list);
    vector_init_ptr(&fn->params);
    vector_init_ptr(&fn->locals);

    /* initial 'start' and 'end' block */
    fn->sbb = new_block(fn, "start");
    fn->ebb = new_block(fn, "end");

    vector_push_back(&m->functions, &fn);
    fn->mod = m;
    return (KlrValue *)fn;
}

KlrValue *klr_func_get_param(KlrValue *val, int index)
{
    KlrFunc *func = (KlrFunc *)val;

    int size = vector_size(&func->params);
    if (index < 0 || index >= size) {
        panic("index %d out of range(0 ..< %d)", index, size);
    }

    KlrValue *item = vector_get(&func->params, index);
    return item;
}

KlrValue *klr_func_add_param(KlrValue *val, TypeSpec *ts, char *name)
{
    KlrFunc *fn = (KlrFunc *)val;
    KlrParam *param = mm_alloc_obj(param);
    INIT_KLR_VALUE(param, KLR_VALUE_PARAM, ts, name);
    vector_push_back(&fn->params, &param);
    return (KlrValue *)param;
}

static KlrGlobal *new_global(TypeSpec *ts, char *name)
{
    KlrGlobal *global = mm_alloc_obj(global);
    INIT_KLR_VALUE(global, KLR_VALUE_GLOBAL, ts, name);
    return global;
}

KlrValue *klr_add_global(KlrModule *m, TypeSpec *ts, char *name, int mut)
{
    KlrGlobal *global = new_global(ts, name);
    vector_push_back(&m->globals, &global);
    global->mutable = mut;
    return (KlrValue *)global;
}

KlrValue *klr_add_ext_func(KlrModule *m, TypeSpec *ret, char *path, char *name)
{
    KlrExtFunc *fn = mm_alloc_obj(fn);
    INIT_KLR_VALUE(fn, KLR_VALUE_EXT_FUNC, ret, name);
    vector_push_back(&m->ext_syms, &fn);
    fn->mod = m;
    fn->path = path;
    return (KlrValue *)fn;
}

KlrValue *klr_add_ext_global(KlrModule *m, TypeSpec *ts, char *path, char *name)
{
    KlrExtGlobal *var = mm_alloc_obj(var);
    INIT_KLR_VALUE(var, KLR_VALUE_EXT_GLOBAL, ts, name);
    vector_push_back(&m->ext_syms, &var);
    var->mod = m;
    var->path = path;
    return (KlrValue *)var;
}

KlrValue *klr_add_klass(KlrModule *m, TypeSpec *ts, char *name)
{
    KlrKlass *klass = mm_alloc_obj(klass);
    INIT_KLR_VALUE(klass, KLR_VALUE_KLASS, NULL, name);
    vector_init_ptr(&klass->fields);
    vector_init_ptr(&klass->methods);
    vector_push_back(&m->klasses, &klass);
    klass->mod = m;
    klass->ts = ts;
    return (KlrValue *)klass;
}

static KlrField *new_field(TypeSpec *ts, char *name)
{
    KlrField *field = mm_alloc_obj(field);
    INIT_KLR_VALUE(field, KLR_VALUE_FIELD, ts, name);
    return field;
}

KlrValue *klr_klass_add_field(KlrValue *klass_val, char *name, TypeSpec *ts)
{
    KlrKlass *klass = (KlrKlass *)klass_val;
    KlrField *field = new_field(ts, name);
    vector_push_back(&klass->fields, &field);
    return (KlrValue *)field;
}

KlrValue *klr_klass_add_method(KlrValue *klass_val, char *name, TypeSpec *ret,
                               TypeSpec **params)
{
    KlrKlass *klass = (KlrKlass *)klass_val;
    KlrFunc *method = mm_alloc_obj(method);
    INIT_KLR_VALUE(method, KLR_VALUE_FUNC, ret, name);

    init_list(&method->bb_list);
    init_list(&method->edge_list);
    vector_init_ptr(&method->params);
    vector_init_ptr(&method->locals);

    /* initial 'start' and 'end' block */
    method->sbb = new_block(method, "start");
    method->ebb = new_block(method, "end");

    /* add params */
    if (params) {
        TypeSpec **item = params;
        while (*item) {
            KlrParam *val = mm_alloc_obj(val);
            INIT_KLR_VALUE(val, KLR_VALUE_PARAM, *item, "");
            vector_push_back(&method->params, &val);
            ++item;
        }
    }

    vector_push_back(&klass->methods, &method);
    method->mod = klass->mod;
    method->klass = klass;
    return (KlrValue *)method;
}

#ifdef __cplusplus
}
#endif

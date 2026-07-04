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

int check_name(char *name)
{
    if (strchr(name, '<')) return 0;
    return 1;
}

KlrValue *klr_const_int(uint64_t val, TypeSpec *ts, KlrModule *m)
{
    int width = ts->int_flt_info.width;

    KlrConst key = {
        .kind = KLR_VALUE_CONST,
        .which = CONST_INT,
        .len = width,
        .ival = val,
    };

    hashmap_entry_init(&key.hnode, mem_hash(&val, sizeof(val)));
    void *entry = hashmap_get(&m->consts, &key.hnode);
    if (entry) {
        return (KlrValue *)CONTAINER_OF(entry, KlrConst, hnode);
    }

    KlrConst *lit = mm_alloc_obj(lit);
    INIT_KLR_VALUE(lit, KLR_VALUE_CONST, ts, "");
    lit->which = CONST_INT;
    lit->len = width;
    lit->ival = val;
    hashmap_entry_init(&lit->hnode, mem_hash(&val, sizeof(val)));
    hashmap_put(&m->consts, &lit->hnode);
    return (KlrValue *)lit;
}

KlrValue *klr_const_uint(uint64_t val, TypeSpec *ts, KlrModule *m)
{
    int width = ts->int_flt_info.width;

    KlrConst key = {
        .kind = KLR_VALUE_CONST,
        .which = CONST_UINT,
        .len = width,
        .ival = val,
    };

    hashmap_entry_init(&key.hnode, mem_hash(&val, sizeof(val)));
    void *entry = hashmap_get(&m->consts, &key.hnode);
    if (entry) {
        return (KlrValue *)CONTAINER_OF(entry, KlrConst, hnode);
    }

    KlrConst *lit = mm_alloc_obj(lit);
    INIT_KLR_VALUE(lit, KLR_VALUE_CONST, ts, "");
    lit->which = CONST_UINT;
    lit->len = width;
    lit->ival = val;
    hashmap_entry_init(&lit->hnode, mem_hash(&val, sizeof(val)));
    hashmap_put(&m->consts, &lit->hnode);
    return (KlrValue *)lit;
}

KlrValue *klr_const_float(double val, TypeSpec *ts, KlrModule *m)
{
    int width = ts->int_flt_info.width;

    KlrConst key = {
        .kind = KLR_VALUE_CONST,
        .which = CONST_FLT,
        .len = width,
        .fval = val,
    };

    hashmap_entry_init(&key.hnode, mem_hash(&val, sizeof(val)));
    void *entry = hashmap_get(&m->consts, &key.hnode);
    if (entry) {
        return (KlrValue *)CONTAINER_OF(entry, KlrConst, hnode);
    }

    KlrConst *lit = mm_alloc_obj(lit);
    INIT_KLR_VALUE(lit, KLR_VALUE_CONST, ts, "");
    lit->which = CONST_FLT;
    lit->len = width;
    lit->fval = val;
    hashmap_entry_init(&lit->hnode, mem_hash(&val, sizeof(val)));
    hashmap_put(&m->consts, &lit->hnode);
    return (KlrValue *)lit;
}

KlrValue *klr_const_bool(int v, KlrModule *m)
{
    KlrConst key = {
        .kind = KLR_VALUE_CONST,
        .which = CONST_BOOL,
        .bval = v,
    };

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
    KlrConst key = {
        .kind = KLR_VALUE_CONST,
        .which = CONST_STR,
        .len = len,
        .sval = s,
    };
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
    KlrConst key = {
        .kind = KLR_VALUE_CONST,
        .which = CONST_NONE,
        .len = 4,
        .sval = "none",
    };
    hashmap_entry_init(&key.hnode, mem_hash("none", 4));
    void *entry = hashmap_get(&m->consts, &key.hnode);
    if (entry) {
        return (KlrValue *)CONTAINER_OF(entry, KlrConst, hnode);
    }

    KlrConst *lit = mm_alloc_obj(lit);
    TypeSpec *ts = optional_type_spec_intern(NULL);
    INIT_KLR_VALUE(lit, KLR_VALUE_CONST, ts, "");
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
    Vector *list = vector_create_ptr();
    for (int i = 0; i < size; i++) {
        vector_push_back(list, &items[i]);
    }
    lit->list = list;
    return (KlrValue *)lit;
}

KlrValue *klr_const_tuple(KlrValue **items, int size, TypeSpec *ts, KlrModule *m)
{
    KlrValue *lit = klr_const_list(items, size, ts, m);
    ((KlrConst *)lit)->which = CONST_TUPLE;
    return lit;
}

KlrValue *klr_const_range(KlrValue **args, TypeSpec *ts, KlrModule *m)
{
    KlrConst *lit = mm_alloc_obj(lit);
    INIT_KLR_VALUE(lit, KLR_VALUE_CONST, ts, "");
    lit->which = CONST_RANGE;
    lit->len = 3;
    Vector *list = vector_create_ptr();
    vector_push_back(list, &args[0]);
    vector_push_back(list, &args[1]);
    vector_push_back(list, &args[2]);
    lit->list = list;
    return (KlrValue *)lit;
}

int klr_is_immutable(KlrValue *val)
{
    if (klr_is_global(val)) {
        KlrGlobal *global = (KlrGlobal *)val;
        return !global->mutable;
    }

    if (klr_is_param(val)) return 1;

    ASSERT(val->kind == KLR_VALUE_INSN);
    KlrInsn *insn = (KlrInsn *)val;
    return (insn->flags & KLR_INSN_FLAGS_CONST) != 0;
}

typedef struct _LocalVarMapEntry {
    HashMapEntry hnode;
    KlrInsn *local;
    KlrValue *val;
    KlrInsn *move;
} LocalVarMapEntry;

static int __local_var_eq__(void *a, void *b)
{
    LocalVarMapEntry *e1 = (LocalVarMapEntry *)a;
    LocalVarMapEntry *e2 = (LocalVarMapEntry *)b;
    return e1->local == e2->local;
}

int klr_update_local_var(KlrBasicBlock *bb, KlrInsn *local, KlrValue *val, KlrInsn *move)
{
    ASSERT(klr_is_local((KlrValue *)local));
    ASSERT(!(local->flags & KLR_INSN_FLAGS_CONST));

    LocalVarMapEntry key = { .local = local };
    hashmap_entry_init(&key.hnode, mem_hash(&local, sizeof(local)));
    LocalVarMapEntry *entry = hashmap_get(&bb->local_var_map, &key);
    if (entry) {
        if (entry->move && !klr_is_const(entry->val)) {
            entry->move->flags |= KLR_INSN_FLAGS_DEAD;
        }
        entry->val = val;
        entry->move = move;
    } else {
        entry = mm_alloc_obj(entry);
        hashmap_entry_init(&entry->hnode, mem_hash(&local, sizeof(local)));
        entry->local = local;
        entry->val = val;
        entry->move = move;
        hashmap_put(&bb->local_var_map, entry);
    }
    return 0;
}

int klr_clear_local_var(KlrBasicBlock *bb, KlrInsn *local)
{
    ASSERT(klr_is_local((KlrValue *)local));
    ASSERT(!(local->flags & KLR_INSN_FLAGS_CONST));

    LocalVarMapEntry key = { .local = local };
    hashmap_entry_init(&key.hnode, mem_hash(&local, sizeof(local)));
    LocalVarMapEntry *entry = hashmap_get(&bb->local_var_map, &key);
    if (entry) {
        entry->val = NULL;
        entry->move = NULL;
    }
    return 0;
}

KlrValue *klr_get_local_var(KlrBasicBlock *bb, KlrInsn *local)
{
    ASSERT(klr_is_local((KlrValue *)local));

    LocalVarMapEntry key = { .local = local };
    hashmap_entry_init(&key.hnode, mem_hash(&local, sizeof(local)));
    LocalVarMapEntry *entry = hashmap_get(&bb->local_var_map, &key);
    if (entry) {
        ASSERT(!(local->flags & KLR_INSN_FLAGS_CONST));
        ASSERT(!(entry->local->flags & KLR_INSN_FLAGS_CONST));
        KlrValue *val = entry->val;
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
        e->move = NULL;
    }
    return 0;
}

static KlrBasicBlock *new_block(KlrFunc *fn, char *name)
{
    KlrBasicBlock *bb = mm_alloc_obj(bb);
    INIT_KLR_VALUE(bb, KLR_VALUE_BLOCK, NULL, name);

    init_list(&bb->link);
    bb->func = fn;
    init_list(&bb->insn_list);
    init_list(&bb->in_edges);
    init_list(&bb->out_edges);

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

void klr_erase_block(KlrBasicBlock *bb)
{
    log_info("[erase-block] '%%%s' start", klr_block_name(bb));

    list_remove(&bb->link);

    KlrEdge *edge, *nxt;

    edge_out_foreach_safe(edge, nxt, bb) {
        log_info("[erase-block] remove out edge '%%%s' -> '%%%s'", klr_block_name(edge->src),
                 klr_block_name(edge->dst));
        klr_remove_edge(edge);
    }

    edge_in_foreach_safe(edge, nxt, bb) {
        log_info("[erase-block] remove in edge '%%%s' -> '%%%s'", klr_block_name(edge->src),
                 klr_block_name(edge->dst));
        klr_remove_edge(edge);
    }

    KlrInsn *insn, *nxt_i;
    insn_foreach_reverse_safe(insn, nxt_i, bb) {
        log_info("[erase-block] remove insn in block '%%%s'", klr_block_name(bb));
        log_insn(insn);
        klr_erase_insn(insn);
    }

    log_info("[erase-block] '%%%s' done", klr_block_name(bb));

    mm_free(bb);
}

void Klr_merge_block(KlrBasicBlock *dst, KlrBasicBlock *src)
{
    ASSERT(dst->func == src->func);
    KlrFunc *fn = dst->func;

    log_info("[basic-block-merging] merge block '%%%s' into '%%%s'", klr_block_name(src),
             klr_block_name(dst));

    KlrInsn *last = insn_last(dst);
    if (last && last->code == OP_JMP) {
        log_info("[basic-block-merging] remove jmp insn in block '%%%s'", klr_block_name(dst));
        klr_erase_insn(last);
    }

    // Setup an insertion tracker to place src's PHIs right after dst's existing PHIs
    // After this loop, dst_phi_anchor will point to the last PHI instruction in dst (or NULL if
    // none exist).
    KlrInsn *dst_phi_anchor = NULL;
    KlrInsn *curr_dst_insn;
    insn_foreach(curr_dst_insn, dst) {
        if (curr_dst_insn->code != OP_IR_PHI) break;
        dst_phi_anchor = curr_dst_insn;
    }

    /* move all instructions from src to dst */
    KlrInsn *insn, *nxt;
    insn_foreach_safe(insn, nxt, src) {
        log_info("[basic-block-merging] move insn from block '%%%s' to '%%%s'",
                 klr_block_name(src), klr_block_name(dst));
        log_insn(insn);
        list_remove(&insn->bb_link);

        if (insn->code == OP_IR_PHI) {
            /* if 'src' has exactly one unique predecessor 'dst'
             * (num_inedges == 1), no valid SSA construction phase should ever create
             * or leave a PHI node inside 'src' that pulls inputs from 'dst'.
             * If this assertion fires, it instantly flags a critical version-tracking
             * or operand-pruning bug inside the upstream optimization passes.
             */
            for (int i = 0; i < insn->filled; i++) {
                ASSERT(insn->phi_preds[i] != dst);
            }

            if (dst_phi_anchor) {
                list_add(&dst_phi_anchor->bb_link, &insn->bb_link);
            } else {
                list_push_front(&dst->insn_list, &insn->bb_link);
            }
            dst_phi_anchor = insn;
        } else {
            list_push_back(&dst->insn_list, &insn->bb_link);
        }

        insn->bb = dst;
        ++dst->num_insns;
        --src->num_insns;
    }

    /* update out-edges (Remap successor PHI predecessor trackers point-to-point) */
    KlrEdge *edge, *nxt_edge;
    edge_out_foreach_safe(edge, nxt_edge, src) {
        log_info("[basic-block-merging] update out edge '%%%s' -> '%%%s'", klr_block_name(dst),
                 klr_block_name(edge->dst));

        KlrBasicBlock *succ = edge->dst;

        KlrInsn *_insn;
        insn_foreach(_insn, succ) {
            if (_insn->code != OP_IR_PHI) break;
            for (int i = 0; i < _insn->filled; i++) {
                if (_insn->phi_preds[i] == src) {
                    _insn->phi_preds[i] = dst;
                }
            }
        }

        klr_link_edge(dst, succ);
        klr_remove_edge(edge);
    }

    /* update in-edges */
    edge_in_foreach_safe(edge, nxt_edge, src) {
        log_info("[basic-block-merging] remove in edge '%%%s' -> '%%%s'",
                 klr_block_name(edge->src), klr_block_name(edge->dst));
        klr_remove_edge(edge);
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
    KlrEdge *edge = mm_alloc_obj(edge);
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

    if (k1->len != k2->len) {
        return 0;
    }

    switch (k1->which) {
        case CONST_NONE:
            return 1;
        case CONST_INT: // fall-through
        case CONST_UINT:
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
    KlrModule *m = mm_alloc_obj(m);
    m->name = name;
    vector_init_ptr(&m->globals);
    init_list(&m->func_list);
    vector_init_ptr(&m->ext_modules);
    vector_init_ptr(&m->klasses);
    hashmap_init(&m->consts, __const_eq__);
    vector_init_ptr(&m->traits);
    m->init = NULL;
    return m;
}

void klr_destroy_module(KlrModule *m) {}

static KlrFunc *new_func(TypeSpec *ret, char *name)
{
    KlrFunc *fn = mm_alloc_obj(fn);
    INIT_KLR_VALUE(fn, KLR_VALUE_FUNC, ret, name);

    init_list(&fn->bb_list);
    init_list(&fn->edge_list);
    vector_init_ptr(&fn->params);
    init_list(&fn->mlink);

    /* initial 'start' and 'end' block */
    fn->sbb = new_block(fn, "start");
    fn->ebb = new_block(fn, "end");

    return fn;
}

KlrValue *klr_add_func(KlrModule *m, TypeSpec *ret, char *name)
{
    KlrFunc *fn = new_func(ret, name);
    list_push_back(&m->func_list, &fn->mlink);
    fn->module = m;
    return (KlrValue *)fn;
}

KlrValue *klr_add_method(KlrKlass *kls, TypeSpec *ret, char *name)
{
    KlrFunc *fn = new_func(ret, name);
    list_push_back(&kls->func_list, &fn->mlink);
    fn->klass = kls;
    fn->module = kls->module;
    return (KlrValue *)fn;
}

static void klr_fini_func(KlrFunc *fn)
{
    KlrBasicBlock *bb, *nxt;
    basic_block_foreach_safe(bb, nxt, fn) {
        klr_erase_block(bb);
    }
}

void klr_delete_func(KlrModule *m, KlrFunc *fn)
{
    list_remove(&fn->mlink);
    fn->module = NULL;
    klr_fini_func(fn);
    mm_free(fn);
}

int klr_func_empty(KlrFunc *fn)
{
    int total_insns = 0;

    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        total_insns += bb->num_insns;
    }

    return total_insns == 0;
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

KlrValue *klr_add_klass(KlrModule *m, TypeSpec *ts, char *name)
{
    KlrKlass *klass = mm_alloc_obj(klass);
    INIT_KLR_VALUE(klass, KLR_VALUE_KLASS, NULL, name);
    vector_init_ptr(&klass->fields);
    init_list(&klass->func_list);
    vector_push_back(&m->klasses, &klass);
    klass->index = vector_size(&m->klasses) - 1;
    klass->module = m;
    klass->ts = ts;
    ASSERT(check_name(name));
    return (KlrValue *)klass;
}

static KlrField *new_field(TypeSpec *ts, char *name)
{
    KlrField *field = mm_alloc_obj(field);
    INIT_KLR_VALUE(field, KLR_VALUE_FIELD, ts, name);
    return field;
}

KlrValue *klr_add_field(KlrValue *klass_val, char *name, TypeSpec *ts)
{
    KlrKlass *klass = (KlrKlass *)klass_val;
    KlrField *field = new_field(ts, name);
    vector_push_back(&klass->fields, &field);
    field->index = vector_size(&klass->fields) - 1;
    return (KlrValue *)field;
}

KlrValue *klr_add_intf(KlrTrait *trait, TypeSpec *ret, char *name)
{
    KlrIntf *intf = mm_alloc_obj(intf);
    INIT_KLR_VALUE(intf, KLR_VALUE_INTF, ret, name);
    intf->trait = trait;
    intf->module = trait->module;
    intf->intf_index = vector_size(&trait->intfs);
    vector_push_back(&trait->intfs, &intf);
    ASSERT(check_name(name));
    return (KlrValue *)intf;
}

KlrValue *klr_add_trait(KlrModule *m, TypeSpec *ts, char *name)
{
    KlrTrait *trait = mm_alloc_obj(trait);
    INIT_KLR_VALUE(trait, KLR_VALUE_TRAIT, NULL, name);
    vector_push_back(&m->traits, &trait);
    trait->index = vector_size(&m->traits) - 1;
    trait->module = m;
    trait->ts = ts;
    vector_init_ptr(&trait->intfs);
    ASSERT(check_name(name));
    return (KlrValue *)trait;
}

KlrValue *klr_get_trait(KlrModule *m, char *name)
{
    KlrValue *sym;
    vector_foreach(sym, &m->traits) {
        if (str_equal(sym->name, name)) {
            return sym;
        }
    }
    return NULL;
}

static KlrExtModule *_get_ext_mod(KlrModule *m, char *name)
{
    KlrExtModule *sym;
    vector_foreach(sym, &m->ext_modules) {
        if (str_equal(sym->name, name)) {
            return sym;
        }
    }
    return NULL;
}

static KlrValue *_get_ext_func(KlrExtModule *m, KlrExtKlass *kls, char *name)
{
    Vector *list = kls ? &kls->methods : &m->symbols;

    KlrValue *sym;
    vector_foreach(sym, list) {
        if (sym->kind == KLR_VALUE_EXT_FUNC) {
            if (str_equal(sym->name, name)) {
                return sym;
            }
        }
    }
    return NULL;
}

static KlrValue *_get_ext_global(KlrExtModule *m, char *name)
{
    KlrValue *sym;
    vector_foreach(sym, &m->symbols) {
        if (sym->kind == KLR_VALUE_EXT_GLOBAL) {
            if (str_equal(sym->name, name)) {
                return sym;
            }
        }
    }
    return NULL;
}

static KlrValue *_get_ext_klass(KlrExtModule *m, char *name)
{
    KlrValue *sym;
    vector_foreach(sym, &m->symbols) {
        if (sym->kind == KLR_VALUE_EXT_KLASS) {
            if (str_equal(sym->name, name)) {
                return sym;
            }
        }
    }
    return NULL;
}

static KlrValue *_get_ext_field(KlrExtKlass *kls, char *name)
{
    KlrValue *sym;
    vector_foreach(sym, &kls->fields) {
        if (str_equal(sym->name, name)) {
            return sym;
        }
    }
    return NULL;
}

KlrExtModule *klr_add_ext_module(KlrModule *m, char *path)
{
    KlrExtModule *sym = _get_ext_mod(m, path);
    if (sym) return sym;

    KlrExtModule *mod = mm_alloc_obj(mod);
    INIT_KLR_VALUE(mod, KLR_VALUE_EXT_MODULE, NULL, path);
    vector_push_back(&m->ext_modules, &mod);
    vector_init_ptr(&mod->symbols);
    return mod;
}

KlrValue *klr_add_ext_func(KlrModule *m, char *ext_m_path, TypeSpec *ret, char *name)
{
    KlrExtModule *ext_m = klr_add_ext_module(m, ext_m_path);
    KlrValue *sym = _get_ext_func(ext_m, NULL, name);
    if (sym) return sym;

    KlrExtFunc *fn = mm_alloc_obj(fn);
    INIT_KLR_VALUE(fn, KLR_VALUE_EXT_FUNC, ret, name);
    vector_push_back(&ext_m->symbols, &fn);
    fn->module = ext_m;
    fn->klass = NULL;
    return (KlrValue *)fn;
}

KlrValue *klr_add_ext_global(KlrModule *m, char *ext_m_path, TypeSpec *ts, char *name)
{
    KlrExtModule *ext_m = klr_add_ext_module(m, ext_m_path);
    KlrValue *sym = _get_ext_global(ext_m, name);
    if (sym) return sym;

    KlrExtGlobal *var = mm_alloc_obj(var);
    INIT_KLR_VALUE(var, KLR_VALUE_EXT_GLOBAL, ts, name);
    vector_push_back(&ext_m->symbols, &var);
    var->module = ext_m;
    var->klass = NULL;
    return (KlrValue *)var;
}

KlrValue *klr_add_ext_klass(KlrModule *m, char *ext_m_path, TypeSpec *ts, char *name)
{
    KlrExtModule *ext_m = klr_add_ext_module(m, ext_m_path);
    KlrValue *sym = _get_ext_klass(ext_m, name);
    if (sym) return sym;

    KlrExtKlass *klass = mm_alloc_obj(klass);
    INIT_KLR_VALUE(klass, KLR_VALUE_EXT_KLASS, ts, name);
    vector_push_back(&ext_m->symbols, &klass);
    klass->module = ext_m;
    vector_init_ptr(&klass->fields);
    vector_init_ptr(&klass->methods);
    ASSERT(check_name(name));
    return (KlrValue *)klass;
}

KlrValue *klr_add_ext_field(KlrExtKlass *kls, TypeSpec *ts, char *name)
{
    KlrValue *sym = _get_ext_field(kls, name);
    if (sym) return sym;

    KlrExtField *field = mm_alloc_obj(field);
    INIT_KLR_VALUE(field, KLR_VALUE_EXT_FIELD, ts, name);
    vector_push_back(&kls->fields, &field);
    field->module = kls->module;
    field->klass = kls;
    return (KlrValue *)field;
}

KlrValue *klr_add_ext_method(KlrExtKlass *kls, TypeSpec *ret, char *name)
{
    KlrValue *sym = _get_ext_func(NULL, kls, name);
    if (sym) return sym;

    KlrExtFunc *fn = mm_alloc_obj(fn);
    INIT_KLR_VALUE(fn, KLR_VALUE_EXT_FUNC, ret, name);
    vector_push_back(&kls->methods, &fn);
    fn->module = kls->module;
    fn->klass = kls;
    return (KlrValue *)fn;
}

static KlrValue *_get_ext_trait(KlrExtModule *m, char *name)
{
    KlrValue *sym;
    vector_foreach(sym, &m->symbols) {
        if (sym->kind == KLR_VALUE_EXT_TRAIT) {
            if (str_equal(sym->name, name)) {
                return sym;
            }
        }
    }
    return NULL;
}

KlrValue *klr_get_ext_intf(KlrExtTrait *trait, char *name)
{
    Vector *list = &trait->intfs;

    KlrValue *sym;
    vector_foreach(sym, list) {
        if (sym->kind == KLR_VALUE_EXT_INTF) {
            if (str_equal(sym->name, name)) {
                return sym;
            }
        }
    }
    return NULL;
}

KlrValue *klr_add_ext_trait(KlrModule *m, char *ext_m_path, TypeSpec *ts, char *name)
{
    KlrExtModule *ext_m = klr_add_ext_module(m, ext_m_path);
    KlrValue *sym = _get_ext_trait(ext_m, name);
    if (sym) return sym;

    KlrExtTrait *trait = mm_alloc_obj(trait);
    INIT_KLR_VALUE(trait, KLR_VALUE_EXT_TRAIT, ts, name);
    vector_push_back(&ext_m->symbols, &trait);
    trait->module = ext_m;
    vector_init_ptr(&trait->intfs);
    return (KlrValue *)trait;
}

KlrValue *klr_add_ext_intf(KlrExtTrait *trait, TypeSpec *ret, char *name)
{
    KlrValue *sym = klr_get_ext_intf(trait, name);
    if (sym) return sym;

    KlrExtIntf *intf = mm_alloc_obj(intf);
    INIT_KLR_VALUE(intf, KLR_VALUE_EXT_INTF, ret, name);
    intf->intf_index = vector_size(&trait->intfs);
    vector_push_back(&trait->intfs, &intf);
    intf->module = trait->module;
    intf->trait = trait;
    return (KlrValue *)intf;
}

KlrValue *klr_new_index(KlrBuilder *bldr, KlrValue *obj, KlrValue *index, int which)
{
    KlrIndexInfo *index_info = mm_alloc_obj(index_info);
    INIT_KLR_VALUE(index_info, KLR_VALUE_INDEX, NULL, "");
    index_info->obj = obj;
    index_info->index = index;
    index_info->which = which;
    return (KlrValue *)index_info;
}

static void dfs_post_order(KlrBasicBlock *bb, List *rpo_list)
{
    // don't visit the end block
    if (bb == bb->func->ebb) return;
    if (bb->visited) return;

    bb->visited = 1;

    /* handle 'if-else' firstly */
    KlrEdge *edge;
    edge_out_foreach_reverse(edge, bb) {
        dfs_post_order(edge->dst, rpo_list);
    }

    // remove from fn->bb_list and push to rpo_list
    list_remove(&bb->link);
    list_push_front(rpo_list, &bb->link);
}

void klr_build_rpo(KlrFunc *fn)
{
    List rpo_list;
    init_list(&rpo_list);

    // clear visited flag
    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        bb->visited = 0;
    }

    // from start basic block, do a post-order DFS traversal and push blocks to
    // rpo_list
    ASSERT(fn->sbb->num_outedges == 1);
    KlrEdge *edge = edge_out_first(fn->sbb);
    dfs_post_order(edge->dst, &rpo_list);

    // move blocks from rpo_list back to fn->bb_list
    list_move(&fn->bb_list, &rpo_list);
    ASSERT(list_empty(&rpo_list));

    int rpo_index = 0;
    basic_block_foreach(bb, fn) {
        // used for back-edge recognition
        bb->index = rpo_index++;
    }

#ifndef NOLOG
    log_info("RPO order for func '%s':", fn->name);
    klr_print_func(fn, stdout);
#endif
}

char *klr_block_name(KlrBasicBlock *bb)
{
    if (bb->name[0]) {
        snprintf(bb->print_name, sizeof(bb->print_name), "bb%d(%s)", bb->tag, bb->name);
    } else {
        snprintf(bb->print_name, sizeof(bb->print_name), "bb%d", bb->tag);
    }
    return bb->print_name;
}

char *klr_value_name(KlrValue *val)
{
    if (val->kind == KLR_VALUE_NONE) {
        strcpy(val->print_name, "undef");
        return val->print_name;
    }

    if (val->name[0]) {
        if (val->kind == KLR_VALUE_GLOBAL) {
            snprintf(val->print_name, sizeof(val->print_name), "@%s", val->name);
        } else {
            snprintf(val->print_name, sizeof(val->print_name), "%%%s", val->name);
        }
    } else {
        if (val->tag == -1) {
            snprintf(val->print_name, sizeof(val->print_name), "%%<unnamed>");
        } else {
            snprintf(val->print_name, sizeof(val->print_name), "%%%d", val->tag);
        }
    }

    return val->print_name;
}

void klr_set_loc(KlrValue *val, char *filename, Loc loc)
{
    val->loc.filename = filename;
    val->loc.loc = loc;
}

#ifdef __cplusplus
}
#endif

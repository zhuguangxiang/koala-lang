/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * Inline the direct calls to the small functions of the current module, e.g. the getters/setters
 * of a class. It runs after irgen, when all generic functions are specialized.
 *
 * A callee is inlined if it:
 *   - is a function/method/static method of the current module (not an external one or an intf),
 *   - has only one basic block, ending with 'ret',
 *   - has no more than INLINE_MAX_INSNS instructions,
 *   - is not a generic (erased) function and does not call a generic function with type args,
 *     except a getter/setter like method of a generic class, whose 'T' is replaced by the type args
 *     of the receiver, e.g. 'Box[int].get()',
 *   - is not (mutually) recursive with the caller.
 *
 * The callees are processed before the callers (bottom-up), so a callee is already inlined into.
 */

#include "atom.h"
#include "cmd.h"
#include "domtree.h"
#include "inline_cost.h"
#include "mm.h"
#include "opt.h"
#include "symbol.h"

#ifdef __cplusplus
extern "C" {
#endif

#define INLINE_MAX_INSNS 8

typedef enum {
    INLINE_UNVISITED = 0,
    INLINE_VISITING,
    INLINE_DONE,
} InlineState;

typedef struct _PtrEntry {
    HashMapEntry hnode;
    void *key;
    void *val;
} PtrEntry;

static int _ptr_equal_(void *a, void *b) { return ((PtrEntry *)a)->key == ((PtrEntry *)b)->key; }

static void ptr_map_put(HashMap *map, void *key, void *val)
{
    PtrEntry *entry = mm_alloc_obj(entry);
    hashmap_entry_init(entry, mem_hash(&key, sizeof(void *)));
    entry->key = key;
    entry->val = val;
    PtrEntry *old = hashmap_put(map, entry);
    if (old) mm_free(old);
}

static void *ptr_map_get(HashMap *map, void *key)
{
    PtrEntry k = { .key = key };
    hashmap_entry_init(&k, mem_hash(&key, sizeof(void *)));
    PtrEntry *entry = hashmap_get(map, &k);
    return entry ? entry->val : NULL;
}

static void _free_entry_(void *e, void *arg) { mm_free(e); }

static void ptr_map_fini(HashMap *map) { hashmap_fini(map, _free_entry_, NULL); }

/* a type with type variables, e.g. 'T', 'list[T]', 'T?' */
static int type_is_open(TypeSpec *ts)
{
    if (!ts) return 0;
    Vector *args = NULL;
    switch (ts->kind) {
        case TYPE_GENERIC_VAR:
        case TYPE_GENERIC_REF:
            return 1;
        case TYPE_OPTIONAL:
            return type_is_open(ts->opt.src);
        case TYPE_VA_LIST:
            return type_is_open(ts->va_list.src);
        case TYPE_UNION:
            args = ts->union_type.args;
            break;
        case TYPE_PROTO:
            if (type_is_open(ts->proto_type.ret)) return 1;
            args = ts->proto_type.args;
            break;
        default:
            return 0;
    }

    TypeSpec *arg;
    vector_foreach(arg, args) {
        if (type_is_open(arg)) return 1;
    }
    return 0;
}

static int func_num_blocks(KlrFunc *fn)
{
    int n = 0;
    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        ++n;
    }
    return n;
}

static KlrBasicBlock *func_first_block(KlrFunc *fn)
{
    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        return bb;
    }
    return NULL;
}

static KlrInsn *block_last_insn(KlrBasicBlock *bb)
{
    KlrInsn *last = NULL;
    KlrInsn *insn;
    insn_foreach(insn, bb) {
        last = insn;
    }
    return last;
}

/* a value an instruction can take as an operand directly */
static int is_reg_value(KlrValue *val)
{
    return val->kind == KLR_VALUE_INSN || val->kind == KLR_VALUE_PARAM ||
           val->kind == KLR_VALUE_CONST;
}

/* a value 'move' can take, e.g. a function value */
static int is_movable_value(KlrValue *val)
{
    return is_reg_value(val) || val->kind == KLR_VALUE_FUNC || val->kind == KLR_VALUE_EXT_FUNC;
}

/* the binding of the type params of a generic class to the type args of a method's receiver */
typedef struct _TypeSubst {
    /* 'Box[Box:T]' */
    TypeSpec *self_ts;
    /* 'Box[int64]' */
    TypeSpec *recv_ts;
    InstanceSymbol *inst;
} TypeSubst;

/* the receiver binding of a call to a method of a generic class, NULL if none */
static int init_type_subst(TypeSubst *sub, KlrInsn *call, KlrFunc *callee)
{
    if (!callee->klass || !callee->self) return 0;
    if (vector_size(&callee->params) < 1) return 0;
    KlrValue *self = vector_get(&callee->params, 0);
    if (self != callee->self || !type_is_open(self->ts)) return 0;

    TypeSpec *recv_ts = insn_oper_value(call, 1)->ts;
    if (!recv_ts || recv_ts->sym_id < 0) return 0;
    Symbol *sym = get_symbol_by_id(recv_ts->sym_id);
    if (!sym || sym->kind != SYM_INSTANCE) return 0;
    InstanceSymbol *inst = (InstanceSymbol *)sym;

    TypeSpec *arg;
    vector_foreach(arg, inst->tp_args) {
        if (!arg || type_is_open(arg)) return 0;
    }

    sub->self_ts = self->ts;
    sub->recv_ts = recv_ts;
    sub->inst = inst;
    return 1;
}

/* 'ts' with 'T', 'T?' and the self type replaced by the receiver's ones; NULL if not supported,
   e.g. 'list[T]' */
static TypeSpec *subst_type(TypeSpec *ts, TypeSubst *sub)
{
    if (!ts || !type_is_open(ts)) return ts;
    if (!sub) return NULL;
    if (ts == sub->self_ts) return sub->recv_ts;

    if (ts->kind == TYPE_GENERIC_VAR) {
        TypeParamSymbol *tp = get_symbol_by_id(ts->sym_id);
        if (!tp || tp->kind != SYM_TYPE_PARAM || tp->owner != sub->inst->origin) return NULL;
        int index = ts->generic_var.index;
        if (index < 0 || index >= vector_size(sub->inst->tp_args)) return NULL;
        return vector_get(sub->inst->tp_args, index);
    }

    if (ts->kind == TYPE_OPTIONAL) {
        TypeSpec *src = subst_type(ts->opt.src, sub);
        return src ? optional_type_spec_intern(src) : NULL;
    }

    return NULL;
}

/* the ops of a generic class method body, which need no type-dependent lowering of 'T' */
static int is_generic_safe_op(KlrInsn *insn)
{
    switch (insn->code) {
        case OP_GET_FIELD:
        case OP_SET_FIELD:
        case OP_IR_LOCAL:
        case OP_MOVE:
        case OP_RET:
        case OP_RET_VOID:
            return 1;
        default:
            return 0;
    }
}

static int can_inline(KlrFunc *callee, TypeSubst *sub)
{
    if (callee->no_inline) return 0;
    if (func_num_blocks(callee) != 1) return 0;
    if (!subst_type(callee->ts, sub)) return 0;

    KlrParam *param;
    vector_foreach(param, &callee->params) {
        if (!subst_type(param->ts, sub)) return 0;
        if (param->ts && param->ts->kind == TYPE_VA_LIST) return 0;
        // a param is written(only by tail call), cannot be replaced by the argument
        if (param->def_count > 0) return 0;
    }

    KlrBasicBlock *bb = func_first_block(callee);
    if (bb->num_insns > INLINE_MAX_INSNS) return 0;

    KlrInsn *last = block_last_insn(bb);
    if (!last || !insn_or(last, OP_RET, OP_RET_VOID)) return 0;
    // 'ret void' of a function with a result, which never returns(e.g. ends with a panic)
    if (insn_is(last, OP_RET_VOID) && !type_is_no_type(callee->ts)) return 0;
    if (insn_is(last, OP_RET) && !is_movable_value(insn_oper_value(last, 0))) return 0;

    KlrInsn *insn;
    insn_foreach(insn, bb) {
        if (insn_is(insn, OP_UNREACHABLE) || insn_is(insn, OP_IR_PHI)) return 0;
        if (insn->tp_args) return 0;
        if (insn != last && insn_is_terminator(insn)) return 0;
        if (insn->ts && !subst_type(insn->ts, sub)) return 0;
        if (insn->target_ts && !subst_type(insn->target_ts, sub)) return 0;
        if (sub && !is_generic_safe_op(insn)) return 0;
    }

    return 1;
}

/* the name of a named value(e.g. a local) of the callee in the caller */
static char *inlined_name(KlrFunc *callee, char *name)
{
    if (!name || !name[0]) return name;
    return atom_concat(3, callee->name, ".", name);
}

/* 'move' val into a new local of type ts, as irgen does for a value converted implicitly, e.g. 'T'
   to 'T?', or a function value */
static KlrValue *build_local_copy(KlrBuilder *bldr, KlrValue *val, TypeSpec *ts)
{
    KlrValue *tmp = klr_build_local_var(bldr, ts, "");
    // a copy is never written again: an immutable function value can be called directly
    if (klr_is_func(val)) ((KlrInsn *)tmp)->flags |= KLR_INSN_FLAGS_CONST;
    klr_build_move(bldr, tmp, val);
    return tmp;
}

/* the name of a function at runtime, e.g. 'Box$get' */
static char *runtime_func_name(KlrFunc *fn)
{
    if (fn->klass) return atom_concat(3, fn->klass->name, "$", fn->name);
    return atom(fn->name);
}

/* the site of an instruction of the callee, inlined at 'site': a site of the callee (inlined
   before) is copied, with its outermost parent set to 'site' */
static KlrInlineSite *rebase_site(KlrInlineSite *s, KlrInlineSite *site, HashMap *site_map)
{
    if (!s) return site;
    KlrInlineSite *copy = ptr_map_get(site_map, s);
    if (copy) return copy;
    copy = mm_alloc_obj(copy);
    *copy = *s;
    copy->parent = rebase_site(s->parent, site, site_map);
    ptr_map_put(site_map, s, copy);
    return copy;
}

/*
 * Replace 'call' with the body of 'callee':
 *   %r = call @callee, %a, %b       =>   (cloned body with params replaced by %a, %b)
 *   use %r                                use %v      (%v is the value of 'ret %v')
 */
static void inline_call(KlrInsn *call, KlrFunc *callee, TypeSubst *sub)
{
    HashMap val_map;
    hashmap_init(&val_map, _ptr_equal_);
    HashMap site_map;
    hashmap_init(&site_map, _ptr_equal_);

    KlrInlineSite *site = mm_alloc_obj(site);
    site->func_name = runtime_func_name(callee);
    site->filename = call->loc.filename;
    site->loc = call->loc.loc;
    site->parent = call->loc.inlined_at;

    KlrBuilder bldr;
    klr_builder_before(&bldr, call);

    KlrParam *param;
    vector_foreach(param, &callee->params) {
        KlrValue *arg = insn_oper_value(call, i__ + 1);
        TypeSpec *param_ts = subst_type(param->ts, sub);
        if (!is_reg_value(arg) || arg->ts != param_ts) {
            arg = build_local_copy(&bldr, arg, param_ts);
        }
        ptr_map_put(&val_map, param, arg);
    }

    KlrBasicBlock *bb = func_first_block(callee);
    KlrInsn *ret = block_last_insn(bb);

    // clone all instructions before setting the operands, so an operand is always mapped
    KlrInsn *insn;
    insn_foreach(insn, bb) {
        if (insn == ret) break;
        KlrInsn *clone = klr_build_clone(&bldr, insn, subst_type(insn->ts, sub));
        clone->target_ts = subst_type(insn->target_ts, sub);
        clone->name = inlined_name(callee, insn->name);
        // keep the callee's location and the inlined site, so the traceback shows the frames of
        // the inlined functions
        if (insn->loc.filename) {
            clone->loc = insn->loc;
            clone->loc.inlined_at = rebase_site(insn->loc.inlined_at, site, &site_map);
        } else {
            clone->loc = call->loc;
        }
        clone->flags |= KLR_INSN_FLAGS_INLINED;
        ptr_map_put(&val_map, insn, clone);
    }

    insn_foreach(insn, bb) {
        if (insn == ret) break;
        KlrInsn *clone = ptr_map_get(&val_map, insn);
        for (int i = 0; i < insn->num_opers; i++) {
            KlrValue *val = insn_oper_value(insn, i);
            KlrValue *mapped = val ? ptr_map_get(&val_map, val) : NULL;
            klr_init_clone_oper(clone, insn, i, mapped ? mapped : val);
        }
    }

    if (insn_is(ret, OP_RET)) {
        KlrValue *val = insn_oper_value(ret, 0);
        KlrValue *mapped = ptr_map_get(&val_map, val);
        if (mapped) val = mapped;

        // the result of the call is a snapshot of a local var, which may be written later
        if (klr_is_local(val) || !is_reg_value(val) || val->ts != call->ts) {
            val = build_local_copy(&bldr, val, call->ts);
        }
        replace_all_uses_with(val, (KlrValue *)call);
    }

    ASSERT(call->use_count == 0);
    klr_erase_insn(call);

    ptr_map_fini(&val_map);
    ptr_map_fini(&site_map);
}

/* a local function value whose function is known: an immutable local only defined by
   'move %v, @func', e.g. 'let f = { |x int| x * 2 }' */
static KlrValue *resolve_func_value(KlrValue *val)
{
    if (!val || !klr_is_local(val)) return val;
    if (!klr_is_immutable(val) || val->def_count != 1) return val;

    KlrUse *def = list_first(&val->def_list, KlrUse, use_link);
    if (!def || !insn_is(def->insn, OP_MOVE)) return val;

    KlrValue *src = insn_oper_value(def->insn, 1);
    return (src && klr_is_func(src)) ? src : val;
}

static KlrFunc *direct_callee(KlrInsn *insn, KlrModule *m)
{
    if (!insn_is(insn, OP_IR_CALL)) return NULL;
    if (insn->tp_args) return NULL;
    KlrValue *val = resolve_func_value(insn_oper_value(insn, 0));
    if (!val || !klr_is_func(val)) return NULL;
    KlrFunc *fn = (KlrFunc *)val;
    if (fn->module != m) return NULL;
    if (insn->num_opers - 1 != vector_size(&fn->params)) return NULL;
    for (int i = 1; i < insn->num_opers; i++) {
        KlrValue *arg = insn_oper_value(insn, i);
        if (!is_movable_value(arg)) return NULL;
    }
    return fn;
}

static int inline_func(KlrFunc *fn, KlrModule *m, HashMap *states)
{
    ptr_map_put(states, fn, (void *)INLINE_VISITING);

    int count = 0;
    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        KlrInsn *insn, *next;
        insn_foreach_safe(insn, next, bb) {
            KlrFunc *callee = direct_callee(insn, m);
            if (!callee || callee == fn) continue;

            InlineState state = (InlineState)(intptr_t)ptr_map_get(states, callee);
            if (state == INLINE_UNVISITED) {
                count += inline_func(callee, m, states);
                state = INLINE_DONE;
            }
            // a (mutual) recursive call
            if (state != INLINE_DONE) continue;
            TypeSubst subst;
            TypeSubst *sub = init_type_subst(&subst, insn, callee) ? &subst : NULL;
            if (!can_inline(callee, sub)) continue;

            KlrInsn *prev = insn_prev(insn, bb);
            inline_call(insn, callee, sub);
            ++count;
            // revisit the inlined body: a call of a function value argument is direct now, e.g.
            // 'fn(x)' of 'apply(fn, x)' called with a lambda
            next = prev ? insn_next(prev, bb) : insn_first(bb);
        }
    }

    ptr_map_put(states, fn, (void *)INLINE_DONE);
    return count;
}

static void dump_funcs(KlrModule *m)
{
    KlrFunc *fn;
    func_foreach(fn, m) {
        if (!fn) continue;
        fprintf(stdout, "--- IR Dump After inline ---\n");
        klr_print_func(fn, stdout);
    }

    KlrKlass *kls;
    vector_foreach(kls, &m->klasses) {
        func_foreach(fn, kls) {
            fprintf(stdout, "--- IR Dump After inline ---\n");
            klr_print_func(fn, stdout);
        }
    }
}

void kl_inline(KlrModule *m)
{
    if (!m || m->errors > 0) return;

    HashMap states;
    hashmap_init(&states, _ptr_equal_);

    KlrFunc *fn;
    func_foreach(fn, m) {
        if (!fn) continue;
        if (ptr_map_get(&states, fn)) continue;
        inline_func(fn, m, &states);
    }

    KlrKlass *kls;
    vector_foreach(kls, &m->klasses) {
        func_foreach(fn, kls) {
            if (ptr_map_get(&states, fn)) continue;
            inline_func(fn, m, &states);
        }
    }

    ptr_map_fini(&states);

    if (dump_inline_ir_enabled()) dump_funcs(m);
}

#define SELF_INLINE_MAX_BLOCKS 16
#define SELF_INLINE_MAX_INSNS  32

typedef struct _SelfInlineBody {
    KlrBasicBlock *blocks[SELF_INLINE_MAX_BLOCKS];
    KlrInsn *insns[SELF_INLINE_MAX_BLOCKS][SELF_INLINE_MAX_INSNS];
    int sizes[SELF_INLINE_MAX_BLOCKS];
    KlrInsn *calls[RECURSIVE_INLINE_MAX_CALLS];
    int nblocks;
    int ncalls;
    int ninsns;
    int nreturns;
} SelfInlineBody;

typedef struct _SelfInlineCopy {
    KlrBasicBlock *continuation;
    KlrBasicBlock *entry;
    KlrValue *result;
} SelfInlineCopy;

typedef struct _SelfInlineWork {
    KlrInsn *call;
    SelfInlineCopy copy;
    int probability;
} SelfInlineWork;

static int self_inline_body(KlrFunc *fn, SelfInlineBody *body)
{
    memset(body, 0, sizeof(*body));
    if (fn->no_inline || fn->has_tailcall || type_is_no_type(fn->ts) || !subst_type(fn->ts, NULL))
        return 0;
    KlrParam *param;
    vector_foreach(param, &fn->params) {
        if (param->def_count || !subst_type(param->ts, NULL) ||
            (param->ts && param->ts->kind == TYPE_VA_LIST))
            return 0;
    }

    int total = 0;
    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        if (body->nblocks == SELF_INLINE_MAX_BLOCKS) return 0;
        int b = body->nblocks++;
        body->blocks[b] = bb;
        KlrInsn *insn;
        insn_foreach(insn, bb) {
            KlrInsn *next = insn_next(insn, bb);
            if (tail_call_enabled() && insn_is(insn, OP_IR_CALL) && next && insn_is(next, OP_RET) &&
                insn_oper_value(next, 0) == (KlrValue *)insn)
                return 0;
            if (++total > SELF_INLINE_MAX_INSNS || insn_is(insn, OP_IR_PHI) ||
                insn_is(insn, OP_UNREACHABLE) || insn_is(insn, OP_RET_VOID) ||
                (insn_is(insn, OP_RET) && !is_movable_value(insn_oper_value(insn, 0))) ||
                (insn != insn_last(bb) && insn_is_terminator(insn)) || insn->tp_args ||
                (insn->ts && !subst_type(insn->ts, NULL)) ||
                (insn->target_ts && !subst_type(insn->target_ts, NULL)))
                return 0;
            body->insns[b][body->sizes[b]++] = insn;
            if (insn_is(insn, OP_RET)) ++body->nreturns;
            if (direct_callee(insn, fn->module) == fn) {
                if (body->ncalls == RECURSIVE_INLINE_MAX_CALLS) return 0;
                body->calls[body->ncalls++] = insn;
            }
        }
        KlrInsn *last = insn_last(bb);
        if (!last || (!insn_or(last, OP_RET, OP_JMP) && !insn_is(last, OP_IR_JMP_COND))) return 0;
    }
    body->ninsns = total;
    return body->nblocks && body->ncalls;
}

static void self_inline_prepare(KlrFunc *fn, SelfInlineBody *body, KlrInsn *call,
                                SelfInlineCopy *copy, KlrInsn **nested)
{
    HashMap map, site_map;
    hashmap_init(&map, _ptr_equal_);
    hashmap_init(&site_map, _ptr_equal_);

    KlrInlineSite *site = mm_alloc_obj(site);
    site->func_name = runtime_func_name(fn);
    site->filename = call->loc.filename;
    site->loc = call->loc.loc;
    site->parent = call->loc.inlined_at;

    KlrBuilder before;
    klr_builder_before(&before, call);
    KlrParam *param;
    vector_foreach(param, &fn->params) {
        KlrValue *arg = insn_oper_value(call, i__ + 1);
        KlrValue *local = build_local_copy(&before, arg, param->ts);
        ptr_map_put(&map, param, local);
    }
    copy->result = klr_build_local_var(&before, call->ts, "");
    copy->continuation = klr_add_block(call->bb, "self-inline-cont");

    for (int b = 0; b < body->nblocks; b++) {
        KlrBasicBlock *cloned = klr_append_block((KlrValue *)fn, body->blocks[b]->name);
        ptr_map_put(&map, body->blocks[b], cloned);
        if (b == 0) copy->entry = cloned;
    }

    for (int b = 0; b < body->nblocks; b++) {
        KlrBuilder builder;
        KlrBasicBlock *cloned = ptr_map_get(&map, body->blocks[b]);
        klr_builder_end(&builder, cloned);
        for (int i = 0; i < body->sizes[b]; i++) {
            KlrInsn *insn = body->insns[b][i];
            if (insn_is(insn, OP_RET)) continue;
            KlrInsn *clone = klr_build_clone(&builder, insn, insn->ts);
            if (insn->loc.filename) {
                clone->loc = insn->loc;
                clone->loc.inlined_at = rebase_site(insn->loc.inlined_at, site, &site_map);
            } else {
                clone->loc = call->loc;
            }
            clone->flags |= KLR_INSN_FLAGS_INLINED;
            ptr_map_put(&map, insn, clone);
        }
    }

    for (int b = 0; b < body->nblocks; b++) {
        KlrBasicBlock *cloned = ptr_map_get(&map, body->blocks[b]);
        for (int i = 0; i < body->sizes[b]; i++) {
            KlrInsn *insn = body->insns[b][i];
            if (insn_is(insn, OP_RET)) {
                KlrValue *val = insn_oper_value(insn, 0);
                KlrValue *mapped = ptr_map_get(&map, val);
                KlrBuilder builder;
                klr_builder_end(&builder, cloned);
                klr_build_move(&builder, copy->result, mapped ? mapped : val);
                klr_build_jmp(&builder, copy->continuation);
                continue;
            }
            KlrInsn *clone = ptr_map_get(&map, insn);
            for (int j = 0; j < insn->num_opers; j++) {
                KlrValue *val = insn_oper_value(insn, j);
                KlrValue *mapped = val ? ptr_map_get(&map, val) : NULL;
                klr_init_clone_oper(clone, insn, j, mapped ? mapped : val);
            }
        }
        KlrEdge *edge;
        edge_out_foreach(edge, body->blocks[b]) {
            if (edge->dst == fn->ebb) continue;
            KlrBasicBlock *dst = ptr_map_get(&map, edge->dst);
            ASSERT(dst);
            klr_link_edge(cloned, dst);
        }
    }
    for (int i = 0; i < body->ncalls; i++) nested[i] = ptr_map_get(&map, body->calls[i]);
    ptr_map_fini(&map);
    ptr_map_fini(&site_map);
}

static void self_inline_finish(KlrInsn *call, SelfInlineCopy *copy)
{
    KlrBasicBlock *bb = call->bb;
    KlrInsn *insn = insn_next(call, bb);
    while (insn) {
        KlrInsn *next = insn_next(insn, bb);
        list_remove(&insn->bb_link);
        list_push_back(&copy->continuation->insn_list, &insn->bb_link);
        insn->bb = copy->continuation;
        --bb->num_insns;
        ++copy->continuation->num_insns;
        insn = next;
    }
    KlrEdge *edge, *next_edge;
    edge_out_foreach_safe(edge, next_edge, bb) {
        klr_link_edge(copy->continuation, edge->dst);
        klr_remove_edge(edge);
    }
    replace_all_uses_with(copy->result, (KlrValue *)call);
    klr_erase_insn(call);
    KlrBuilder builder;
    klr_builder_end(&builder, bb);
    klr_build_jmp(&builder, copy->entry);
}

static int self_inline_func(KlrFunc *fn)
{
    SelfInlineBody body;
    if (!self_inline_body(fn, &body)) return 0;

    SelfInlineWork work[RECURSIVE_INLINE_MAX_SITES];
    int probabilities[RECURSIVE_INLINE_MAX_CALLS];
    recursive_inline_call_probabilities(fn, body.calls, body.ncalls, probabilities);
    int queued = 0, prepared = 0;
    for (int i = 0; i < body.ncalls; i++) {
        if (recursive_inline_probability_allowed(probabilities[i]))
            work[queued++] =
                (SelfInlineWork){ .call = body.calls[i], .probability = probabilities[i] };
    }
    RecursiveInlineCost cost;
    recursive_inline_cost_init(&cost, body.ninsns, body.nreturns, vector_size(&fn->params),
                               body.nblocks);
    int level_end = queued;

    // Prepare all copies from the unchanged body before splitting any call blocks.
    for (int depth = 1; queued > prepared && depth <= RECURSIVE_INLINE_MAX_DEPTH; depth++) {
        int level_count = level_end - prepared;
        if (!recursive_inline_cost_allow_level(&cost, depth, level_count)) break;
        while (prepared < level_end) {
            SelfInlineWork *item = &work[prepared++];
            KlrInsn *nested[RECURSIVE_INLINE_MAX_CALLS];
            self_inline_prepare(fn, &body, item->call, &item->copy, nested);
            if (depth < RECURSIVE_INLINE_MAX_DEPTH) {
                for (int i = 0; i < body.ncalls; i++) {
                    int probability =
                        recursive_inline_probability_product(item->probability, probabilities[i]);
                    if (!recursive_inline_probability_allowed(probability)) continue;
                    ASSERT(queued < RECURSIVE_INLINE_MAX_SITES);
                    work[queued++] =
                        (SelfInlineWork){ .call = nested[i], .probability = probability };
                }
            }
        }
        level_end = queued;
    }
    for (int i = 0; i < prepared; i++) self_inline_finish(work[i].call, &work[i].copy);
    if (!prepared) return 0;
    klr_func_free_domtree(fn);
    if (dump_recursive_inline_ir_enabled()) {
        fprintf(stdout, "--- IR Dump After recursive-self-inline ---\n");
        klr_print_func(fn, stdout);
    }
    return prepared;
}

int kl_recursive_self_inline(KlrModule *m)
{
    if (!m || m->errors > 0) return 0;
    int count = 0;
    KlrFunc *fn;
    func_foreach(fn, m) {
        if (fn) count += self_inline_func(fn);
    }
    KlrKlass *kls;
    vector_foreach(kls, &m->klasses) {
        func_foreach(fn, kls) {
            if (fn) count += self_inline_func(fn);
        }
    }
    return count;
}

#ifdef __cplusplus
}
#endif

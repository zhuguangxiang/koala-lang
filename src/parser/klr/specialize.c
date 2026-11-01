/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/* Generic function specialization: clone a generic function's IR with its
   type parameters replaced by concrete type arguments. */

#include "ir.h"
#include "mm.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _KlrMapMap {
    HashMapEntry hnode;
    void *old_ptr;
    void *new_ptr;
} KlrMapEntry;

static int _ptr_equal_(void *a, void *b)
{
    KlrMapEntry *e1 = a;
    KlrMapEntry *e2 = b;
    return e1->old_ptr == e2->old_ptr;
}

static void add_to_map(HashMap *map, void *old_ptr, void *new_ptr)
{
    KlrMapEntry *entry = mm_alloc_obj(entry);
    hashmap_entry_init(entry, mem_hash(&old_ptr, sizeof(void *)));
    entry->old_ptr = old_ptr;
    entry->new_ptr = new_ptr;
    void *ret = hashmap_put(map, entry);
    ASSERT(ret == NULL);
}

static void *get_from_map(HashMap *map, void *old_ptr)
{
    KlrMapEntry key = { .old_ptr = old_ptr };
    hashmap_entry_init(&key, mem_hash(&old_ptr, sizeof(void *)));
    KlrMapEntry *entry = hashmap_get(map, &key);
    return entry ? entry->new_ptr : NULL;
}

/* map an operand of the generic function to the specialized one */
static KlrValue *map_value(HashMap *map, KlrValue *val)
{
    if (!val) return NULL;
    KlrValue *new_val = get_from_map(map, val);
    if (new_val) return new_val;
    // function-local values are all mapped, the others (const, global, func, klass, field, ext
    // symbols, ...) are shared with the generic function
    ASSERT(val->kind != KLR_VALUE_INSN && val->kind != KLR_VALUE_PARAM &&
           val->kind != KLR_VALUE_BLOCK);
    return val;
}

/* specialize the type args (with type variables) of a call in the generic function */
static Vector *specialize_tp_args(Vector *call_tp_args, Vector *tp_args, KlrSpecializeHooks *hooks)
{
    Vector *args = vector_create_ptr();
    TypeSpec *ts;
    vector_foreach(ts, call_tp_args) {
        TypeSpec *arg = hooks->specialize_ts(ts, tp_args, hooks->ctx);
        vector_push_back(args, &arg);
    }
    return args;
}

/*
 * An intf call on a receiver of a type variable, e.g. 'x.to_str()' with 'T : ToString': after
 * specialization the receiver is a value of a concrete class, which carries no itable. Call the
 * method of the class directly, or if the class does not define it, make the receiver an intf
 * value of the trait for the intf call. An intf call on an open trait instance gets the callee
 * of the concrete instance.
 */
static void specialize_intf_call(KlrInsn *insn, KlrInsn *clone, KlrSpecializeHooks *hooks)
{
    KlrValue *callee = insn_oper_value(insn, 0);
    TypeSpec *trait_ts;
    if (callee->kind == KLR_VALUE_INTF) {
        trait_ts = ((KlrIntf *)callee)->trait->ts;
    } else if (callee->kind == KLR_VALUE_EXT_INTF) {
        trait_ts = ((KlrExtIntf *)callee)->trait->ts;
    } else {
        return;
    }

    ASSERT(insn->num_opers >= 2);
    TypeSpec *recv_ts = insn_oper_value(insn, 1)->ts;
    KlrValue *recv = insn_oper_value(clone, 1);

    // an intf call on an open trait instance (Sequence[T]): the receiver is now a value of the
    // concrete instance (Sequence[int64]), call the intf of that instance, as hand-written code
    if (recv_ts && recv_ts->kind == TYPE_GENERIC_REF) {
        KlrValue *meth = hooks->resolve_method(recv->ts, callee, hooks->ctx);
        if (meth) set_operand_at(clone, 0, meth);
        return;
    }

    if (!type_is_generic_var(recv_ts)) return;

    KlrValue *meth = hooks->resolve_method(recv->ts, callee, hooks->ctx);
    if (meth) {
        set_operand_at(clone, 0, meth);
        return;
    }

    int intf_index = hooks->intf_index(recv->ts, trait_ts, hooks->ctx);
    if (intf_index < 0) return;
    KlrBuilder bldr;
    klr_builder_before(&bldr, clone);
    KlrValue *val = klr_build_make_intf(&bldr, recv, trait_ts, intf_index, "");
    set_operand_at(clone, 1, val);
}

/*
 * 'cast %x T to Trait' is left by irgen where a value of a type variable goes into a trait (it is
 * a move for the generic function): after specialization a value of a concrete class needs a
 * make_intf.
 */
static void specialize_intf_cast(KlrInsn *insn, KlrInsn *clone, KlrSpecializeHooks *hooks)
{
    if (!type_is_generic_var(insn_oper_value(insn, 0)->ts)) return;

    KlrValue *src = insn_oper_value(clone, 0);
    int intf_index = hooks->intf_index(src->ts, clone->ts, hooks->ctx);
    if (intf_index < 0) return;
    clone->code = OP_MAKE_INTF;
    set_raw_imm(&clone->raws[2], intf_index);
}

/* Clone the body of generic function 'fn' into 'new_fn', the specialized function already
   declared (with its specialized signature) for type arguments 'tp_args'. A call to a generic
   function with type variables is routed with the concrete type args, or kept as the erased
   call if there is no such function; the uses of values of type variables as intf values are
   fixed for the concrete types. */
void klr_specialize_func(KlrFunc *fn, KlrFunc *new_fn, Vector *tp_args, KlrSpecializeHooks *hooks)
{
    KlrValue *fval = (KlrValue *)new_fn;
    ASSERT(vector_size(&fn->params) == vector_size(&new_fn->params));

    // create value map
    HashMap val_map;
    hashmap_init(&val_map, _ptr_equal_);

    // map parameters
    KlrParam *arg;
    vector_foreach(arg, &fn->params) {
        KlrValue *new_param = vector_get(&new_fn->params, i__);
        add_to_map(&val_map, arg, new_param);
    }

    // copy basic blocks
    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        KlrBasicBlock *new_bb = klr_append_block(fval, bb->name);
        add_to_map(&val_map, bb, new_bb);
    }

    // clone instructions with specialized types; all are cloned before any operand is set, so
    // that an operand defined later in block order is already mapped
    basic_block_foreach(bb, fn) {
        KlrBasicBlock *new_bb = get_from_map(&val_map, bb);
        KlrBuilder bldr;
        klr_builder_end(&bldr, new_bb);
        KlrInsn *insn;
        insn_foreach(insn, bb) {
            TypeSpec *ts = hooks->specialize_ts(insn->ts, tp_args, hooks->ctx);
            KlrInsn *clone = klr_build_clone(&bldr, insn, ts);
            clone->target_ts = hooks->specialize_ts(insn->target_ts, tp_args, hooks->ctx);
            add_to_map(&val_map, insn, clone);
        }
    }

    // copy the CFG edges, which the jmp/branch/ret builders add when building, not the clone; the
    // edge <sbb, first block> is added by klr_append_block()
    add_to_map(&val_map, fn->ebb, new_fn->ebb);
    basic_block_foreach(bb, fn) {
        KlrEdge *edge;
        edge_out_foreach(edge, bb) {
            KlrBasicBlock *dst = get_from_map(&val_map, edge->dst);
            ASSERT(dst);
            klr_link_edge(get_from_map(&val_map, bb), dst);
        }
    }

    // set operands
    basic_block_foreach(bb, fn) {
        KlrInsn *insn;
        insn_foreach(insn, bb) {
            KlrInsn *clone = get_from_map(&val_map, insn);
            for (int i = 0; i < insn->num_opers; i++) {
                KlrValue *ref = map_value(&val_map, insn_oper_value(insn, i));
                klr_init_clone_oper(clone, insn, i, ref);
            }

            // route a call to a generic func (incl. a recursive one) with the concrete type args
            if (insn->tp_args) {
                ASSERT(insn->code == OP_IR_CALL);
                Vector *args = specialize_tp_args(insn->tp_args, tp_args, hooks);
                KlrValue *callee = hooks->resolve_call(insn_oper_value(insn, 0), args, hooks->ctx);
                if (callee) set_operand_at(clone, 0, callee);
                clone->tp_args = NULL;
                vector_destroy(args);
            } else if (insn->code == OP_IR_CALL) {
                specialize_intf_call(insn, clone, hooks);
            } else if (insn->code == OP_IR_CAST) {
                specialize_intf_cast(insn, clone, hooks);
            }
        }
    }
}

#ifdef __cplusplus
}
#endif

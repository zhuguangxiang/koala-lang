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

static void build_specialize_insn(KlrInsn *insn, HashMap *map, KlrBuilder *bldr)
{
    switch (insn->code) {
        case OP_IR_LOCAL: {
            KlrValue *local;
            if (insn->flags & KLR_INSN_FLAGS_CONST) {
                local = klr_build_local(bldr, insn->ts, insn->name);
            } else {
                local = klr_build_local_var(bldr, insn->ts, insn->name);
            }
            add_to_map(map, insn, local);
            break;
        }

        case OP_MOVE: {
            KlrValue *src = insn_oper_value(insn, 0);
            KlrValue *dst = insn_oper_value(insn, 1);
            src = get_from_map(map, src);
            dst = get_from_map(map, dst);
            klr_build_move(bldr, src, dst);
            break;
        }

        case OP_BINARY_GT: {
            KlrValue *v1 = insn_oper_value(insn, 0);
            KlrValue *v2 = insn_oper_value(insn, 1);
            v1 = get_from_map(map, v1);
            v2 = get_from_map(map, v2);
            KlrValue *ret = klr_build_cmpgt(bldr, v1, v2, insn->name);
            add_to_map(map, insn, ret);
            break;
        }

        case OP_BINARY_LT: {
            KlrValue *v1 = insn_oper_value(insn, 0);
            KlrValue *v2 = insn_oper_value(insn, 1);
            v1 = get_from_map(map, v1);
            v2 = get_from_map(map, v2);
            KlrValue *ret = klr_build_cmplt(bldr, v1, v2, insn->name);
            add_to_map(map, insn, ret);
            break;
        }

        case OP_IR_JMP_COND: {
            KlrValue *v = insn_oper_value(insn, 0);
            v = get_from_map(map, v);

            KlrValue *bb0 = insn_oper_value(insn, 2);
            KlrValue *bb1 = insn_oper_value(insn, 3);

            bb0 = get_from_map(map, bb0);
            bb1 = get_from_map(map, bb1);
            klr_build_jmp_cond(bldr, v, (KlrBasicBlock *)bb0, (KlrBasicBlock *)bb1);
            break;
        }

        case OP_JMP: {
            KlrValue *bb = insn_oper_value(insn, 0);
            bb = get_from_map(map, bb);
            klr_build_jmp(bldr, (KlrBasicBlock *)bb);
            break;
        }

        case OP_RET: {
            KlrValue *v = insn_oper_value(insn, 0);
            v = get_from_map(map, v);
            klr_build_ret(bldr, v);
            break;
        }

        case OP_RET_VOID: {
            klr_build_ret_void(bldr);
            break;
        }

        default: {
            NYI();
            break;
        }
    }
}

KlrValue *klr_specialize_func(KlrFunc *fn, char *mangled_name, Vector *tp_args)
{
    // new function
    KlrModule *m = fn->module;
    TypeSpec *ret = fn->ts;
    TypeSpec *new_ret = type_spec_specialize(fn->ts, tp_args);
    char *new_name = mangled_name;
    KlrValue *fval = klr_add_func(m, new_ret, new_name);
    KlrFunc *new_fn = (KlrFunc *)fval;

    // create value map
    HashMap val_map;
    hashmap_init(&val_map, _ptr_equal_);

    // add func to map(recursive call)
    add_to_map(&val_map, fn, new_fn);

    // copy parameters
    KlrParam *arg;
    vector_foreach(arg, &fn->params) {
        TypeSpec *_ts = type_spec_specialize(arg->ts, tp_args);
        KlrValue *new_param = klr_func_add_param(fval, _ts, arg->name);
        add_to_map(&val_map, arg, new_param);
    }

    // copy basic blocks
    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        KlrBasicBlock *new_bb = klr_append_block(fval, bb->name);
        add_to_map(&val_map, bb, new_bb);
    }

    // copy instructions
    basic_block_foreach(bb, fn) {
        KlrBasicBlock *new_bb = get_from_map(&val_map, bb);
        KlrBuilder bldr;
        klr_builder_end(&bldr, new_bb);
        KlrInsn *insn;
        insn_foreach(insn, bb) {
            build_specialize_insn(insn, &val_map, &bldr);
        }
    }

    return fval;
}

#ifdef __cplusplus
}
#endif

/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "log.h"
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

#define slots(idx) tp->slots[idx]

static unsigned int slot_tp_hash(TValue *self)
{
    TypeObject *tp = kl_typeof(self);
    Object *fn = slots(SLOT_HASH);
    if (IS_CFUNC(fn)) {
        CFuncObject *cfn = (CFuncObject *)fn;
        TValue val = cfn->func(self, NULL, 0);
        return to_int64(&val);
    } else {
        TValue val = obj_value(fn);
        val = kl_do_call_one_arg(&val, self);
        return to_int64(&val);
    }
}

static TValue slot_tp_richcmp(TValue *self, TValue *other, int op)
{
    TypeObject *tp = kl_typeof(self);
    Object *fn = slots(SLOT_EQ + op);
    if (IS_CFUNC(fn)) {
        CFuncObject *cfn = (CFuncObject *)fn;
        return cfn->func(self, other, 1);
    } else {
        TValue val = obj_value(fn);
        TValue args[] = { *self, *other };
        return kl_do_call(&val, args, 2);
    }
}

static TValue slot_tp_str(TValue *self)
{
    TypeObject *tp = kl_typeof(self);
    Object *fn = slots(SLOT_STR);
    if (IS_CFUNC(fn)) {
        CFuncObject *cfn = (CFuncObject *)fn;
        return cfn->func(self, NULL, 0);
    } else {
        TValue val = obj_value(fn);
        return kl_do_call_one_arg(&val, self);
    }
}

static TValue slot_tp_call(TValue *self, TValue *args, int nargs)
{
    TypeObject *tp = kl_typeof(self);
    Object *fn = slots(SLOT_CALL);
    if (IS_CFUNC(fn)) {
        CFuncObject *cfn = (CFuncObject *)fn;
        return cfn->func(self, args, nargs);
    } else {
        TValue val = obj_value(fn);
        return kl_do_call(&val, args, nargs);
    }
}

#define DEFINE_ARITHMETIC_BIN_SLOT(slot, id) \
    static TValue slot_arith_bin_##slot(TValue *self, TValue *other) \
    { \
        TypeObject *tp = kl_typeof(self); \
        Object *fn = slots(id); \
        if (IS_CFUNC(fn)) { \
            CFuncObject *cfn = (CFuncObject *)fn; \
            return cfn->func(self, other, 1); \
        } else { \
            TValue val = obj_value(fn); \
            TValue args[] = { *self, *other }; \
            return kl_do_call(&val, args, 2); \
        } \
    }

DEFINE_ARITHMETIC_BIN_SLOT(add, SLOT_ADD);
DEFINE_ARITHMETIC_BIN_SLOT(sub, SLOT_SUB);
DEFINE_ARITHMETIC_BIN_SLOT(mul, SLOT_MUL);
DEFINE_ARITHMETIC_BIN_SLOT(div, SLOT_DIV);
DEFINE_ARITHMETIC_BIN_SLOT(mod, SLOT_MOD);

static size_t slot_seq_len(TValue *self)
{
    TypeObject *tp = kl_typeof(self);
    Object *fn = slots(SLOT_LEN);
    if (IS_CFUNC(fn)) {
        CFuncObject *cfn = (CFuncObject *)fn;
        TValue val = cfn->func(self, NULL, 0);
        return (size_t)to_int64(&val);
    } else {
        TValue val = obj_value(fn);
        val = kl_do_call_one_arg(&val, self);
        return (size_t)to_int64(&val);
    }
}

static int slot_seq_contains(TValue *self, TValue *item)
{
    TypeObject *tp = kl_typeof(self);
    Object *fn = slots(SLOT_CONTAINS);
    if (IS_CFUNC(fn)) {
        CFuncObject *cfn = (CFuncObject *)fn;
        TValue val = cfn->func(self, item, 1);
        return (int)to_int64(&val);
    } else {
        TValue val = obj_value(fn);
        TValue args[] = { *self, *item };
        val = kl_do_call(&val, args, 2);
        return (int)to_int64(&val);
    }
}

static TValue slot_seq_get(TValue *self, size_t index)
{
    TypeObject *tp = kl_typeof(self);
    Object *fn = slots(SLOT_GET_ITEM);
    if (IS_CFUNC(fn)) {
        CFuncObject *cfn = (CFuncObject *)fn;
        TValue args[] = { int64_value(index) };
        TValue val = cfn->func(self, args, 1);
        return val;
    } else {
        TValue val = obj_value(fn);
        TValue args[] = { *self, int64_value(index) };
        val = kl_do_call(&val, args, 2);
        return val;
    }
}

static void slot_seq_set(TValue *self, size_t index, TValue *value)
{
    TypeObject *tp = kl_typeof(self);
    Object *fn = slots(SLOT_SET_ITEM);
    if (IS_CFUNC(fn)) {
        CFuncObject *cfn = (CFuncObject *)fn;
        TValue args[] = { int64_value(index), *value };
        cfn->func(self, args, 3);
    } else {
        TValue val = obj_value(fn);
        TValue args[] = { *self, int64_value(index), *value };
        kl_do_call(&val, args, 3);
    }
}

typedef struct _SlotDef {
    char *name;
    int offset;
    void *func;
    SlotId id;
} SlotDef;

#define TPSLOT(NAME, SLOT, FUNC, ID) { NAME, offsetof(TypeObject, SLOT), (void *)(FUNC), ID }

static SlotDef slotdefs[] = {
    TPSLOT("__hash__", hash, slot_tp_hash, SLOT_HASH),
    TPSLOT("__eq__", cmp, slot_tp_richcmp, SLOT_EQ),
    TPSLOT("__ne__", cmp, slot_tp_richcmp, SLOT_NE),
    TPSLOT("__lt__", cmp, slot_tp_richcmp, SLOT_LT),
    TPSLOT("__le__", cmp, slot_tp_richcmp, SLOT_LE),
    TPSLOT("__gt__", cmp, slot_tp_richcmp, SLOT_GT),
    TPSLOT("__ge__", cmp, slot_tp_richcmp, SLOT_GE),
    TPSLOT("__str__", str, slot_tp_str, SLOT_STR),
    TPSLOT("__call__", call, slot_tp_call, SLOT_CALL),
    { NULL },
};

#define BINSLOT(NAME, SLOT, FUNC, ID) \
    { NAME, offsetof(ArithmeticMethods, SLOT), (void *)(FUNC), ID }

static SlotDef arith_slotdefs[] = {
    BINSLOT("__add__", add, slot_arith_bin_add, SLOT_ADD),
    BINSLOT("__sub__", sub, slot_arith_bin_sub, SLOT_SUB),
    BINSLOT("__mul__", mul, slot_arith_bin_mul, SLOT_MUL),
    BINSLOT("__div__", div, slot_arith_bin_div, SLOT_DIV),
    BINSLOT("__mod__", mod, slot_arith_bin_mod, SLOT_MOD),
    { NULL },
};

#define SEQSLOT(NAME, SLOT, FUNC, ID) { NAME, offsetof(SeqMethods, SLOT), (void *)(FUNC), ID }

static SlotDef seq_slotdefs[] = {
    SEQSLOT("__len__", len, slot_seq_len, SLOT_LEN),
    SEQSLOT("__contains__", contains, slot_seq_contains, SLOT_CONTAINS),
    SEQSLOT("__getitem__", get, slot_seq_get, SLOT_GET_ITEM),
    SEQSLOT("__setitem__", set, slot_seq_set, SLOT_SET_ITEM),
    SEQSLOT("__getslice__", get_slice, NULL, SLOT_GET_SLICE),
    SEQSLOT("__setslice__", set_slice, NULL, SLOT_SET_SLICE),
    { NULL },
};

#define MAPSLOT(NAME, SLOT, FUNC, ID) { NAME, offsetof(MapMethods, SLOT), (void *)(FUNC), ID }

static SlotDef map_slotdefs[] = {
    MAPSLOT("__len__", len, NULL, SLOT_LEN),
    MAPSLOT("__contains__", contains, NULL, SLOT_CONTAINS),
    MAPSLOT("__getsub__", get_sub, NULL, SLOT_GET_SUBSCRIPT),
    MAPSLOT("__setsub__", set_sub, NULL, SLOT_SET_SUBSCRIPT),
    { NULL },
};

void kl_tp_install_slots(TypeObject *tp)
{
    // initialize slots[]
    memset(tp->slots, 0, sizeof(tp->slots));

    // bind to slots[]
    for (SlotDef *slot = slotdefs; slot->name; slot++) {
        Object *fn = stbl_find_obj(&tp->members, slot->name);
        if (fn) {
            log_info("binding method '%s' to slots[%d] of class '%s'", slot->name, slot->id,
                     tp->name);

            tp->slots[slot->id] = fn;

            void **field = (void **)((char *)tp + slot->offset);
            // if the type has implemented this slot function, do not override it.
            if (*field == NULL) *field = slot->func;
        }
    }

    // bind arith slots[]
    for (SlotDef *slot = arith_slotdefs; slot->name; slot++) {
        Object *fn = stbl_find_obj(&tp->members, slot->name);
        if (fn) {
            log_info("binding arithmetic op '%s' to slots[%d] of class '%s'", slot->name, slot->id,
                     tp->name);
            tp->slots[slot->id] = fn;

            ArithmeticMethods *arith = tp->arith;
            if (!arith) {
                arith = mm_alloc(sizeof(ArithmeticMethods));
                tp->arith = arith;
            }

            void **field = (void **)((char *)arith + slot->offset);
            // if the type has implemented this slot function, do not override it.
            if (*field == NULL) *field = slot->func;
        }
    }

    // bind bit slots[]

    // bind sequence slots[]
    for (SlotDef *slot = seq_slotdefs; slot->name; slot++) {
        Object *fn = stbl_find_obj(&tp->members, slot->name);
        if (fn) {
            log_info("binding sequence op '%s' to slots[%d] of class '%s'", slot->name, slot->id,
                     tp->name);
            tp->slots[slot->id] = fn;

            SeqMethods *seq = tp->seq;
            if (!seq) {
                seq = mm_alloc(sizeof(SeqMethods));
                tp->seq = seq;
            }

            void **field = (void **)((char *)seq + slot->offset);
            // if the type has implemented this slot function, do not override it.
            if (*field == NULL) *field = slot->func;
        }
    }

    // bind map slots[]
}

#ifdef __cplusplus
}
#endif

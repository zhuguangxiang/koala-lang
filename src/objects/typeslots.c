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

#define BINSLOT(NAME, SLOT, FUNC, ID) { NAME, offsetof(NumberMethods, SLOT), (void *)(FUNC), ID }

static SlotDef num_slotdefs[] = {
    BINSLOT("__add__", add, NULL, SLOT_ADD), BINSLOT("__sub__", sub, NULL, SLOT_SUB),
    BINSLOT("__mul__", mul, NULL, SLOT_MUL), BINSLOT("__div__", div, NULL, SLOT_DIV),
    BINSLOT("__mod__", mod, NULL, SLOT_MOD), { NULL },
};

#define SEQSLOT(NAME, SLOT, FUNC, ID) { NAME, offsetof(SeqMethods, SLOT), (void *)(FUNC), ID }

static SlotDef seq_slotdefs[] = {
    SEQSLOT("__len__", len, NULL, SLOT_LEN),
    SEQSLOT("__contains__", contains, NULL, SLOT_CONTAINS),
    SEQSLOT("__getitem__", get, NULL, SLOT_GET_ITEM),
    SEQSLOT("__setitem__", set, NULL, SLOT_SET_ITEM),
    SEQSLOT("__getslice__", get_slice, NULL, SLOT_GET_SLICE),
    SEQSLOT("__setslice__", set_slice, NULL, SLOT_SET_SLICE),
    { NULL },
};

#define MAPSLOT(NAME, SLOT, FUNC, ID) { NAME, offsetof(MapMethods, SLOT), (void *)(FUNC), ID }

static SlotDef map_slotdefs[] = {
    MAPSLOT("__len__", len, NULL, SLOT_LEN),
    MAPSLOT("__contains__", contains, NULL, SLOT_CONTAINS),
    MAPSLOT("__getsub__", get, NULL, SLOT_GET_SUBSCRIPT),
    MAPSLOT("__setsub__", set, NULL, SLOT_SET_SUBSCRIPT),
    { NULL },
};

void type_install_slots(TypeObject *tp)
{
    // initialize slots[]
    memset(tp->slots, 0, sizeof(tp->slots));

    // bind to slots[]
    for (SlotDef *slot = slotdefs; slot->name; slot++) {
        Object *fn = stbl_find_obj(&tp->members, slot->name);
        if (fn) {
            log_info("binding method '%s' to slots[%d] of class/trait '%s'", slot->name, slot->id,
                     tp->name);

            tp->slots[slot->id] = fn;

            void **field = (void **)((char *)tp + slot->offset);
            // if the type has implemented this slot function, do not override it.
            if (*field == NULL) *field = slot->func;
        }
    }
}

#ifdef __cplusplus
}
#endif

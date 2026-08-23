/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "log.h"
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _SlotDef {
    char *name;
    SlotId id;
} SlotDef;

#define TPSLOT(NAME, ID) { NAME, ID }

static SlotDef slotdefs[] = {
    TPSLOT("__eq__", SLOT_EQ),
    TPSLOT("__ne__", SLOT_NE),
    TPSLOT("__lt__", SLOT_LT),
    TPSLOT("__le__", SLOT_LE),
    TPSLOT("__gt__", SLOT_GT),
    TPSLOT("__ge__", SLOT_GE),

    TPSLOT("__hash__", SLOT_HASH),

    TPSLOT("__len__", SLOT_LEN),
    TPSLOT("__getitem__", SLOT_GET_ITEM),
    TPSLOT("__setitem__", SLOT_SET_ITEM),
    TPSLOT("__contains__", SLOT_CONTAINS),

    TPSLOT("__getslice__", SLOT_GET_SLICE),
    TPSLOT("__setslice__", SLOT_SET_SLICE),

    TPSLOT("__getsub__", SLOT_GET_SUB),
    TPSLOT("__setsub__", SLOT_SET_SUB),

    TPSLOT("__str__", SLOT_STR),

    TPSLOT("__add__", SLOT_ADD),
    TPSLOT("__sub__", SLOT_SUB),
    TPSLOT("__mul__", SLOT_MUL),
    TPSLOT("__div__", SLOT_DIV),
    TPSLOT("__mod__", SLOT_MOD),
    TPSLOT("__neg__", SLOT_NEG),

    TPSLOT("__shl__", SLOT_SHL),
    TPSLOT("__shr__", SLOT_SHR),
    TPSLOT("__bitand__", SLOT_BIT_AND),
    TPSLOT("__bitor__", SLOT_BIT_OR),
    TPSLOT("__bitxor__", SLOT_BIT_XOR),
    TPSLOT("__bitnot__", SLOT_BIT_NOT),

    TPSLOT(NULL, 0),
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
        }
    }
}

#ifdef __cplusplus
}
#endif

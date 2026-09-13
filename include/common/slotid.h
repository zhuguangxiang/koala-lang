/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_SLOTID_H_
#define _KOALA_SLOTID_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    /* hot slots -- dict/loop hot paths, all within the first cache line of TypeObject */

    /* comparison protocol */
    SLOT_EQ, // __eq__
    SLOT_NE, // __ne__
    SLOT_LT, // __lt__
    SLOT_LE, // __le__
    SLOT_GT, // __gt__
    SLOT_GE, // __ge__

    /* hashable protocol -- hit on every dict/set probe */
    SLOT_HASH, // __hash__

    /* warm slots -- second cache line */

    /* sequence protocol */
    SLOT_LEN,      // __len__
    SLOT_GET_ITEM, // __getitem__
    SLOT_SET_ITEM, // __setitem__
    SLOT_CONTAINS, // __contains__

    /* slice protocol */
    SLOT_GET_SLICE, // __getslice__
    SLOT_SET_SLICE, // __setslice__

    /* cold slots */

    /* printable protocol  */
    SLOT_STR, // __str__

    /* arithmetic protocol */
    SLOT_ADD, // __add__
    SLOT_SUB, // __sub__
    SLOT_MUL, // __mul__
    SLOT_DIV, // __div__
    SLOT_MOD, // __mod__
    SLOT_NEG, // __neg__

    /* bitwise protocol */
    SLOT_SHL,     // __shl__
    SLOT_SHR,     // __shr__
    SLOT_BIT_AND, // __and__
    SLOT_BIT_OR,  // __or__
    SLOT_BIT_XOR, // __xor__
    SLOT_BIT_NOT, // __invert__

    /* other slots */
    SLOT_INIT, // __init__
    SLOT_FINI, // __fini__

    SLOT_MAX
} SlotId;

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_SLOTID_H_ */

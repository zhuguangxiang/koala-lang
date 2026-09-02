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
    SLOT_EQ, // OP_NUM_EQ
    SLOT_NE, // OP_NUM_NE
    SLOT_LT, // OP_NUM_LT
    SLOT_LE, // OP_NUM_LE
    SLOT_GT, // OP_NUM_GT
    SLOT_GE, // OP_NUM_GE

    /* hashable protocol -- hit on every dict/set probe */
    SLOT_HASH,

    /* warm slots -- second cache line */

    /* sequence protocol */
    SLOT_LEN,      // OP_LEN
    SLOT_GET_ITEM, // OP_SEQ_GET and OP_SEQ_GET_IMM
    SLOT_SET_ITEM, // OP_SEQ_SET and OP_SEQ_SET_IMM
    SLOT_CONTAINS, // shared by sequence and mapping protocols, the 'in' OP

    /* slice protocol */
    SLOT_GET_SLICE, // __getslice__
    SLOT_SET_SLICE, // __setslice__

    /* mapping subscript protocol */
    SLOT_GET_SUB, // __getsub__
    SLOT_SET_SUB, // __setsub__

    /* cold slots */

    /* printable protocol  */
    SLOT_STR, // __str__

    /* arithmetic protocol */
    SLOT_ADD, // OP_NUM_ADD
    SLOT_SUB, // OP_NUM_SUB
    SLOT_MUL, // OP_NUM_MUL
    SLOT_DIV, // OP_NUM_DIV
    SLOT_MOD, // OP_NUM_MOD
    SLOT_NEG, // unary minus

    /* bitwise protocol */
    SLOT_SHL,     // OP_NUM_SHL
    SLOT_SHR,     // OP_NUM_SHR
    SLOT_BIT_AND, // OP_NUM_AND
    SLOT_BIT_OR,  // OP_NUM_OR
    SLOT_BIT_XOR, // OP_NUM_XOR
    SLOT_BIT_NOT, // bitwise NOT

    /* other slots */
    SLOT_INIT, // __init__
    SLOT_FINI, // __fini__

    SLOT_MAX
} SlotId;

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_SLOTID_H_ */

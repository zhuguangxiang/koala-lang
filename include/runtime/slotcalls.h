
#include "slotid.h"

static inline TValue kl_call_cmp(TValue *a, TValue *b, int slot)
{
    TValue args[] = { *a, *b };
    return kl_call_slot(args, 2, slot);
}

static inline unsigned int kl_hash(TValue *val)
{
    TValue ret = kl_call_slot(val, 1, SLOT_HASH);
    return (unsigned int)to_int64(&ret);
}

static inline void kl_call_fmt(TValue *obj, Formatter *fmt)
{
    TValue args[] = { *obj, obj_value(fmt) };
    kl_call_slot(args, 2, SLOT_FMT);
}

static inline TValue kl_call_iter(TValue *obj, int64_t step)
{
    TValue args[] = { *obj, int64_value(step) };
    return kl_call_slot(args, 2, SLOT_ITER);
}

static inline TValue kl_call_next(TValue *obj) { return kl_call_slot(obj, 1, SLOT_NEXT); }

static inline TValue kl_call_get_item(TValue *obj, TValue *index)
{
    TValue args[] = { *obj, *index };
    return kl_call_slot(args, 2, SLOT_GET_ITEM);
}

static inline TValue kl_call_set_item(TValue *obj, TValue *index, TValue *value)
{
    TValue args[] = { *obj, *index, *value };
    return kl_call_slot(args, 3, SLOT_SET_ITEM);
}

static inline TValue kl_call_contains(TValue *obj, TValue *item)
{
    TValue args[] = { *obj, *item };
    return kl_call_slot(args, 2, SLOT_CONTAINS);
}

static inline TValue kl_call_get_slice(TValue *obj, TValue *slice)
{
    TValue args[] = { *obj, *slice };
    return kl_call_slot(args, 2, SLOT_GET_SLICE);
}

static inline TValue kl_call_set_slice(TValue *obj, TValue *slice, TValue *value)
{
    TValue args[] = { *obj, *slice, *value };
    return kl_call_slot(args, 3, SLOT_SET_SLICE);
}

static inline TValue kl_call_add(TValue *a, TValue *b)
{
    TValue args[] = { *a, *b };
    return kl_call_slot(args, 2, SLOT_ADD);
}

static inline TValue kl_call_sub(TValue *a, TValue *b)
{
    TValue args[] = { *a, *b };
    return kl_call_slot(args, 2, SLOT_SUB);
}

static inline TValue kl_call_mul(TValue *a, TValue *b)
{
    TValue args[] = { *a, *b };
    return kl_call_slot(args, 2, SLOT_MUL);
}

static inline TValue kl_call_div(TValue *a, TValue *b)
{
    TValue args[] = { *a, *b };
    return kl_call_slot(args, 2, SLOT_DIV);
}

static inline TValue kl_call_mod(TValue *a, TValue *b)
{
    TValue args[] = { *a, *b };
    return kl_call_slot(args, 2, SLOT_MOD);
}

static inline TValue kl_call_neg(TValue *a)
{
    TValue args[] = { *a };
    return kl_call_slot(args, 1, SLOT_NEG);
}

static inline TValue kl_call_bit_lshift(TValue *a, TValue *b)
{
    TValue args[] = { *a, *b };
    return kl_call_slot(args, 2, SLOT_SHL);
}

static inline TValue kl_call_bit_rshift(TValue *a, TValue *b)
{
    TValue args[] = { *a, *b };
    return kl_call_slot(args, 2, SLOT_SHR);
}

static inline TValue kl_call_bit_and(TValue *a, TValue *b)
{
    TValue args[] = { *a, *b };
    return kl_call_slot(args, 2, SLOT_BIT_AND);
}

static inline TValue kl_call_bit_or(TValue *a, TValue *b)
{
    TValue args[] = { *a, *b };
    return kl_call_slot(args, 2, SLOT_BIT_OR);
}

static inline TValue kl_call_bit_xor(TValue *a, TValue *b)
{
    TValue args[] = { *a, *b };
    return kl_call_slot(args, 2, SLOT_BIT_XOR);
}

static inline TValue kl_call_bit_not(TValue *a)
{
    TValue args[] = { *a };
    return kl_call_slot(args, 1, SLOT_BIT_NOT);
}

static inline Object *kl_to_str(TValue *val)
{
    TValue s = kl_call_slot(val, 1, SLOT_STR);
    return to_obj(&s);
}

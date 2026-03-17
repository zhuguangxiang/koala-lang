/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "ir.h"
#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif

void isel_lower_local(KlrInsn *insn, KlrFunc *fn)
{
    KlrValue *var = insn_oper_value(insn, 0);
    TypeSpec *ts = type_table[var->id];

    if (type_is_int(ts->ts)) {
        insn->code = OP_IR_LOCAL_INT;
    } else if (type_is_float(ts->ts)) {
        insn->code = OP_IR_LOCAL_FLOAT;
    } else {
        insn->code = OP_IR_LOCAL_GENERIC;
    }
}

#ifdef __cplusplus
}
#endif

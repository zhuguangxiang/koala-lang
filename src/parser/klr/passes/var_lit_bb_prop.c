/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "ir.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
The `var` variable is mutable, so it can be propated only one basic block inside, and only
for literal values. This is a local constant propagation and no need SSA format. It can be
used to fold list/tuple/map/set literals, and also can be used to fold const variables. In
one basic block, if there are many store insns to the same variable, only the last store
insn can be propated, and the previous store insns will be removed.
*/
void klr_var_lit_bb_prop_pass(KlrFunc *func, void *ctx)
{
    KlrInsn *insn;
    basic_block_foreach(insn, func) {
    }
}

#ifdef __cplusplus
}
#endif

/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "bytebuf.h"
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

static TValue splitter_hash_next(TValue *self, TValue *args, int nargs) { return bool_value(1); }

static TValue splitter_next(TValue *self, TValue *args, int nargs) { return bool_value(1); }

static TValue lines_hash_next(TValue *self, TValue *args, int nargs) { return bool_value(1); }

static TValue lines_next(TValue *self, TValue *args, int nargs) { return bool_value(1); }

static TValue words_hash_next(TValue *self, TValue *args, int nargs) { return bool_value(1); }

static TValue words_next(TValue *self, TValue *args, int nargs) { return bool_value(1); }

void io_native_lib_init(NativeLib *lib)
{
    kl_reg_meth(lib, "Splitter", "has_next", splitter_hash_next);
    kl_reg_meth(lib, "Splitter", "next", splitter_next);
    kl_reg_meth(lib, "Lines", "has_next", lines_hash_next);
    kl_reg_meth(lib, "Lines", "next", lines_next);
    kl_reg_meth(lib, "Words", "has_next", words_hash_next);
    kl_reg_meth(lib, "Words", "next", words_next);
    kl_reg_type(lib, &bytebuf_type);
}

#ifdef __cplusplus
}
#endif

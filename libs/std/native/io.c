/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

static TValue buf_reader_read_until(TValue *self, TValue *args, int nargs) { return nil_value; }

static TValue splitter_hash_next(TValue *self, TValue *args, int nargs) { return bool_value(1); }

static TValue splitter_next(TValue *self, TValue *args, int nargs) { return bool_value(1); }

static TValue lines_hash_next(TValue *self, TValue *args, int nargs) { return bool_value(1); }

static TValue lines_next(TValue *self, TValue *args, int nargs) { return bool_value(1); }

static TValue words_hash_next(TValue *self, TValue *args, int nargs) { return bool_value(1); }

static TValue words_next(TValue *self, TValue *args, int nargs) { return bool_value(1); }

void io_native_lib_init(NativeLib *lib)
{
    kl_reg_meth(lib, "BufReader", "read_until", buf_reader_read_until);
    kl_reg_meth(lib, "Splitter", "has_next", splitter_hash_next);
    kl_reg_meth(lib, "Splitter", "next", splitter_next);
    kl_reg_meth(lib, "Lines", "has_next", lines_hash_next);
    kl_reg_meth(lib, "Lines", "next", lines_next);
    kl_reg_meth(lib, "Words", "has_next", words_hash_next);
    kl_reg_meth(lib, "Words", "next", words_next);
}

#ifdef __cplusplus
}
#endif

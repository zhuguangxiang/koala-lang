/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

static TValue buf_reader_read_until(TValue *self, TValue *args, int nargs) { return none_value; }

static TValue splitter___hash_next__(TValue *self, TValue *args, int nargs)
{
    return bool_value(1);
}

static TValue splitter___next__(TValue *self, TValue *args, int nargs) { return bool_value(1); }

static TValue lines___hash_next__(TValue *self, TValue *args, int nargs) { return bool_value(1); }

static TValue lines___next__(TValue *self, TValue *args, int nargs) { return bool_value(1); }

static TValue words___hash_next__(TValue *self, TValue *args, int nargs) { return bool_value(1); }

static TValue words___next__(TValue *self, TValue *args, int nargs) { return bool_value(1); }

void io_native_lib_init(NativeLib *lib)
{
    kl_reg_meth(lib, "BufReader", "read_until", buf_reader_read_until);
    kl_reg_meth(lib, "Splitter", "__has_next__", splitter___hash_next__);
    kl_reg_meth(lib, "Splitter", "__next__", splitter___next__);
    kl_reg_meth(lib, "Lines", "__has_next__", lines___hash_next__);
    kl_reg_meth(lib, "Lines", "__next__", lines___next__);
    kl_reg_meth(lib, "Words", "__has_next__", words___hash_next__);
    kl_reg_meth(lib, "Words", "__next__", words___next__);
}

#ifdef __cplusplus
}
#endif

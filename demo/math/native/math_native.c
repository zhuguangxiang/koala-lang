/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "object.h"

/// Return `x * 3`.
///
/// Koala declaration:
///
///     @native
///     func triple(x int) int {}
///
/// ABI: `(self, args, nargs)`, one int argument, int return.
static TValue native_triple(TValue *self, TValue *args, int nargs)
{
    printf("native_triple called with %d args\n", nargs);
    ASSERT(nargs == 1);
    int64_t x = kl_arg_int(0);
    return int64_value(x * 3);
}

/// Entry point called by the runtime after `dlopen("libdemo_math.so")`.
///
/// The name must be `<native.name>_native_lib_init`, where
/// `<native.name>` is `demo_math` from `__pkg__.toml`.
void math_native_lib_init(NativeLib *lib) { kl_reg_func(lib, "triple", native_triple); }

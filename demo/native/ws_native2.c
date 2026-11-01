/*
 * demo workspace — second shared native library.
 */

#include "object.h"

static TValue native_ws_square(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    int64_t x = kl_arg_int(0);
    return int64_value(x * x);
}

void use_two_natives_native_lib_init(NativeLib *lib)
{
    kl_reg_func(lib, "ws_square", native_ws_square);
}

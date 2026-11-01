/*
 * demo workspace — shared native helpers.
 */

#include "object.h"

static TValue native_ws_double(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    int64_t x = kl_arg_int(0);
    return int64_value(x * 2);
}

void use_ws_native_native_lib_init(NativeLib *lib)
{
    kl_reg_func(lib, "ws_double", native_ws_double);
}

void use_two_natives_native_lib_init(NativeLib *lib)
{
    kl_reg_func(lib, "ws_double", native_ws_double);
}

/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <stdlib.h>
#include "args.h"
#include "listobj.h"

#ifdef __cplusplus
extern "C" {
#endif

static TValue os_args(TValue *self, TValue *args, int nargs)
{
    Object *list = kl_new_list();

    KoalaOptions *opt = kl_get_cmd_opt();
    for (int i = 1; i < opt->argc; i++) {
        TValue s = kl_val_str(opt->argv[i]);
        kl_list_append(list, s);
    }

    return obj_value(list);
}

/*
func exit(code = 0)
*/
static TValue sys_exit(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    int64_t code = kl_arg_int(0);
    exit((int)code);
    return no_value; // unreachable
}

void sys_native_lib_init(NativeLib *lib)
{
    kl_reg_func(lib, "args", os_args);
    kl_reg_func(lib, "exit", sys_exit);
}

#ifdef __cplusplus
}
#endif

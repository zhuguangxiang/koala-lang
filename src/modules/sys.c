/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "args.h"
#include "listobj.h"

#ifdef __cplusplus
extern "C" {
#endif

extern KoalaOptions kl_cmd_opt;

static TValue os_args(TValue *self, TValue *args, int nargs)
{
    Object *list = kl_new_list();

    for (int i = 1; i < kl_cmd_opt.argc; i++) {
        TValue s = kl_val_str(kl_cmd_opt.argv[i]);
        kl_list_append(list, s);
    }

    return obj_value(list);
}

void sys_native_lib_init(NativeLib *lib) { kl_reg_func(lib, "args", os_args); }

#ifdef __cplusplus
}
#endif

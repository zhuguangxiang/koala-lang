/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "object.h"

TValue test_pkg_foo_func_(TValue *self, TValue *args, int nargs) { return none_value; }

void pkg1_native_module_init(void) { kl_register_native("test_pkg_foo_func", test_pkg_foo_func_); }

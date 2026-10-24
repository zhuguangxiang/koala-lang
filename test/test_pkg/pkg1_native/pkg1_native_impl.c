/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "object.h"

/*
 gcc -fPIC -shared pkg1_native/pkg1_native_impl.c -o libpkg1_native.so -I../../include/runtime \
 -I../../include/common -L../../build/DebugTest/lib/ -lkoala -g
*/

/* func test_pkg_foo(an Animal) */
TValue test_pkg_foo_func(TValue *self, TValue *args, int nargs)
{
    // an.fly("Eagle", 3)
    // printf("test_pkg_foo_func is called\n");
    TValue _args[] = { args[0], obj_value(kl_new_str("Eagle")), int64_value(3) };
    return kl_call_intf(_args, 3, 0);
}

// func swim(name str, age int) str
TValue foo_swim_func(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);

    TValue *name = &args[0];
    TValue *age = &args[1];

    ASSERT(kl_typeof(name) == &str_type);
    ASSERT(kl_typeof(age) == &int64_type);

    char *_name = STR_BUF(to_obj(name));
    int _age = to_int64(age);

    Object *s = kl_new_fmt_str("<%s is swimming, age %d>", _name, _age);
    return obj_value(s);
}

/* pub static func magic_add(a int, b int) int (class NativeMath in pkg1) */
TValue native_math_magic_add_func(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    return int64_value(kl_arg_int64(0) + kl_arg_int64(1));
}

/* pub static func magic_mul(a int, b int) int (class NativeUtil in test_static_methods.kl) */
TValue native_util_magic_mul_func(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    return int64_value(kl_arg_int64(0) * kl_arg_int64(1));
}

void pkg1_native_lib_init(NativeLib *lib)
{
    kl_reg_func(lib, "test_pkg_foo", test_pkg_foo_func);
    kl_reg_meth(lib, "NativeMath", "magic_add", native_math_magic_add_func);
}

void test_pkg_4_native_lib_init(NativeLib *lib) { kl_reg_meth(lib, "Foo", "swim", foo_swim_func); }

void test_static_methods_native_lib_init(NativeLib *lib)
{
    kl_reg_meth(lib, "NativeUtil", "magic_mul", native_util_magic_mul_func);
}

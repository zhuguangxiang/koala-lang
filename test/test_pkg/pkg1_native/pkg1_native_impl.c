/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "object.h"

/* func test_pkg_foo(an Animal) */
TValue test_pkg_foo_func(TValue *self, TValue *args, int nargs)
{
    // an.fly("Eagle", 3)
    Object *fn = kl_get_intf_func(args, 0);
    TValue _args[] = { args[0], obj_value(kl_new_str("Eagle")), int64_value(3) };
    printf("test_pkg_foo_func is called\n");
    return kl_object_call(fn, _args, 3);
}

// func swim(name str, age int) str
TValue foo_swim_func(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);

    TValue *name = &args[0];
    TValue *age = &args[1];

    ASSERT(kl_typeof(name) == &str_type);
    ASSERT(kl_typeof(age) == &int_type);

    char *_name = STR_BUF(to_obj(name));
    int _age = to_int64(age);

    Object *s = kl_new_fmt_str("<%s is swimming, age %d>", _name, _age);
    return obj_value(s);
}

void pkg1_native_module_init(NativeModule *m)
{
    kl_register_func(m, "test_pkg_foo", test_pkg_foo_func);
    kl_register_method(m, "Foo", "swim", foo_swim_func);
}

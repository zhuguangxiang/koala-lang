/*===-- init.c - Koala Virtual Machine Initializer ----------------*- C -*-===*\
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) 2020 James, https://github.com/zhuguangxiang                 *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* This file is the initializer of koala virtual machine                      *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "gc.h"
#include "object.h"
#include "task.h"

HashMap *modules;

extern HashMap *modules;
extern TypeObject *type_type;
extern TypeObject *any_type;
extern TypeObject *method_type;
extern TypeObject *field_type;
extern TypeObject *module_type;

static void init_root_module(void)
{
    /* init types */
    init_core_types();
    /*
    init_byte_type();
    init_integer_type();
    init_float_type();
    init_bool_type();
    init_char_type();
    init_string_type();
    init_array_type();
    init_tuple_type();
    init_map_type();
    init_code_type();
    init_label_type();
    init_range_type();
    init_iter_type();
    init_closure_type();
    init_result_type();
    init_error_type();
    init_option_type();
    */

    /* add core types into root module */
    Object *obj = module_new("/");
    module_add_type(obj, type_type);
    module_add_type(obj, any_type);
    module_add_type(obj, method_type);
    module_add_type(obj, field_type);
    module_add_type(obj, module_type);

    /* install root module */
    mtbl_add(modules, "/", obj);
}

static void init_io_module(void)
{
    /* add types into io module */
    Object *obj = module_new("io");
    /* install io module */
    mtbl_add(modules, "io", obj);
}

static void init_fs_module(void)
{
    /* add types into fs module */
    Object *obj = module_new("fs");
    /* install fs module */
    mtbl_add(modules, "fs", obj);
}

void koala_init(void)
{
    /* init strtab */

    /* init gc */
    gc_init();

    /* init modules */
    modules = mtbl_create();
    init_root_module();
    init_io_module();
    init_fs_module();

    /* init coroutine */
    init_procs(1);
}

void koala_fini(void)
{
    /* fini coroutine */
    fini_procs();

    /* fini gc */
    gc_fini();

    /* fini modules */
    mtbl_destroy(modules);
}

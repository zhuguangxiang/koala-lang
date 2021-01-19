/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangXiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "module.h"

#ifdef __cplusplus
extern "C" {
#endif

static HashMap *modmap;
static Object *root;

/* `Module` object layout. */
typedef struct ModuleObject {
    /* object */
    OBJECT_HEAD
    int size;
    void **values;
} ModuleObject;

#define MODULE_MIN_VALUES_SIZE (16 * 32)

Object *module_new(char *path)
{
    TypeObject *type = type_new_mod(path);
    type_set_public_final(type);
    ModuleObject *mobj = mm_alloc(sizeof(ModuleObject));
    obj_set_type(mobj, type);
    type_set_singleton(type, mobj);
    mobj->size = MODULE_MIN_VALUES_SIZE;
    mobj->values = mm_alloc(MODULE_MIN_VALUES_SIZE);
    return (Object *)type;
}

int module_add_type(Object *mod, TypeObject *type)
{
    TypeObject *_type = (TypeObject *)mod;
    if (!type_is_mod(_type)) {
        printf("error: object '%s' is not module\n", _type->name);
        abort();
    }

    printf("add type '%s' into module '%s'\n", type->name, _type->name);
    return type_add_type(_type, type);
}

int module_add_var(Object *mod, FieldObject *field)
{
    TypeObject *_type = (TypeObject *)mod;
    if (!type_is_mod(_type)) {
        printf("error: object '%s' is not module\n", _type->name);
        abort();
    }

    printf("add field '%s' into module '%s'\n", field->name, _type->name);

    ModuleObject *mobj = (ModuleObject *)_type->instance;
    if (_type->next_offset >= mobj->size) {
        printf("expand module '%s' values' memory\n", _type->name);
        int new_size = mobj->size << 1;
        void **new_values = mm_alloc(new_size);
        memcpy(new_values, mobj->values, mobj->size);
        mm_free(mobj->values);
        mobj->values = new_values;
        mobj->size = new_size;
    }

    field->offset = _type->next_offset;
    _type->next_offset += field->size;
    return type_add_field(_type, field);
}

int module_add_func(Object *mod, MethodObject *meth)
{
    TypeObject *_type = (TypeObject *)mod;
    if (!type_is_mod(_type)) {
        printf("error: object '%s' is not module\n", _type->name);
        abort();
    }

    printf("add method '%s' into module '%s'\n", meth->name, _type->name);
    return type_add_method(_type, meth);
}

void module_show(Object *mod)
{
    TypeObject *_type = (TypeObject *)mod;
    if (!type_is_mod(_type)) {
        printf("error: object '%s' is not module\n", _type->name);
        abort();
    }

    printf("module '%s':\n", _type->name);
    type_show(_type);
}

extern TypeObject *type_type;
extern TypeObject *any_type;
extern TypeObject *method_type;
extern TypeObject *field_type;

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

    Object *mod = module_new("/");

    /* add types */
    module_add_type(mod, type_type);
    module_add_type(mod, any_type);
    module_add_type(mod, method_type);
    module_add_type(mod, field_type);

    /* add functions */

    /* install root module */
    modmap = mtbl_create();
    mtbl_add(modmap, "/", mod);
    root = mod;
}

static void init_sys_module(void)
{
    Object *mod = module_new("sys");

    /* add types */

    /* add functions */

    /* install sys module */
    mtbl_add(modmap, "sys", mod);
}

static void init_io_module(void)
{
    Object *mod = module_new("io");

    /* add types */

    /* add functions */

    /* install io module */
    mtbl_add(modmap, "io", mod);
}

static void init_fs_module(void)
{
    Object *mod = module_new("fs");

    /* add types */

    /* add functions */

    /* install fs module */
    mtbl_add(modmap, "fs", mod);
}

void init_core_modules(void)
{
    /* init root module */
    init_root_module();

    /* init sys module */
    init_sys_module();

    /* init io module */
    init_io_module();

    /* init fs module */
    init_fs_module();
}

void fini_modules(void)
{
    mtbl_destroy(modmap);
}

void gc_trace_modules(void)
{
}

#ifdef __cplusplus
}
#endif

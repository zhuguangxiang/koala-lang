/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "core/typedesc.h"
#include "util/atom.h"

#ifdef __cplusplus
extern "C" {
#endif

void test_proto(void)
{
    TypeDesc *proto = kl_proto_from_str("Lio.File;si32", "b");
    Vector *params = kl_get_proto_params(proto);
    TypeDesc *type = null;

    vector_get(params, 0, &type);
    assert(type->kind == TYPE_KLASS_KIND);
    char *path = kl_get_klass_path(type);
    char *name = kl_get_klass_name(type);
    assert(!strcmp(path, "io"));
    assert(!strcmp(name, "File"));

    type = null;
    vector_get(params, 1, &type);
    assert(type->kind == TYPE_STR_KIND);

    type = null;
    vector_get(params, 2, &type);
    assert(type->kind == TYPE_I32_KIND);

    type = null;
    vector_get(params, 3, &type);
    assert(!type);

    type = kl_get_proto_ret(proto);
    assert(type->kind == TYPE_BOOL_KIND);
}

void test_array(void)
{
    TypeDesc *proto = kl_proto_from_str("Lio.File;s[i64", "b");
    Vector *params = kl_get_proto_params(proto);
    TypeDesc *type = null;

    vector_get(params, 0, &type);
    assert(type->kind == TYPE_KLASS_KIND);
    char *path = kl_get_klass_path(type);
    char *name = kl_get_klass_name(type);
    assert(!strcmp(path, "io"));
    assert(!strcmp(name, "File"));

    type = null;
    vector_get(params, 1, &type);
    assert(type->kind == TYPE_STR_KIND);

    type = null;
    vector_get(params, 2, &type);
    assert(type->kind == TYPE_ARRAY_KIND);
    type = kl_get_array_sub(type);
    assert(type->kind == TYPE_I64_KIND);

    type = null;
    vector_get(params, 3, &type);
    assert(!type);

    type = kl_get_proto_ret(proto);
    assert(type->kind == TYPE_BOOL_KIND);
}

void test_map(void)
{
}

void test_klass(void)
{
}

int main(int argc, char *argv[])
{
    init_atom();

    kl_init_desc();

    test_proto();
    test_array();
    test_map();
    test_klass();

    kl_fini_desc();

    fini_atom();

    return 0;
}

#ifdef __cplusplus
}
#endif

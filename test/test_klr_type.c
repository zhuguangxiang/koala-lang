/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 */

#include "klr/klr.h"
#include "util/atom.h"

#ifdef __cplusplus
extern "C" {
#endif

int main(int argc, char *argv[])
{
    init_atom();

    klr_init_types();

    KlrTypeRef proto = klr_proto_from_str("Lio.File;si2", "b");
    Vector *params = klr_get_params(proto);
    KlrTypeRef type = NULL;
    vector_get(params, 0, &type);
    assert(type->kind == KLR_TYPE_KLASS);
    char *path = klr_get_path(type);
    char *name = klr_get_name(type);
    assert(!strcmp(path, "io"));
    assert(!strcmp(name, "File"));

    type = NULL;
    vector_get(params, 1, &type);
    assert(type->kind == KLR_TYPE_STR);

    type = NULL;
    vector_get(params, 2, &type);
    assert(type->kind == KLR_TYPE_INT16);

    type = NULL;
    vector_get(params, 3, &type);
    assert(!type);

    type = klr_get_ret(proto);
    assert(type->kind == KLR_TYPE_BOOL);

    klr_fini_types();

    fini_atom();

    return 0;
}

#ifdef __cplusplus
}
#endif

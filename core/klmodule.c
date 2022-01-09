/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "vm.h"

#ifdef __cplusplus
extern "C" {
#endif

static HashMap gmodules;
static Vector gvalues;

static KlTypeInfo root_pkg = {
    .name = "/",
    .flags = TP_PKG | TP_FINAL,
};

KlTypeInfo *pkg_lookup(char *path)
{
    MNode key = { .name = path };
    hashmap_entry_init(&key, str_hash(path));
    return hashmap_get(&pkgs, &key);
}

void pkg_add_type(char *path, KlTypeInfo *type)
{
    KlTypeInfo *pkg = pkg_lookup(path);
    if (!pkg) {
        panic("cannot find '%s' package", path);
    }
    mtbl_add_type(type_get_mtbl(pkg), type);
}

static void init_pkgs(void)
{
    hashmap_init(&pkgs, mtbl_equal_func);
    hashmap_entry_init(&root_pkg, str_hash(root_pkg.name));
    hashmap_put_absent(&pkgs, &root_pkg);
}

static void init_root_pkg(void)
{
    type_ready(&root_pkg);
    root_pkg.instance.vtbl = KL_GC_VTBL(root_pkg.vtbl[0]);
    assert(root_pkg.instance.vtbl->type == &root_pkg);
    type_show(&root_pkg);
}

INIT_LEVEL_0(init_pkgs);
INIT_LEVEL_2(init_root_pkg);

#ifdef __cplusplus
}
#endif

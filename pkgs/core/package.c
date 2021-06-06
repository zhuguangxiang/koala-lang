/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 */

#include "typeinfo.h"
#include "util/hash.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _Package Package;

struct _Package {
    HashMapEntry entry;
    char *path;
    HashMap *map;
};

static HashMap packages;

static int pkg_equal(void *e1, void *e2)
{
    Package *pkg1 = e1;
    Package *pkg2 = e2;
    return !strcmp(pkg1->path, pkg2->path);
}

static Package *__get_pkg(char *path)
{
    Package key = { .path = path };
    hashmap_entry_init(&key, str_hash(path));
    return hashmap_get(&packages, &key);
}

void pkg_add_type(char *path, TypeInfo *type)
{
    Package *pkg = __get_pkg(path);
    MbrInfo *mbr = mbr_new_type(type);
    hashmap_put_absent(pkg->map, mbr);
}

void pkg_add_gvar(char *path, char *name)
{
    Package *pkg = __get_pkg(path);
    MbrInfo *mbr = mbr_new_gvar(name);
    hashmap_put_absent(pkg->map, mbr);
}

void pkg_add_cfunc(char *path, char *name, void *cfunc)
{
    Package *pkg = __get_pkg(path);
    MbrInfo *mbr = mbr_new_cfunc(name, cfunc);
    hashmap_put_absent(pkg->map, mbr);
}

void pkg_add_kfunc(char *path, char *name, CodeInfo *code)
{
    Package *pkg = __get_pkg(path);
    MbrInfo *mbr = mbr_new_kfunc(name, code);
    hashmap_put_absent(pkg->map, mbr);
}

void init_any_type(void);
void init_class_type(void);
void init_field_type(void);
void init_method_type(void);
void init_string_type(void);
void init_array_type(void);
// void init_map_type(void);

static void init_types(void)
{
    init_any_type();
    init_class_type();
    init_field_type();
    init_method_type();
    init_string_type();
    init_array_type();
    init_map_type();
}

void init_core_pkg(void)
{
    hashmap_init(&packages, pkg_equal);

    Package *pkg = mm_alloc_obj(pkg);
    pkg->path = "/";
    hashmap_entry_init(pkg, "/");
    pkg->map = mbr_map_new();
    hashmap_put_absent(&packages, pkg);

    init_types();
}

#ifdef __cplusplus
}
#endif

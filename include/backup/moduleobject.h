/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_MODULE_OBJECT_H_
#define _KOALA_MODULE_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*InitModuleFunc)(Object *m);
typedef void (*FiniModuleFunc)(Object *m);

typedef struct _ModuleDef {
    char *name;
    /* extend state size */
    size_t size;
    /* global variables */
    MemberDef *globals;
    /* global functions */
    MethodDef *methods;
    /* init function(extend state) */
    InitModuleFunc init;
    /* fini function(extend state) */
    FiniModuleFunc fini;
} ModuleDef;

typedef struct _ModuleObject {
    OBJECT_HEAD
    /* module definition */
    ModuleDef *def;
    /* extend state */
    void *state;
    /* init function */
    Object *__init__;
    /* all symbols(globals & functions) */
    Vector symbols;
    /* symbol map for link */
    HashMap map;
    /* constants */
    Vector consts;
    /* relocations for external symbols */
    Vector rels;
} ModuleObject;

typedef struct _RelocEntry {
    /* key: <path>/<symbol> */
    char *key;
    /* kind */
    int kind;
#define REL_TYPE_MODULE 1
#define REL_TYPE_KLASS  2
#define REL_TYPE_FUNC   3
#define REL_TYPE_VAR    4
    /* parent */
    int parent;
    /* object */
    Object *obj;
} RelocEntry;

extern TypeObject module_type;
#define IS_MODULE(ob) IS_TYPE((ob), &module_type)

Object *kl_new_module(char *name);
Object *kl_module_from_moddef(ModuleDef *def);

// int module_add_member(Object *_m, MemberDef *member);
// int module_add_getset(Object *_m, GetSetDef *getset);
int module_add_cfunc(Object *m, MethodDef *def);
int module_add_obj(Object *_m, char *name, Object *obj);

static inline char *module_get_name(Object *_m)
{
    ModuleObject *m = (ModuleObject *)_m;
    return m->def->name;
}

Object *module_lookup(Object *_m, char *name, int len);

int cp_add_int(Object *_m, int64_t val);
int cp_add_str(Object *_m, char *s);
int cp_add_obj(Object *_m, Object *obj);

int kl_do_link(Object *_m);

int kl_add_rel(Object *_m, int kind, char *key, int parent);

static inline int kl_add_rel_mod(Object *_m, char *path)
{
    return kl_add_rel(_m, REL_TYPE_MODULE, path, 0);
}

static inline int kl_add_rel_cls(Object *_m, char *cls, int parent)
{
    return kl_add_rel(_m, REL_TYPE_KLASS, cls, parent);
}

static inline int kl_add_rel_func(Object *_m, char *func, int parent)
{
    return kl_add_rel(_m, REL_TYPE_FUNC, func, parent);
}

static inline int kl_add_rel_var(Object *_m, char *var, int parent)
{
    return kl_add_rel(_m, REL_TYPE_VAR, var, parent);
}

static inline RelocEntry *kl_get_rel(Object *_m, int index)
{
    ModuleObject *m = (ModuleObject *)_m;
    RelocEntry *rel = vector_get_ptr(&m->rels, index);
    return rel;
}

static inline Object *kl_rel_get_obj(Object *_m, int index)
{
    RelocEntry *rel = kl_get_rel(_m, index);
    ASSERT(rel);
    return rel->obj;
}

Object *kl_load_module(char *path);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_MODULE_OBJECT_H_ */

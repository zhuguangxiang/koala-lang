/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_MODULE_H_
#define _KOALA_MODULE_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*InitModuleFunc)(Object *m);
typedef void (*FiniModuleFunc)(Object *m);

typedef struct _ModuleDef {
    /* module path */
    char *path;
    /* extended state size */
    int size;
    /* global variables */
    int nvars;
    /* global functions */
    MethodDef *funcs;
    /* types */
    TypeObject **types;
    /* init function(extended state) */
    InitModuleFunc init;
    /* fini function(extended state) */
    FiniModuleFunc fini;
} ModuleDef;

typedef struct _ModuleObject {
    OBJECT_HEAD

    /* hot zone */
    uint32_t *codes;     // code base
    uint32_t num_codes;  // number of codes
    uint32_t num_values; // number of global values
    TValue *values;      // global values
    Vector func_entries; // func entry array
    Vector const_pool;   // constant pool
    Vector import_table; // import table(wasm)

    /* cold zone */
    Object *__init__; // init func of koala
    Object *main;     // main func of koala
    Vector funcs;     // functions of this module
    Vector types;     // types defined in this module
    // Vector globals;    // vars defined in this module
    HashMap symbols;  // symbols for exported map
    char *path;       // module path
    Object *not_impl; // not implemented function

    /* native module */
    ModuleDef *def; // module defined by c extension
    void *state;    // module private pointer
} ModuleObject;

typedef enum {
    IMPORT_KIND_FUNC,   /* free function */
    IMPORT_KIND_GLOBAL, /* global variable */
    IMPORT_KIND_TYPE,   /* type object */
    IMPORT_KIND_METHOD, /* method of a type (slot-based) */
    IMPORT_KIND_FIELD   /* field of a type (offset-based) */
} ImportKind;

typedef struct _ImportEntry {
    ImportKind kind; /* what this import represents */
    char *path;      /* module path string */
    char *name;      /* symbol name string */
    void *address;   /* resolved runtime address */
} ImportEntry;

// module->funcs, cache-line 64
typedef struct _FuncEntry {
    Object *obj;
} FuncEntry;

extern TypeObject module_type;

#define IS_MODULE(ob) IS_TYPE((ob), &module_type)

Object *kl_new_module(char *path);
Object *kl_new_native_module(ModuleDef *def);
void kl_free_module(Object *m);
int kl_init_module(Object *_m);
#define kl_mo_path(m) (((ModuleObject *)(m))->path)
void kl_mo_set_code(Object *_m, uint32_t *insns, size_t n);
int kl_bind_func(Object *_m, Object *obj);
int kl_mo_add_func(Object *_m, Object *obj);
int kl_mo_add_type(Object *_m, TypeObject *tp);
int kl_mo_add_const(Object *_m, TValue *val);
int kl_mo_add_str(Object *_m, char *s);
int kl_mo_add_tuple(Object *_m, Vector *list);
int kl_mo_add_int(Object *_m, int64_t k, int type_info);
int kl_mo_add_uint(Object *_m, uint64_t k, int type_info);
int kl_mo_add_float(Object *_m, double k, int type_info);
int kl_mo_add_import(Object *_m, ImportKind kind, char *path, char *name);
Object *kl_mo_find(Object *_m, char *name);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_MODULE_H_ */

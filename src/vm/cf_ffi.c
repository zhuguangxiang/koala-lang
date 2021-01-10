/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangxiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "cf_ffi.h"

#ifdef __cplusplus
extern "C" {
#endif

static struct basemap {
    char kind;
    char *str;
    ffi_type *type;
} basemaps[] = {
    { TYPE_BYTE, "byte", &ffi_type_sint8 },
    { TYPE_INT, "int", &ffi_type_sint64 },
    { TYPE_FLOAT, "float", &ffi_type_double },
    { TYPE_BOOL, "bool", &ffi_type_sint8 },
    { TYPE_CHAR, "char", &ffi_type_sint32 },
    { TYPE_STR, "string", &ffi_type_pointer },
    { TYPE_ANY, "any", &ffi_type_pointer },
    { 0, NULL, NULL },
};

static ffi_type *map_ffi_type(TypeDesc *type)
{
    if (!type) return &ffi_type_void;
    struct basemap *map = basemaps;
    while (map->kind) {
        if (map->kind == type->kind) return map->type;
        ++map;
    }
    abort();
    return NULL;
}

cfunc_t *kl_new_cfunc(TypeDesc *desc, void *ptr)
{
    ProtoDesc *proto = (ProtoDesc *)desc;
    Vector *ptypes = proto->ptypes;
    int nargs = vector_size(ptypes);
    cfunc_t *cf = mm_alloc(sizeof(cfunc_t) + nargs * sizeof(void *));

    /* parameter types */
    TypeDesc *type;
    for (int i = 0; i < nargs; i++) {
        type = vector_get_ptr(ptypes, i);
        cf->ptypes[i] = map_ffi_type(type);
    }
    /* return type */
    cf->rtype = map_ffi_type(proto->rtype);

    /* function ptr */
    cf->ptr = ptr;
    ffi_prep_cif(&cf->cif, FFI_DEFAULT_ABI, nargs, cf->rtype, cf->ptypes);
    return cf;
}

void kl_call_cfunc(cfunc_t *cf, TValueRef *args, int narg, intptr_t *ret)
{
    if (narg > 0) {
        void *aval[narg];
        for (int i = 0; i < narg; i++) { aval[i] = &((args + i)->_v); }
        if (cf->rtype != &ffi_type_void) {
            ffi_call(&cf->cif, cf->ptr, ret, aval);
        }
        else {
            ffi_call(&cf->cif, cf->ptr, NULL, aval);
        }
    }
    else {
        if (cf->rtype != &ffi_type_void) {
            ffi_call(&cf->cif, cf->ptr, ret, NULL);
        }
        else {
            ffi_call(&cf->cif, cf->ptr, NULL, NULL);
        }
    }
}

#ifdef __cplusplus
}
#endif

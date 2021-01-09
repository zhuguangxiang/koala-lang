/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangxiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "typedesc.h"

#ifdef __cplusplus
extern "C" {
#endif

TypeDesc kl_type_byte = { TYPE_BYTE };
TypeDesc kl_type_int = { TYPE_INT };
TypeDesc kl_type_float = { TYPE_FLOAT };
TypeDesc kl_type_bool = { TYPE_BOOL };
TypeDesc kl_type_char = { TYPE_CHAR };
TypeDesc kl_type_str = { TYPE_STR };
TypeDesc kl_type_any = { TYPE_ANY };

static struct basemap {
    TypeDesc *type;
    char *str;
} basemaps[] = {
    { &kl_type_byte, "byte" },
    { &kl_type_int, "int" },
    { &kl_type_float, "float" },
    { &kl_type_bool, "bool" },
    { &kl_type_char, "char" },
    { &kl_type_str, "string" },
    { &kl_type_any, "any" },
    { NULL, NULL },
};

char *base_type_str(char kind)
{
    struct basemap *map = basemaps;
    while (map->type) {
        if (map->type->kind == kind) return map->str;
        ++map;
    }
    abort();
    return NULL;
}

static TypeDesc *base_type(char kind)
{
    struct basemap *map = basemaps;
    while (map->type) {
        if (map->type->kind == kind) return map->type;
        ++map;
    }
    abort();
    return NULL;
}

TypeDesc *desc_from_proto(Vector *ptypes, TypeDesc *rtype)
{
    ProtoDesc *desc = mm_alloc(sizeof(ProtoDesc));
    desc->kind = TYPE_PROTO;
    desc->ptypes = ptypes;
    desc->rtype = rtype;
    return (TypeDesc *)desc;
}

static TypeDesc *__to_desc(char **str)
{
    char *s = *str;
    char ch = *s;
    TypeDesc *desc;

    switch (ch) {
        case 'L': {
            break;
        }
        case '(': {
            break;
        }
        case '<': {
            break;
        }
        case '.': {
            break;
        }
        case '[': {
            break;
        }
        case 'M': {
            break;
        }
        case 'P': {
            break;
        }
        default: {
            desc = base_type(ch);
            s++;
            break;
        }
    }
    *str = s;
    return desc;
}

TypeDesc *to_desc(char *s)
{
    if (!s) return NULL;
    return __to_desc(&s);
}

TypeDesc *to_proto(char *para, char *ret)
{
    Vector *ptypes = NULL;
    if (para) {
        ptypes = vector_new(sizeof(void *));
        char *s = para;
        TypeDesc *desc;
        while (*s) {
            desc = __to_desc(&s);
            vector_push_back(ptypes, desc);
        }
    }

    TypeDesc *rtype = NULL;
    if (ret) {
        char *s = ret;
        rtype = __to_desc(&s);
    }

    return desc_from_proto(ptypes, rtype);
}

#ifdef __cplusplus
}
#endif

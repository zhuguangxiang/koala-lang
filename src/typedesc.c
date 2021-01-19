/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangXiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "typedesc.h"

#ifdef __cplusplus
extern "C" {
#endif

TypeDesc kl_type_int8 = { TYPE_INT8 };
TypeDesc kl_type_int16 = { TYPE_INT16 };
TypeDesc kl_type_int32 = { TYPE_INT32 };
TypeDesc kl_type_int64 = { TYPE_INT64 };
TypeDesc kl_type_int = { TYPE_INT64 };
TypeDesc kl_type_float32 = { TYPE_FLOAT32 };
TypeDesc kl_type_float64 = { TYPE_FLOAT64 };
TypeDesc kl_type_float = { TYPE_FLOAT64 };
TypeDesc kl_type_bool = { TYPE_BOOL };
TypeDesc kl_type_char = { TYPE_CHAR };
TypeDesc kl_type_str = { TYPE_STRING };
TypeDesc kl_type_any = { TYPE_ANY };
TypeDesc kl_type_array = { TYPE_ARRAY };
TypeDesc kl_type_dict = { TYPE_DICT };
TypeDesc kl_type_tuple = { TYPE_TUPLE };
TypeDesc kl_type_valist = { TYPE_VALIST };
TypeDesc kl_type_klass = { TYPE_KLASS };
TypeDesc kl_type_proto = { TYPE_PROTO };

static struct basemap {
    TypeDesc *type;
    char *str;
} basemaps[] = {
    { &kl_type_int, "int" },
    { &kl_type_int8, "int8" },
    { &kl_type_int16, "int16" },
    { &kl_type_int32, "int32" },
    { &kl_type_int64, "int64" },
    { &kl_type_float, "float" },
    { &kl_type_float32, "float32" },
    { &kl_type_float64, "float64" },
    { &kl_type_bool, "bool" },
    { &kl_type_char, "char" },
    { &kl_type_str, "string" },
    { &kl_type_any, "any" },
    { NULL, NULL },
};

TypeDesc *base_desc(uint8_t kind)
{
    struct basemap *map = basemaps;
    while (map->type) {
        if (map->type->kind == kind) return map->type;
        ++map;
    }
    abort();
    return NULL;
}

char *base_desc_str(uint8_t kind)
{
    struct basemap *map = basemaps;
    while (map->type) {
        if (map->type->kind == kind) return map->str;
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
    TypeDesc *t1, *t2;

    switch (ch) {
        case 'i': {
            desc = &kl_type_int;
            s++;
            break;
        }
        case 'z': {
            desc = &kl_type_bool;
            s++;
            break;
        }
        case 's': {
            desc = &kl_type_str;
            s++;
            break;
        }
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
            s++;
            Vector *args = NULL;
            if (*s != ':') {
                args = vector_new(sizeof(TypeDesc *));
                while (*s != ':') {
                    t1 = __to_desc(&s);
                    vector_push_back(args, &t1);
                }
            }
            s++;
            t1 = __to_desc(&s);
            desc = desc_from_proto(args, t1);
            break;
        }
        case 0: {
            desc = NULL;
            break;
        }
        default: {
            abort();
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
            vector_push_back(ptypes, &desc);
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

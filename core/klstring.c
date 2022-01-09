/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "gc.h"
#include "kltypes.h"

#ifdef __cplusplus
extern "C" {
#endif

KlType string_type = {
    .name = "string",
    .flags = TP_CLASS | TP_FINAL,
    .gc = 1,
};

KlFuncTbl string_vtbl = {
    &string_type,
    0,
};

static int string_gcmap[] = {
    1,
    offsetof(KlString, data),
};

KlValue kl_string_new(char *str)
{
    char *data = null;
    char **wrap = null;
    KlString *s = null;

    KL_GC_STACK(3);
    kl_gc_push(&data, 0);
    kl_gc_push(&wrap, 1);
    kl_gc_push(&s, 2);

    int len = strlen(str);
    data = kl_gc_alloc_raw(len + 1);
    wrap = kl_gc_alloc_wrap();
    *wrap = data;
    s = kl_gc_alloc_struct(sizeof(*s), string_gcmap);
    s->data = wrap;
    s->cap = len + 1;
    s->start = 0;
    s->end = len;
    memcpy(data, str, len);
    kl_gc_pop();

    return (KlValue){ .obj = s, &string_vtbl };
}

KlValue kl_string_substring(KlValue *val, int start, int len)
{
    KlString *s = null;

    KL_GC_STACK(2);
    kl_gc_push(&s, 0);
    kl_gc_push(&val->obj, 1);

    s = kl_gc_alloc_struct(sizeof(*s), string_gcmap);
    s->data = ((KlString *)val->obj)->data;
    s->cap = ((KlString *)val->obj)->cap;
    s->start = ((KlString *)val->obj)->start + start;
    s->end = s->start + len;

    kl_gc_pop();
    return (KlValue){ .obj = s, &string_vtbl };
}

int kl_string_push(KlValue *val, int ch)
{
    KlString *s = val->obj;

    KL_GC_STACK(2);
    kl_gc_push(&s, 0);
    kl_gc_push(&val->obj, 1);

    if (s->end + 1 >= s->cap) {
        printf("expand memory\n");
        char *data = kl_gc_alloc_raw(s->cap * 2);
        memcpy(data, *s->data, s->end);
        *s->data = data;
        s->cap *= 2;
    }

    (*s->data)[s->end] = (char)ch;
    ++s->end;

    kl_gc_pop();
    return 0;
}

void init_string_type(void)
{
    /*
    MethodDef methods[] = {
        { "push", "c", null, kl_string_push },
        { null },
    };
    */

    // type_add_methdefs(&string_type, methods);
    // type_ready(&string_type);
    // string_vtbl = string_type.vtbl;
    // pkg_add_type("/", &string_type);
}

/*
KlTypeInfo char_type = {
    .name = "char",
    .flags = TP_CLASS | TP_FINAL,
};

void init_char_type(void)
{
    type_ready(&char_type);
    pkg_add_type("/", &char_type);
}

INIT_LEVEL_2(init_string_type);
INIT_LEVEL_2(init_char_type);
*/

#ifdef __cplusplus
}
#endif

/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangxiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "klc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct str_entry {
    HashMapEntry entry;
    int idx;
    klc_str_t s;
} str_entry_t;

static int str_entry_equal(void *k1, void *k2)
{
    str_entry_t *e1 = k1;
    str_entry_t *e2 = k2;
    return e1->s.len == e2->s.len && !strcmp(e1->s.s, e2->s.s);
}

static int klc_str_append(klc_file_t *klc, char *s)
{
    klc_str_t str = { strlen(s), s };
    Vector *vec = &klc->strs.vec;
    int idx = vector_size(vec);
    vector_push_back(vec, &str);
    klc->strs.size += sizeof(uint32_t) + str.len + 1;

    str_entry_t *e = mm_alloc(sizeof(*e));
    hashmap_entry_init(e, strhash(s));
    e->idx = idx;
    e->s = str;
    hashmap_put_absent(&klc->strtab, e);

    return idx;
}

static int klc_str_get(klc_file_t *klc, char *s)
{
    str_entry_t key = { .s = { strlen(s), s } };
    hashmap_entry_init(&key, strhash(s));
    str_entry_t *e = hashmap_get(&klc->strtab, &key);
    return e ? e->idx : -1;
}

static int klc_str_set(klc_file_t *klc, char *s)
{
    int idx = klc_str_get(klc, s);
    if (idx >= 0) return idx;
    return klc_str_append(klc, s);
}

static int _isbase(int kind)
{
    return (kind == TYPE_BYTE || kind == TYPE_INT || kind == TYPE_FLOAT ||
        kind == TYPE_BOOL || kind == TYPE_CHAR || kind == TYPE_STR ||
        kind == TYPE_ANY);
}

static int klc_base_append(klc_file_t *klc, TypeDesc *type)
{
    Vector *vec = &klc->types.vec;
    int idx = vector_size(vec);
    klc_type_t *base = mm_alloc(sizeof(*base));
    base->kind = type->kind;
    vector_push_back(vec, &base);
    klc->types.size += 1;
    return idx;
}

static int klc_type_set(klc_file_t *klc, TypeDesc *type)
{
    if (_isbase(type->kind)) return klc_base_append(klc, type);
    abort();
}

void klc_add_var(klc_file_t *klc, char *name, TypeDesc *type)
{
    int name_idx = klc_str_set(klc, name);
    int type_idx = klc_type_set(klc, type);
    klc_var_t var = { name_idx, type_idx };
    klc_sect_t *vars = &klc->vars;
    vector_push_back(&vars->vec, &var);
    vars->size += sizeof(var);
}

#define VERSION_MAJOR 0
#define VERSION_MINOR 9
#define VERSION_BUILD 10

klc_file_t *klc_create(void)
{
    klc_file_t *klc = mm_alloc(sizeof(*klc));

    /* magic: klc */
    klc->magic[0] = 'k';
    klc->magic[1] = 'l';
    klc->magic[2] = 'c';
    klc->magic[3] = '\0';

    /* version: xx.xx.xx.xx */
    klc->version[0] = '0' + VERSION_MAJOR;
    klc->version[1] = '0' + VERSION_MINOR;
    klc->version[2] = '0' + ((VERSION_BUILD >> 8) & 0xFF);
    klc->version[3] = '0' + (VERSION_BUILD & 0xFF);

    /* init string table */
    hashmap_init(&klc->strtab, str_entry_equal);

    /* init sections */
    klc->strs.id = SECT_STRING_ID;
    klc->strs.size = sizeof(uint16_t);
    vector_init(&klc->strs.vec, sizeof(klc_str_t));

    klc->types.id = SECT_TYPE_ID;
    klc->types.size = sizeof(uint16_t);
    vector_init(&klc->types.vec, sizeof(klc_type_t *));

    klc->vars.id = SECT_VAR_ID;
    klc->vars.size = sizeof(uint16_t);
    vector_init(&klc->vars.vec, sizeof(klc_var_t));

    klc->funcs.id = SECT_FUNC_ID;
    klc->funcs.size = sizeof(uint16_t);
    vector_init(&klc->funcs.vec, sizeof(klc_func_t));

    klc->codes.id = SECT_CODE_ID;
    klc->codes.size = sizeof(uint16_t);
    vector_init(&klc->codes.vec, sizeof(klc_code_t *));

    return klc;
}

void klc_destroy(klc_file_t *klc)
{
    mm_free(klc);
}

static void klc_sec_show(klc_sect_t *sec)
{
    static char *sec_ids[SECT_MAX_ID] = { NULL, "string", "type", "var", "func",
        "code", "klass" };
    uint8_t id = sec->id;
    uint32_t size = sec->size;
    uint16_t count = vector_size(&sec->vec);
    printf("\n%s(size: %u, count: %u):\n", sec_ids[id], size, count);
}

static void klc_str_show(klc_file_t *klc)
{
    klc_sect_t *sec = &klc->strs;
    klc_sec_show(sec);
    klc_str_t *str;
    vector_foreach(str, &sec->vec)
    {
        printf("[%u]:\n", i);
        printf("  len: %u\n", str->len);
        printf("  str: %s\n", str->s);
    }
}

static void klc_type_show(klc_file_t *klc)
{
    klc_sect_t *sec = &klc->types;
    klc_sec_show(sec);
    klc_type_t **typep;
    klc_type_t *type;
    vector_foreach(typep, &sec->vec)
    {
        type = *typep;
        if (_isbase(type->kind)) {
            printf("[%u]: %s\n", i, base_type_str(type->kind));
        }
    }
}

static void klc_var_show(klc_file_t *klc)
{
    klc_sect_t *sec = &klc->vars;
    klc_sec_show(sec);
    Vector *strs = &klc->strs.vec;
    Vector *types = &klc->types.vec;
    char *typestr;
    klc_str_t *str;
    klc_type_t *t;
    klc_var_t *var;
    vector_foreach(var, &sec->vec)
    {
        str = vector_get_ptr(strs, var->name);
        vector_get(types, var->type, &t);
        if (_isbase(t->kind)) typestr = base_type_str(t->kind);
        printf("[%u]:\n", i);
        printf("  name-index: %u\n", var->name);
        printf("  (%s)\n", str->s);
        printf("  type-index: %u\n", var->type);
        printf("  (%s)\n", typestr);
    }
}

void klc_show(klc_file_t *klc)
{
    printf("\n-------------------------\n");
    printf("magic: %s\n", (char *)klc->magic);
    uint8_t major = klc->version[0] - '0';
    uint8_t minor = klc->version[1] - '0';
    uint16_t build = ((klc->version[2] - '0') << 8) + (klc->version[3] - '0');
    printf("version: %u.%u.%u\n", major, minor, build);
    klc_str_show(klc);
    klc_type_show(klc);
    klc_var_show(klc);
    printf("-------------------------\n");
}

#ifdef __cplusplus
}
#endif

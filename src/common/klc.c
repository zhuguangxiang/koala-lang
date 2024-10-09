/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "klc.h"
#include "buffer.h"
#include "version.h"

#ifdef __cplusplus
extern "C" {
#endif

#define KLC_TYPE_NONE   'N'
#define KLC_TYPE_FALSE  'F'
#define KLC_TYPE_TRUE   'T'
#define KLC_TYPE_INT    'i'
#define KLC_TYPE_FLOAT  'f'
#define KLC_TYPE_ASCII  'A'
#define KLC_TYPE_STRING 's'
#define KLC_TYPE_TUPLE  '('
#define KLC_TYPE_LIST   '['
#define KLC_TYPE_MAP    '{'
#define KLC_TYPE_SET    '<'
#define KLC_TYPE_CODE   'c'
#define KLC_TYPE_BYTES  'b'
#define KLC_TYPE_CLASS  'C'
#define KLC_TYPE_TRAIT  'I'
#define KLC_TYPE_VAR    'v'
#define KLC_TYPE_PROTO  'p'
#define KLC_TYPE_FUNC   'q'

/* with a type, add obj to index */
#define FLAG_REF '\x80'

#define KLC_TYPE_SHORT_ASCII 'a'
#define KLC_TYPE_SMALL_TUPLE ')'

typedef struct _KlcObject {
    int type;
    int len;
    union {
        int64_t ival;
        double f64val;
        void *val;
    };
} KlcObject;

void klc_add_int32(KlcFile *klc, int32_t val)
{
    KlcObject obj;
    obj.type = KLC_TYPE_INT;
    obj.len = 4;
    obj.ival = val;
    vector_push_back(&klc->objs, &obj);
}

void klc_add_bytes(KlcFile *klc, const char *insns, int insns_size)
{
    KlcObject obj;
    obj.type = KLC_TYPE_BYTES;
    obj.len = insns_size;
    obj.val = (void *)insns;
    vector_push_back(&klc->objs, &obj);
}

void klc_add_var(KlcFile *klc, const char *name, const char *desc)
{
    KlcObject obj;

    obj.type = KLC_TYPE_VAR;
    obj.len = 0;
    obj.val = NULL;
    vector_push_back(&klc->objs, &obj);

    int len = strlen(name);
    ASSERT(len <= 255);
    obj.type = KLC_TYPE_SHORT_ASCII;
    obj.len = len;
    obj.val = (void *)name;
    vector_push_back(&klc->objs, &obj);

    len = strlen(desc);
    if (len <= 255)
        obj.type = KLC_TYPE_SHORT_ASCII;
    else
        obj.type = KLC_TYPE_ASCII;
    obj.len = len;
    obj.val = (void *)desc;
    vector_push_back(&klc->objs, &obj);
}

void klc_add_func(KlcFile *klc, const char *name, const char *desc)
{
    KlcObject obj;

    obj.type = KLC_TYPE_FUNC;
    obj.len = 0;
    obj.val = NULL;
    vector_push_back(&klc->objs, &obj);

    int len = strlen(name);
    ASSERT(len <= 255);
    obj.type = KLC_TYPE_SHORT_ASCII;
    obj.len = len;
    obj.val = (void *)name;
    vector_push_back(&klc->objs, &obj);

    len = strlen(desc);
    if (len <= 255)
        obj.type = KLC_TYPE_SHORT_ASCII;
    else
        obj.type = KLC_TYPE_ASCII;
    obj.len = len;
    obj.val = (void *)desc;
    vector_push_back(&klc->objs, &obj);
}

void klc_add_code(KlcFile *klc, CodeSpec *cs)
{
    KlcObject obj = { 0 };
    obj.type = KLC_TYPE_CODE;
    obj.len = 0; // no length
    obj.val = cs;
    vector_push_back(&klc->objs, &obj);
}

static FILE *open_klc_file(const char *path, char *mode)
{
    FILE *fp = fopen(path, mode);
    if (fp == NULL) {
        char *end = strrchr(path, '/');
        if (!end) {
            panic("error: Cannot open '%s'.", path);
        }

        /* path contains directories, create them */
        BUF(cmd);
        buf_write_str(&cmd, "mkdir -p ");
        buf_write_nstr(&cmd, (char *)path, end - path);
        int status = system(BUF_STR(cmd));
        ASSERT(status == 0);
        FINI_BUF(cmd);

        /* reopen */
        fp = fopen(path, mode);
        if (fp == NULL) {
            panic("error: Cannot open '%s'.", path);
        }
    }
    return fp;
}

static int read_byte(KlcFile *klc)
{
    int val = 0;
    fread(&val, 1, 1, klc->filp);
    return val;
}

static int read_short(KlcFile *klc)
{
    int val = 0;
    fread(&val, 2, 1, klc->filp);
    return val;
}

static int read_int(KlcFile *klc)
{
    int val = 0;
    fread(&val, 4, 1, klc->filp);
    return val;
}

static char *read_ascii(int len, KlcFile *klc)
{
    char *s = mm_alloc_fast(len + 1);
    ASSERT(s);
    fread(s, len, 1, klc->filp);
    s[len] = '\0';
    return s;
}

#define save(obj) vector_push_back(&klc->objs, (obj))

static void read_object(KlcFile *klc)
{
    int code = read_byte(klc);
    int type = code & ~FLAG_REF;
    int flag = code & FLAG_REF;

    KlcObject obj = { 0 };
    obj.type = type;

    switch (type) {
        case KLC_TYPE_VAR: {
            save(&obj);
            read_object(klc);
            read_object(klc);
            break;
        }
        case KLC_TYPE_FUNC: {
            save(&obj);
            read_object(klc);
            read_object(klc);
            break;
        }
        case KLC_TYPE_SHORT_ASCII: {
            int len = read_byte(klc);
            char *s = read_ascii(len, klc);
            obj.len = len;
            obj.val = (void *)s;
            save(&obj);
            break;
        }
        // case KLC_TYPE_CODE: {
        //     int nargs = read_short(klc);
        //     int nlocals = read_short(klc);
        //     int stack_size = read_short(klc);
        //     KlcObject bytes = { 0 };
        //     read_object(&bytes, klc);
        //     CodeSpec *cs = mm_alloc_obj(cs);
        //     cs->nargs = nargs;
        //     cs->nlocals = nlocals;
        //     cs->stack_size = stack_size;
        //     cs->insns_size = bytes.len;
        //     cs->insns = bytes.val;
        //     obj->len = 0;
        //     obj->val = cs;
        //     break;
        // }
        case KLC_TYPE_INT: {
            NYI();
            break;
        }
        // case KLC_TYPE_BYTES: {
        //     int size = read_int(klc);
        //     char *buf = mm_alloc_fast(size + 1);
        //     fread(buf, 1, size, klc->filp);
        //     obj->len = size;
        //     obj->val = buf;
        //     break;
        // }
        default: {
            UNREACHABLE();
            break;
        }
    }
}

// for compiler, only_meta is true; for vm, only_meta is false.
static void read_objects(KlcFile *klc, int only_meta)
{
    int total = 0;

    total += klc->num_vars;
    total += klc->num_funcs;
    total += klc->num_types;

    if (!only_meta) {
        total += klc->num_relocs;
        total += klc->num_codes;
        total += klc->num_consts;
    }

    for (int i = 0; i < total; i++) {
        read_object(klc);
    }
}

static inline void read_check_magic(KlcFile *klc)
{
    uint8_t magic[4] = { 0 };
    fread(magic, sizeof(magic), 1, klc->filp);
    if (memcmp(magic, klc->magic, 4)) {
        panic("error: klc file magic check failed");
    }
}

static inline void read_check_version(KlcFile *klc)
{
    uint32_t version = 0;
    fread(&version, sizeof(version), 1, klc->filp);
    if (version > klc->version) {
        panic("error: klc file version check failed");
    }
}

static inline void read_numbers(KlcFile *klc)
{
    fread(&klc->num_vars, sizeof(uint8_t), 1, klc->filp);
    fread(&klc->num_funcs, sizeof(uint8_t), 1, klc->filp);
    fread(&klc->num_types, sizeof(uint8_t), 1, klc->filp);
    fread(&klc->num_relocs, sizeof(uint8_t), 1, klc->filp);
    fread(&klc->num_codes, sizeof(uint16_t), 1, klc->filp);
    fread(&klc->num_consts, sizeof(uint16_t), 1, klc->filp);
}

int read_klc_file(KlcFile *klc, int only_meta)
{
    FILE *fp = open_klc_file(klc->path, "r");
    klc->filp = fp;
    read_check_magic(klc);
    read_check_version(klc);
    read_numbers(klc);
    read_objects(klc, only_meta);
    fclose(fp);
    return 0;
}

CodeSpec *get_code_spec(KlcFile *klc)
{
    KlcObject *obj = vector_get(&klc->objs, 0);
    return obj->val;
}

static void write_object(KlcObject *obj, KlcFile *klc)
{
    FILE *fp = klc->filp;

    fwrite(&obj->type, 1, 1, fp);

    switch (obj->type) {
        case KLC_TYPE_CODE: {
            CodeSpec *cs = obj->val;
            fwrite(&cs->nargs, 2, 1, fp);
            fwrite(&cs->nlocals, 2, 1, fp);
            fwrite(&cs->stack_size, 2, 1, fp);
            KlcObject bytes = {
                .type = KLC_TYPE_BYTES,
                .len = cs->insns_size,
                .val = (void *)cs->insns,
            };
            write_object(&bytes, klc);
            break;
        }
        case KLC_TYPE_INT: {
            fwrite(&obj->len, 1, 1, fp);
            fwrite(&obj->ival, obj->len, 1, fp);
            break;
        }
        case KLC_TYPE_BYTES: {
            fwrite(&obj->len, 4, 1, fp);
            fwrite(obj->val, 1, obj->len, fp);
            break;
        }
        case KLC_TYPE_VAR:
        // fall-through
        case KLC_TYPE_FUNC:
        // fall-through
        case KLC_TYPE_PROTO: {
            // do nothing
            break;
        }
        case KLC_TYPE_SHORT_ASCII: {
            fwrite(&obj->len, 1, 1, fp);
            fwrite(obj->val, 1, obj->len, fp);
            break;
        }
        default: {
            UNREACHABLE();
            break;
        }
    }
}

static void write_objects(KlcFile *klc)
{
    KlcObject *obj;
    vector_foreach(obj, &klc->objs) {
        write_object(obj, klc);
    }
}

static inline void write_magic(KlcFile *klc)
{
    fwrite(klc->magic, sizeof(klc->magic), 1, klc->filp);
}

static inline void write_version(KlcFile *klc)
{
    fwrite(&klc->version, sizeof(klc->version), 1, klc->filp);
}

static inline void write_numbers(KlcFile *klc)
{
    fwrite(&klc->num_vars, sizeof(uint8_t), 1, klc->filp);
    fwrite(&klc->num_funcs, sizeof(uint8_t), 1, klc->filp);
    fwrite(&klc->num_types, sizeof(uint8_t), 1, klc->filp);
    fwrite(&klc->num_relocs, sizeof(uint8_t), 1, klc->filp);
    fwrite(&klc->num_codes, sizeof(uint16_t), 1, klc->filp);
    fwrite(&klc->num_consts, sizeof(uint16_t), 1, klc->filp);
}

int write_klc_file(KlcFile *klc)
{
    FILE *fp = open_klc_file(klc->path, "w");
    klc->filp = fp;
    write_magic(klc);
    write_version(klc);
    write_numbers(klc);
    write_objects(klc);
    fclose(fp);
    return 0;
}

void init_klc_file(KlcFile *klc, const char *path)
{
    memcpy(klc->magic, "klc", 4);
    uint32_t version =
        (KOALA_VERSION_MAJOR << 24) + (KOALA_VERSION_MINOR << 16) + KOALA_VERSION_PATCH;
    klc->version = version;
    klc->path = path;
    klc->filp = NULL;
    vector_init(&klc->objs, sizeof(KlcObject));
}

void fini_klc_file(KlcFile *klc) {}

void klc_dump(KlcFile *klc)
{
    printf("number of vars: %d\n", klc->num_vars);
    printf("number of funcs: %d\n", klc->num_funcs);

    int i = 0;
    while (i < vector_size(&klc->objs)) {
        KlcObject *v = vector_get(&klc->objs, i);
        switch (v->type) {
            case KLC_TYPE_VAR: {
                KlcObject *name = vector_get(&klc->objs, i + 1);
                KlcObject *desc = vector_get(&klc->objs, i + 2);
                printf("var %s:%s\n", (char *)name->val, (char *)desc->val);
                i += 3;
                break;
            }
            case KLC_TYPE_FUNC: {
                KlcObject *name = vector_get(&klc->objs, i + 1);
                KlcObject *desc = vector_get(&klc->objs, i + 2);
                printf("func %s:%s\n", (char *)name->val, (char *)desc->val);
                i += 3;
                break;
            }
            default: {
                UNREACHABLE();
                break;
            }
        }
    }
}

#ifdef __cplusplus
}
#endif

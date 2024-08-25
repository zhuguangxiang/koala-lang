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

#define KLC_TYPE_NONE    'N'
#define KLC_TYPE_FALSE   'F'
#define KLC_TYPE_TRUE    'T'
#define KLC_TYPE_INT     'i'
#define KLC_TYPE_FLOAT   'f'
#define KLC_TYPE_ASCII   'A'
#define KLC_TYPE_UNICODE 'U'
#define KLC_TYPE_TUPLE   '('
#define KLC_TYPE_LIST    '['
#define KLC_TYPE_MAP     '{'
#define KLC_TYPE_SET     '<'
#define KLC_TYPE_CODE    'c'
#define KLC_TYPE_BYTES   'b'
#define KLC_TYPE_DESCR   'L'
#define KLC_TYPE_CLASS   'C'
#define KLC_TYPE_TRAIT   'I'
#define KLC_TYPE_VAR     'v'
#define KLC_TYPE_PROTO   'p'
#define KLC_TYPE_NAME    'n'
#define KLC_TYPE_REF     'r'

/* with a type, add obj to index */
#define FLAG_REF '\x80'

#define TYPE_SHORT_ASCII 'a'
#define TYPE_SMALL_TUPLE ')'

typedef struct _KlcObject {
    int type;
    int len;
    union {
        int64_t ival;
        float f32val;
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

static void read_object(KlcObject *obj, KlcFile *klc)
{
    int code = read_byte(klc);
    int type = code & ~FLAG_REF;
    int flag = code & FLAG_REF;

    obj->type = type;

    switch (type) {
        case KLC_TYPE_CODE: {
            int nargs = read_short(klc);
            int nlocals = read_short(klc);
            int stack_size = read_short(klc);
            KlcObject bytes = { 0 };
            read_object(&bytes, klc);
            CodeSpec *cs = mm_alloc_obj(cs);
            cs->nargs = nargs;
            cs->nlocals = nlocals;
            cs->stack_size = stack_size;
            cs->insns_size = bytes.len;
            cs->insns = bytes.val;
            obj->len = 0;
            obj->val = cs;
            break;
        }
        case KLC_TYPE_INT: {
            NIY();
            break;
        }
        case KLC_TYPE_BYTES: {
            int size = read_int(klc);
            char *buf = mm_alloc_fast(size + 1);
            fread(buf, 1, size, klc->filp);
            obj->len = size;
            obj->val = buf;
            break;
        }
        default: {
            UNREACHABLE();
            break;
        }
    }
}

static void read_objects(KlcFile *klc)
{
    KlcObject obj;
    read_object(&obj, klc);
    vector_push_back(&klc->objs, &obj);
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

int read_klc_file(KlcFile *klc)
{
    FILE *fp = open_klc_file(klc->path, "r");
    klc->filp = fp;
    read_check_magic(klc);
    read_check_version(klc);
    read_objects(klc);
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

int write_klc_file(KlcFile *klc)
{
    FILE *fp = open_klc_file(klc->path, "w");
    klc->filp = fp;
    write_magic(klc);
    write_version(klc);
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

#ifdef __cplusplus
}
#endif

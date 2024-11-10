/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "klc.h"
#include "atom.h"
#include "buffer.h"
#include "version.h"

#ifdef __cplusplus
extern "C" {
#endif

void klc_add_none(KlcFile *klc)
{
    KlcObject obj;
    obj.type = KLC_TYPE_NONE;
    obj.len = 0;
    obj.ival = 0;
    vector_push_back(&klc->objs, &obj);
}

void klc_add_int(KlcFile *klc, int64_t val, int len)
{
    KlcObject obj;
    obj.type = KLC_TYPE_INT;
    obj.len = len;
    obj.ival = val;
    vector_push_back(&klc->objs, &obj);
}

void klc_add_float(KlcFile *klc, double val, int len)
{
    KlcObject obj;
    obj.type = KLC_TYPE_FLOAT;
    obj.len = len;
    obj.fval = val;
    vector_push_back(&klc->objs, &obj);
}

void klc_add_str(KlcFile *klc, char *s, int len)
{
    KlcObject obj;
    if (len <= 255) {
        obj.type = KLC_TYPE_SHORT_ASCII;
    } else {
        obj.type = KLC_TYPE_ASCII;
    }
    obj.len = len;
    obj.val = atom_nstr(s, len);
    vector_push_back(&klc->objs, &obj);
}

void klc_add_utf8(KlcFile *klc, char *s, int len)
{
    KlcObject obj;
    if (len <= 255) {
        obj.type = KLC_TYPE_SHORT_UTF8;
    } else {
        obj.type = KLC_TYPE_UTF8;
    }
    obj.len = len;
    obj.val = (void *)s;
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

void klc_add_var(KlcFile *klc, char *name, char *desc, int has_value, int flags)
{
    KlcObject obj;

    if (has_value) {
        obj.type = KLC_TYPE_VAR_VAL;
    } else {
        obj.type = KLC_TYPE_VAR;
    }
    obj.len = 0;
    obj.val = NULL;
    vector_push_back(&klc->objs, &obj);

    int len = strlen(name);
    ASSERT(len <= 255);
    klc_add_str(klc, name, len);

    len = strlen(desc);
    klc_add_str(klc, (char *)desc, len);

    klc_add_int(klc, flags, 2);

    ++klc->hdr.num_symbols;
}

void klc_add_func(KlcFile *klc, char *name, char *desc, int flags)
{
    KlcObject obj;

    obj.type = KLC_TYPE_FUNC;
    obj.len = 0;
    obj.val = NULL;
    vector_push_back(&klc->objs, &obj);

    int len = strlen(name);
    ASSERT(len <= 255);
    klc_add_str(klc, name, len);

    len = desc ? strlen(desc) : 0;
    klc_add_str(klc, desc ?: "", len);

    klc_add_int(klc, flags, 2);

    ++klc->hdr.num_symbols;
}

void klc_add_code(KlcFile *klc, CodeSpec *cs)
{
    KlcObject obj = { 0 };
    obj.type = KLC_TYPE_CODE;
    obj.len = 0; // no length
    obj.val = cs;
    vector_push_back(&klc->objs, &obj);
}

static char *read_ascii(int len, FILE *fp)
{
    char *s = mm_alloc_fast(len + 1);
    ASSERT(s);
    fread(s, len, 1, fp);
    s[len] = '\0';
    return s;
}

int read_object(FILE *fp, KlcObject *obj)
{
    int type = read_byte(fp);
    obj->type = type;

    switch (type) {
        case KLC_TYPE_VAR: {
            // do nothing
            break;
        }
        case KLC_TYPE_SHORT_ASCII: {
            int len = read_byte(fp);
            char *s = read_ascii(len, fp);
            obj->len = len;
            obj->val = (void *)s;
            break;
        }
        default:
            break;
    }
    return 0;
}

FILE *open_klc_file(const char *path, char *mode)
{
    FILE *fp = fopen(path, mode);
    if (fp == NULL) {
        char *end = strrchr(path, '/');
        if (!end) {
            print_error("Cannot open '%s'.", path);
            return NULL;
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
            print_error("Cannot open '%s'.", path);
            return NULL;
        }
    }
    return fp;
}

static int read_byte2(KlcFile *klc)
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

static int read_int(KlcFile *klc, int len)
{
    int val = 0;
    fread(&val, len, 1, klc->filp);
    return val;
}

static char *read_ascii2(int len, KlcFile *klc)
{
    char *s = mm_alloc_fast(len + 1);
    ASSERT(s);
    fread(s, len, 1, klc->filp);
    s[len] = '\0';
    return s;
}

#define save(obj) vector_push_back(&klc->objs, (obj))

static void read_object2(KlcFile *klc)
{
    int code = read_byte2(klc);
    int type = code & ~FLAG_REF;
    int flag = code & FLAG_REF;

    KlcObject obj = { 0 };
    obj.type = type;

    switch (type) {
        case KLC_TYPE_NONE: {
            break;
        }
        case KLC_TYPE_VAR: {
            save(&obj);
            read_object2(klc);
            read_object2(klc);
            read_object2(klc);
            break;
        }
        case KLC_TYPE_FUNC: {
            save(&obj);
            read_object2(klc);
            read_object2(klc);
            read_object2(klc);
            read_object2(klc);
            read_object2(klc);
            break;
        }
        case KLC_TYPE_SHORT_ASCII: {
            int len = read_byte2(klc);
            char *s = read_ascii2(len, klc);
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
        //     read_object2(&bytes, klc);
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
            int len = read_byte2(klc);
            int64_t v = read_int(klc, len);
            obj.len = len;
            obj.ival = v;
            save(&obj);
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

    total += klc->hdr.num_symbols;

    if (!only_meta) {
        total += klc->hdr.num_relocs;
        total += klc->hdr.num_codes;
        total += klc->hdr.num_consts;
    }

    for (int i = 0; i < total; i++) {
        read_object2(klc);
    }
}

static inline int read_check_magic(KlcFileHeader *hdr, FILE *filp)
{
    uint8_t magic[4] = { 0 };
    fread(magic, sizeof(magic), 1, filp);
    if (memcmp(magic, hdr->magic, 4)) {
        print_error("error: klc file magic check failed");
        return -1;
    }
    return 0;
}

static inline int read_check_version(KlcFileHeader *hdr, FILE *filp)
{
    uint32_t version = 0;
    fread(&version, sizeof(version), 1, filp);
    if (version > hdr->version) {
        print_error("error: klc file version check failed");
        return -1;
    }
    return 0;
}

static inline void read_numbers(KlcFileHeader *hdr, FILE *filp)
{
    fread(&hdr->num_symbols, 2, 1, filp);
    fread(&hdr->num_relocs, 2, 1, filp);
    fread(&hdr->num_codes, 2, 1, filp);
    fread(&hdr->num_consts, 2, 1, filp);
}

int read_klc_file(KlcFile *klc, int only_meta)
{
    FILE *fp = open_klc_file(klc->path, "r");
    klc->filp = fp;
    read_check_magic(&klc->hdr, fp);
    read_check_version(&klc->hdr, fp);
    read_numbers(&klc->hdr, fp);
    read_objects(klc, only_meta);
    fclose(fp);
    return 0;
}

void init_klc_file_header(KlcFileHeader *hdr)
{
    memcpy(hdr->magic, "klc", 4);
    uint32_t version = (KOALA_VERSION_MAJOR << 24);
    version += (KOALA_VERSION_MINOR << 16);
    version += KOALA_VERSION_PATCH;
    hdr->version = version;
}

int read_klc_file_header(KlcFileHeader *hdr, FILE *filp)
{
    if (read_check_magic(hdr, filp) < 0) return -1;
    if (read_check_version(hdr, filp) < 0) return -1;
    read_numbers(hdr, filp);
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
        case KLC_TYPE_NONE: {
            // nothing to do
            break;
        }
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
        case KLC_TYPE_FLOAT: {
            fwrite(&obj->len, 1, 1, fp);
            fwrite(&obj->fval, obj->len, 1, fp);
            break;
        }
        case KLC_TYPE_BYTES: {
            fwrite(&obj->len, 4, 1, fp);
            fwrite(obj->val, 1, obj->len, fp);
            break;
        }
        case KLC_TYPE_VAR:
        // fall-through
        case KLC_TYPE_VAR_VAL:
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
    fwrite(klc->hdr.magic, sizeof(klc->hdr.magic), 1, klc->filp);
}

static inline void write_version(KlcFile *klc)
{
    fwrite(&klc->hdr.version, sizeof(klc->hdr.version), 1, klc->filp);
}

static inline void write_numbers(KlcFile *klc)
{
    fwrite(&klc->hdr.num_symbols, sizeof(uint16_t), 1, klc->filp);
    fwrite(&klc->hdr.num_relocs, sizeof(uint16_t), 1, klc->filp);
    fwrite(&klc->hdr.num_codes, sizeof(uint16_t), 1, klc->filp);
    fwrite(&klc->hdr.num_consts, sizeof(uint16_t), 1, klc->filp);
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
    memcpy(klc->hdr.magic, "klc", 4);
    uint32_t version =
        (KOALA_VERSION_MAJOR << 24) + (KOALA_VERSION_MINOR << 16) + KOALA_VERSION_PATCH;
    klc->hdr.version = version;
    klc->path = path;
    klc->filp = NULL;
    vector_init(&klc->objs, sizeof(KlcObject));
}

void fini_klc_file(KlcFile *klc) {}

void klc_dump(KlcFile *klc)
{
    printf("number of symbols: %d\n", klc->hdr.num_symbols);

    int i = 0;
    while (i < vector_size(&klc->objs)) {
        KlcObject *v = vector_get(&klc->objs, i);
        switch (v->type) {
            case KLC_TYPE_VAR: {
                KlcObject *name = vector_get(&klc->objs, i + 1);
                KlcObject *desc = vector_get(&klc->objs, i + 2);
                KlcObject *flags = vector_get(&klc->objs, i + 3);
                printf("var %s:%s\n", (char *)name->val, (char *)desc->val);
                i += 4;
                break;
            }
            case KLC_TYPE_VAR_VAL: {
                KlcObject *name = vector_get(&klc->objs, i + 1);
                KlcObject *desc = vector_get(&klc->objs, i + 2);
                KlcObject *flags = vector_get(&klc->objs, i + 3);
                KlcObject *val = vector_get(&klc->objs, i + 4);
                printf("var %s: %s\n", (char *)name->val, (char *)desc->val);
                if (val->type == KLC_TYPE_INT) {
                    printf("val: %ld\n", val->ival);
                } else if (val->type == KLC_TYPE_FLOAT) {
                    printf("val: %g\n", val->fval);
                } else if (val->type == KLC_TYPE_SHORT_ASCII) {
                    printf("val: %s\n", (char *)val->val);
                } else if (val->type == KLC_TYPE_NONE) {
                    printf("val: none\n");
                }
                i += 5;
                break;
            }
            case KLC_TYPE_FUNC: {
                KlcObject *name = vector_get(&klc->objs, i + 1);
                KlcObject *desc = vector_get(&klc->objs, i + 2);
                KlcObject *flags = vector_get(&klc->objs, i + 3);
                printf("func %s:%s\n", (char *)name->val, (char *)desc->val);
                int v = flags->ival;
                if (v & (1 << 5)) {
                    KlcObject *s = vector_get(&klc->objs, i + 4);
                    printf("at: %s\n", (char *)s->val);
                    i += 5;
                } else if (v & (1 << 6)) {
                    KlcObject *s = vector_get(&klc->objs, i + 4);
                    KlcObject *s2 = vector_get(&klc->objs, i + 5);
                    printf("at: %s, %s\n", (char *)s->val, (char *)s2->val);
                    i += 6;
                }
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

/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_KLC_H_
#define _KOALA_KLC_H_

#include "codespec.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ITEM_STRING   0
#define ITEM_DESC_STR 1
#define ITEM_LITERAL  2
#define ITEM_CODE     3
#define ITEM_VAR      4
#define ITEM_FUNC     5
#define ITEM_KLASS    6
#define ITEM_LOCAL    7
#define ITEM_MAX      8

#define KLC_TYPE_NONE    'N'
#define KLC_TYPE_INT     'i'
#define KLC_TYPE_FLOAT   'f'
#define KLC_TYPE_ASCII   'A'
#define KLC_TYPE_UTF8    'U'
#define KLC_TYPE_TUPLE   '('
#define KLC_TYPE_LIST    '['
#define KLC_TYPE_MAP     '{'
#define KLC_TYPE_SET     '<'
#define KLC_TYPE_CODE    'C'
#define KLC_TYPE_BYTES   'B'
#define KLC_TYPE_CLASS   'K'
#define KLC_TYPE_TRAIT   'I'
#define KLC_TYPE_VAR     'v'
#define KLC_TYPE_VAR_VAL 'V'
#define KLC_TYPE_PROTO   'p'
#define KLC_TYPE_FUNC    'P'

/* with a type, add obj to index */
#define FLAG_REF '\x80'

#define KLC_TYPE_SHORT_ASCII 'a'
#define KLC_TYPE_SHORT_UTF8  'u'
#define KLC_TYPE_SMALL_TUPLE ')'

typedef struct _ItemHdr {
    uint32_t offset;
    uint32_t size;
} ItemHdr;

typedef struct _KlcFileHeader {
    uint8_t magic[4];
    uint32_t version;
    ItemHdr item_hdrs[ITEM_MAX];
} KlcFileHeader;

typedef struct _KlcObject {
    char type;
    int len;
    union {
        int64_t ival;
        double fval;
        void *val;
    };
} KlcObject;

static int read_byte(FILE *fp)
{
    int val = 0;
    fread(&val, 1, 1, fp);
    return val;
}

int read_object(FILE *fp, KlcObject *obj);

typedef struct _KlcFile {
    /* file path */
    const char *path;
    /* file pointer */
    FILE *filp;
    /* header */
    KlcFileHeader hdr;
    /* unique */
    HashMap map;
    /* items */
    Vector items[ITEM_MAX];
} KlcFile;

void klc_add_var(KlcFile *klc, char *name, char *desc, int has_value, int flags);
void klc_add_func(KlcFile *klc, char *name, char *desc, int flags);

void klc_add_none(KlcFile *klc);
void klc_add_int(KlcFile *klc, int64_t val, int len);
void klc_add_float(KlcFile *klc, double val, int len);
void klc_add_str(KlcFile *klc, char *s, int len);
void klc_add_utf8(KlcFile *klc, char *s, int len);
void klc_add_code(KlcFile *klc, CodeSpec *cs);

void init_klc_file(KlcFile *klc, const char *path);
void fini_klc_file(KlcFile *klc);
int write_klc_file(KlcFile *klc);
int read_klc_file(KlcFile *klc, int only_meta);
void init_klc_file_header(KlcFileHeader *hdr);
FILE *open_klc_file(const char *path, char *mode);
int read_klc_file_header(KlcFileHeader *hdr, FILE *filp);
void klc_dump(KlcFile *klc);

void kl_write_to_klc();

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_KLC_H_ */

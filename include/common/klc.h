/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_KLC_H_
#define _KOALA_KLC_H_

#include "codespec.h"
#include "hashmap.h"

#ifdef __cplusplus
extern "C" {
#endif

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

typedef struct _KlcFileHeader {
    uint8_t magic[4];
    uint32_t version;
    uint16_t num_symbols;
    uint16_t num_relocs;
    uint16_t num_consts;
    uint16_t num_codes;
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

#define ITEM_LIT_STR  0
#define ITEM_DESC_STR 1
#define ITEM_SYM_STR  2
#define ITEM_LITERAL  3
#define ITEM_RELOC    4
#define ITEM_UNI_MAX  5

typedef struct _UniItem {
    HashMap map;
    Vector value;
} UniItem;

typedef struct _KlcString {
    int len;
    char data[0];
} KlcString;

#define LIT_INT   1
#define LIT_FLT   2
#define LIT_STR   3
#define LIT_TUPLE 4
#define LIT_LIST  5

typedef struct _KlcLiteral {
    int type;
    int len;
    union {
        /* integer */
        int64_t ival;
        /* float */
        double fval;
        /* str/tuple/list/dict/set */
        void *val;
    };
} KlcLiteral;

typedef struct _KlcVar {
    /* flags */
    uint16_t flags;
    /* ITEM_SYM_STR */
    uint16_t name_index;
    /* ITEM_DESC_STR */
    uint16_t type_index;
    /* ITEM_LITERAL */
    uint16_t literal_index;
} KlcVar;

typedef struct _KlcFunc {
    /* flags */
    uint16_t flags;
    /* ITEM_SYM_STR */
    uint16_t name_index;
    /* ITEM_DESC_STR */
    uint16_t desc_index;
    /* ITEM_DESC_STR */
    uint16_t tps_index;
    /* number of annotations */
    uint16_t num_anns;
    /* code index */
    uint16_t code_index;
} KlcFunc;
 
typedef struct _KlcAnnot {
    /* ITEM_SYM_STR */
    uint16_t name_index;
    /* ITEM_SYM_STR */
    uint16_t key_index;
    /* ITEM_LITERAL */
    uint16_t value_index;
} KlcAnnot;

typedef struct _KlcFile {
    const char *path;
    FILE *filp;
    KlcFileHeader hdr;
    UniItem uniques[ITEM_UNI_MAX];
    Vector vars;
    Vector funcs;
    Vector classes;
    Vector codes;
    Vector objs;
} KlcFile;

void klc_add_var(KlcFile *klc, char *name, char *desc, int flags);
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

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_KLC_H_ */

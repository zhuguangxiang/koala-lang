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

#define ITEM_CONST 0
#define ITEM_VAR   1
#define ITEM_FUNC  2
#define ITEM_CLASS 3
#define ITEM_RELOC 4
#define ITEM_CODES 5
#define ITEM_MAX   6

typedef struct _KlcConst {
    int type;
    int len;
    union {
        /* integer */
        int64_t ival;
        /* float */
        double fval;
        /* string */
        char *sval;
        /* tuple/list/dict/set */
        void *val;
    };
} KlcConst;

#define KLC_CONST_NONE  'N'
#define KLC_CONST_INT   'i'
#define KLC_CONST_FLT   'f'
#define KLC_CONST_ASCII 'A'
#define KLC_CONST_UTF8  'U'
#define KLC_CONST_TUPLE '('
#define KLC_CONST_LIST  '['
#define KLC_CONST_DICT  '{'
#define KLC_CONST_SET   '<'
// object created in __init__
#define KLC_CONST_OBJECT 'O'

#define KLC_CONST_SHORT_ASCII 'a'
#define KLC_CONST_SHORT_UTF8  'u'
#define KLC_CONST_SHORT_TUPLE ')'
#define KLC_CONST_SHORT_LIST  ']'

typedef struct _KlcVar {
    /* flags */
    uint16_t flags;
    /* ITEM_CONST */
    uint16_t name_index;
    /* ITEM_CONST */
    uint16_t type_index;
    /* ITEM_CONST */
    uint16_t const_index;
} KlcVar;

typedef struct _KlcFunc {
    /* flags */
    uint16_t flags;
    /* ITEM_CONST */
    uint16_t name_index;
    /* ITEM_CONST */
    uint16_t ret_type_index;
    /* code index */
    uint16_t code_index;
    /* arguments */
    Vector args;
    /* type parameters */
    Vector tps;
    /* annotations */
    Vector anns;
} KlcFunc;

typedef struct _KlcArgument {
    /* ITEM_CONST */
    uint16_t name_index;
    /* ITEM_CONST */
    uint16_t type_index;
    /* ITEM_CONST */
    uint16_t const_index;
} KlcArgument;

typedef struct _KlcTypePara {
    /* ITEM_CONST */
    uint16_t name_index;
    /* ITEM_CONST */
    uint16_t type_index;
} KlcTypePara;

typedef struct _KlcAnnot {
    /* ITEM_CONST */
    uint16_t name_index;
    /* ITEM_CONST */
    uint16_t key_index;
    /* ITEM_CONST */
    uint16_t value_index;
} KlcAnnot;

typedef struct _KlcKlass {
    /* flags */
    uint16_t flags;
    /* ITEM_CONST */
    uint16_t name_index;
    /* ITEM_CONST */
    uint16_t tps_index;
    /* number of bases */
    uint16_t num_bases;
    /* number of fields */
    uint16_t num_fields;
    /* number of methods */
    uint16_t num_methods;
} KlcKlass;

typedef struct _KlcCode {
    /* number of locals */
    uint16_t num_locals;
    /* byte codes size */
    uint16_t code_size;
    /* codes */
    char *codes;
} KlcCode;

typedef struct _KlcReloc {
    /* ITEM_CONST */
    uint16_t ns_index;
    /* ITEM_CONST */
    uint16_t sym_index;
} KlcReloc;

typedef struct _KlcFile {
    const char *path;
    FILE *filp;
    uint8_t magic[4];
    uint32_t version;
    HashMap map;
    Vector objs[ITEM_MAX];
} KlcFile;

KlcVar *klc_add_var(KlcFile *klc, char *name, char *desc, uint16_t index, int flags);
KlcFunc *klc_add_func(KlcFile *klc, char *name, char *ret_desc, int flags);
int klc_func_add_arg(KlcFile *klc, KlcFunc *fn, char *name, char *desc, uint16_t index);
int klc_func_add_tp(KlcFile *klc, KlcFunc *fn, char *name, char *desc);
int klc_func_add_ann(KlcFile *klc, KlcFunc *fn, char *name, char *key, char *value);

uint16_t klc_add_none(KlcFile *klc);
uint16_t klc_add_int(KlcFile *klc, int64_t val);
uint16_t klc_add_float(KlcFile *klc, double val);
uint16_t klc_add_str(KlcFile *klc, char *s, int len);
uint16_t klc_add_utf8(KlcFile *klc, char *s, int len);

void init_klc_file(KlcFile *klc, const char *path);
void fini_klc_file(KlcFile *klc);

int write_klc_file(KlcFile *klc);
int read_klc_file(KlcFile *klc, int all);

KlcConst *klc_get_const(KlcFile *klc, uint16_t index);

void klc_dump(KlcFile *klc);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_KLC_H_ */

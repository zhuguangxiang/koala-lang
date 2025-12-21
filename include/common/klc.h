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
#define ITEM_CODE  5
#define ITEM_MAX   6

typedef struct _KlcFile {
    const char *path;
    FILE *filp;
    uint8_t magic[4];
    uint32_t version;
    HashMap map;
    Vector objs[ITEM_MAX];
} KlcFile;

typedef struct _KlcConst {
    short type;
    short sign;
    int len;
    union {
        /* integer */
        uint64_t ival;
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

#define KLC_FLAGS_PUB     (1 << 0)
#define KLC_FLAGS_MUTABLE (1 << 1)
#define KLC_FLAGS_FINAL   (1 << 2)
#define KLC_FLAGS_STATIC  (1 << 3)

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
    /* point back to klc file */
    KlcFile *filp;
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
    /* type params */
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
    /* point back to klc file */
    KlcFile *filp;
    /* flags */
    uint16_t flags;
    /* ITEM_CONST */
    uint16_t name_index;
    /* type params */
    Vector tps;
    /* annotations */
    Vector anns;
    /* bases */
    Vector bases;
    /* fields */
    Vector fields;
    /* methods */
    Vector methods;
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

KlcVar *klc_add_var(KlcFile *klc, char *name, char *desc, uint16_t index, int flags);

KlcFunc *klc_add_func(KlcFile *klc, char *name, char *ret_desc, int flags);
int klc_func_add_arg(KlcFunc *fn, char *name, char *desc, uint16_t index);
int klc_func_add_tp(KlcFunc *fn, char *name, char *desc);
int klc_func_add_ann(KlcFunc *fn, char *name, char *key, char *value);

KlcKlass *klc_add_klass(KlcFile *klc, char *name, int flags);
int klc_klass_add_tp(KlcKlass *kls, char *name, char *desc);
int klc_klass_add_ann(KlcKlass *kls, char *name, char *key, char *value);
int klc_klass_add_base(KlcKlass *kls, char *base_desc);
KlcVar *klc_klass_add_field(KlcKlass *kls, char *name, char *desc, int flags);
KlcFunc *klc_klass_add_func(KlcKlass *kls, char *name, char *ret_desc, int flags);

uint16_t klc_add_none(KlcFile *klc);
uint16_t klc_add_int(KlcFile *klc, uint64_t val, int sign, int width);
uint16_t klc_add_float(KlcFile *klc, double val);
uint16_t klc_add_str(KlcFile *klc, char *s, int len);
uint16_t klc_add_utf8(KlcFile *klc, char *s, int len);

uint16_t klc_add_code(KlcFile *klc, int num_locals, int code_size, char *codes);
uint16_t klc_add_reloc(KlcFile *klc, char *ns, char *sym);

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

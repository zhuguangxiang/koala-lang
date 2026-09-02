/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_KLC_H_
#define _KOALA_KLC_H_

#include "hashmap.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ITEM_RT_CONST 0
#define ITEM_IMPORT   1
#define ITEM_LINK     2
#define ITEM_CODE     3
#define ITEM_BYTECODE 4
#define ITEM_CONST    5
#define ITEM_VAR      6
#define ITEM_FUNC     7
#define ITEM_CLASS    8
#define ITEM_MAX      9

typedef struct _KlcFile {
    char *path;
    char *pkg_path;
    FILE *filp;
    uint8_t magic[4];
    uint32_t version;
    uint16_t num_rt_consts;
    uint16_t pkg_path_index;
    uint8_t endian;
    uint8_t padding[3];
    HashMap map;
    Vector objs[ITEM_MAX];
} KlcFile;

typedef struct _KlcConst {
    short type;
    short sign;
    int len;
    int type_info;
    int unused;
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
#define KLC_CONST_BOOL  'b'
#define KLC_CONST_ASCII 'A'
#define KLC_CONST_UTF8  'U'
#define KLC_CONST_TUPLE '('
#define KLC_CONST_LIST  '['
#define KLC_CONST_DICT  '{'
#define KLC_CONST_SET   '<'
#define KLC_CONST_RANGE 'R'
#define KLC_CONST_SLICE 'S'

// object created in __init__
#define KLC_CONST_OBJECT 'O'

#define KLC_CONST_SHORT_ASCII 'a'
#define KLC_CONST_SHORT_UTF8  'u'
#define KLC_CONST_SHORT_TUPLE ')'
#define KLC_CONST_SHORT_LIST  ']'

#define KLC_FLAGS_PUB    (1 << 0)
#define KLC_FLAGS_MUT    (1 << 1)
#define KLC_FLAGS_TRAIT  (1 << 2)
#define KLC_FLAGS_METH   (1 << 3)
#define KLC_FLAGS_NATIVE (1 << 4)
#define KLC_FLAGS_STATIC (1 << 5)

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
    /* slot id */
    int16_t slot_id;
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

typedef struct _KlcTypeParam {
    /* ITEM_CONST */
    uint16_t name_index;
    /* flags */
    uint8_t which;
    /* up-bounds(type_index) */
    Vector bounds;
} KlcTypeParam;

typedef struct _KlcAnnot {
    /* ITEM_CONST */
    uint16_t name_index;
    /* ITEM_CONST */
    uint16_t key_index;
    /* ITEM_CONST */
    uint16_t value_index;
} KlcAnnot;

typedef struct _KlcIntfEntry {
    /* ITEM_CONST(parent trait) */
    uint16_t name_index;
    /* methods */
    Vector methods;
    /* upcast parents */
    Vector parents;
} KlcIntfEntry;

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
    /* pip */
    Vector pip;
    /* lro */
    Vector lro;
    /* fields */
    Vector fields;
    /* methods */
    Vector methods;
    /* intf entry */
    Vector intf_table;
} KlcKlass;

typedef struct _KlcCode {
    /* name index */
    uint16_t name_index;
    /* flags */
    uint16_t flags;
    /* number of locals */
    uint16_t nlocals;
    /* max call arguments */
    uint16_t max_call_args;
    /* code offset */
    uint32_t start_pc;
    /* number of insns */
    uint32_t num_insns;
} KlcCode;

typedef struct _KlcImport {
    /* the same as KlMachImport kind */
    uint8_t kind;
    /* ITEM_CONST(path) */
    uint16_t ns_index;
    /* ITEM_CONST(klass) */
    uint16_t kls_index;
    /* ITEM_CONST(name) */
    uint16_t sym_index;
} KlcImport;

typedef struct _KlcByteCode {
    /* size of the bytecode array in bytes */
    uint32_t size;
    /* pointer to the bytecode array */
    uint8_t *codes;
} KlcByteCode;

KlcVar *klc_add_var(KlcFile *klc, char *name, char *type, uint16_t index, int flags);

KlcFunc *klc_add_func(KlcFile *klc, char *name, char *type, int flags);
int klc_func_add_arg(KlcFunc *fn, char *name, char *type, uint16_t index);
KlcTypeParam *klc_func_add_tp(KlcFunc *fn, char *name);
int klc_func_add_ann(KlcFunc *fn, char *name, char *key, char *value);

KlcKlass *klc_add_klass(KlcFile *klc, char *name, int flags);
KlcTypeParam *klc_klass_add_tp(KlcKlass *kls, char *name);
KlcFunc *klc_klass_add_func(KlcKlass *kls, char *name, char *ret_type, int flags);
KlcVar *klc_klass_add_field(KlcKlass *kls, char *name, char *type, int flags);
KlcIntfEntry *klc_klass_add_intf_entry(KlcKlass *kls);

uint16_t klc_add_none(KlcFile *klc);
uint16_t klc_add_int(KlcFile *klc, uint64_t val, int sign, int width);
uint16_t klc_add_float(KlcFile *klc, double val, int width);
uint16_t klc_add_str(KlcFile *klc, char *s, int len);
uint16_t klc_add_utf8(KlcFile *klc, char *s, int len);

uint16_t klc_add_code(KlcFile *klc, char *name, int flags, uint16_t num_locals,
                      uint16_t max_call_args, uint32_t start_pc, uint32_t code_size);
KlcCode *klc_get_code(KlcFile *klc, uint16_t index);

uint16_t klc_add_rt_none(KlcFile *klc);
uint16_t klc_add_rt_bool(KlcFile *klc, int val);
uint16_t klc_add_rt_int(KlcFile *klc, uint64_t val, int sign, int width);
uint16_t klc_add_rt_float(KlcFile *klc, double val, int width);
uint16_t klc_add_rt_str(KlcFile *klc, char *s, int len);
uint16_t klc_add_rt_tuple(KlcFile *klc, Vector *list);
uint16_t klc_add_rt_range(KlcFile *klc, Vector *list);
uint16_t klc_add_rt_list(KlcFile *klc, Vector *list);
uint16_t klc_add_rt_slice(KlcFile *klc, Vector *list);

void klc_add_import(KlcFile *klc, int kind, char *ns, char *kls, char *sym);
void klc_add_link(KlcFile *klc, char *path);

void klc_add_bytecodes(KlcFile *klc, uint32_t size, uint8_t *codes);

void init_klc_file(KlcFile *klc, char *path, char *pkg_path);
void fini_klc_file(KlcFile *klc);

int write_klc_file(KlcFile *klc);

KlcFile *read_klc_file(char *path, int rt);
void free_klc_file(KlcFile *klc);

KlcConst *klc_get_const(KlcFile *klc, uint16_t index);
KlcConst *klc_get_rt_const(KlcFile *klc, uint16_t index);
uint32_t klc_get_bytecodes(KlcFile *klc, uint8_t **codes);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_KLC_H_ */

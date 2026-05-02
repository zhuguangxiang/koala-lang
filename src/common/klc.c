/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "klc.h"
#include "atom.h"
#include "buffer.h"
#include "version.h"

#ifdef __cplusplus
extern "C" {
#endif

/* hash node for data is unique */
typedef struct _ItemEntry {
    /* hash entry */
    HashMapEntry entry;
    /* item type */
    int type;
    /* item data */
    KlcConst *data;
    /* index of vector */
    uint16_t index;
} ItemEntry;

static int __const_equal(KlcConst *k1, KlcConst *k2)
{
    if (k1->type != k2->type) return 0;

    switch (k1->type) {
        case KLC_CONST_NONE: {
            return 1;
        }
        case KLC_CONST_INT: {
            if (k1->sign != k2->sign) return 0;
            if (k1->len != k2->len) return 0;
            return k1->ival == k2->ival;
        }
        case KLC_CONST_FLT: {
            if (k1->len != k2->len) return 0;
            return k1->fval == k2->fval;
        }
        case KLC_CONST_ASCII:      // fall-through
        case KLC_CONST_UTF8:       // fall-through
        case KLC_CONST_SHORT_UTF8: // fall-through
        case KLC_CONST_SHORT_ASCII: {
            if (k1->sval == k2->sval) return 1;
            if (k1->len != k2->len) return 0;
            return !strncmp(k1->sval, k2->sval, k1->len);
        }
        default: {
            UNREACHABLE();
        }
    }
}

static int __item_entry_equal(void *n1, void *n2)
{
    ItemEntry *e1 = (ItemEntry *)n1;
    ItemEntry *e2 = (ItemEntry *)n2;
    if (e1->type != e2->type) return 0;
    return __const_equal(e1->data, e2->data);
}

static void __item_entry_free(void *obj, void *arg) { mm_free(obj); }

static unsigned int init_item_entry(ItemEntry *item, int type, KlcConst *data)
{
    uint64_t hash = mem_hash(data, sizeof(KlcConst));
    item->type = type;
    item->data = data;
    item->index = 0;
    hashmap_entry_init(item, hash);
    return 0;
}

static uint16_t __index(KlcFile *klc, int type, void *data)
{
    ItemEntry key;
    init_item_entry(&key, type, data);
    ItemEntry *res = hashmap_get(&klc->map, &key);
    return res ? res->index : 0;
}

static uint16_t __append(KlcFile *klc, int type, KlcConst *data)
{
    Vector *objs = klc->objs + type;
    vector_push_back(objs, &data);
    uint16_t index = vector_size(objs) - 1;
    ASSERT(index > 0);

    ItemEntry *e = mm_alloc_obj(e);
    init_item_entry(e, type, data);
    e->index = index;
    int res = hashmap_put_absent(&klc->map, e);
    ASSERT(res == 0);

    return index;
}

#define STR_TYPE(len)  ((len <= 255) ? KLC_CONST_SHORT_ASCII : KLC_CONST_ASCII)
#define UTF8_TYPE(len) ((len <= 255) ? KLC_CONST_SHORT_UTF8 : KLC_CONST_UTF8)

static uint16_t __add_none(KlcFile *klc, int type)
{
    KlcConst k = { .type = KLC_CONST_NONE };
    uint16_t idx = __index(klc, type, &k);
    if (idx == 0) {
        KlcConst *item = mm_alloc_obj(item);
        item->type = KLC_CONST_NONE;
        idx = __append(klc, type, item);
    }
    return idx;
}

static uint16_t __add_int(KlcFile *klc, uint64_t val, int sign, int width, int type)
{
    // 1,2,4,8 → 0,1,2,3
    int _width = __builtin_ctz(width);
    // 0=i, 1=u
    int _sign = sign ? 0 : 1;
    int ti = 0b1000 + (_sign << 2) + _width;

    KlcConst k = {
        .type = KLC_CONST_INT,
        .sign = sign,
        .len = width,
        .type_info = ti,
        .ival = val,
    };

    uint16_t idx = __index(klc, type, &k);
    if (idx == 0) {
        KlcConst *item = mm_alloc_obj(item);
        item->type = KLC_CONST_INT;
        item->sign = sign;
        item->len = width;
        item->ival = val;
        item->type_info = ti;
        idx = __append(klc, type, item);
    }
    return idx;
}

static uint16_t __add_float(KlcFile *klc, double val, int width, int type)
{
    // 2,4,8 → 1,2,3
    int _width = __builtin_ctz(width);
    int ti = 0b010000 + _width;

    KlcConst k = {
        .type = KLC_CONST_FLT,
        .len = width,
        .type_info = ti,
        .fval = val,
    };

    uint16_t idx = __index(klc, type, &k);
    if (idx == 0) {
        KlcConst *item = mm_alloc_obj(item);
        item->type = KLC_CONST_FLT;
        item->len = width;
        item->fval = val;
        item->type_info = ti;
        idx = __append(klc, type, item);
    }
    return idx;
}

static uint16_t __add_str(KlcFile *klc, char *s, int len, int type)
{
    if (len == 0) return 0;

    int _type = STR_TYPE(len);
    char *_s = atom(s);
    KlcConst k = { .type = _type, .len = len, .sval = _s };
    uint16_t idx = __index(klc, type, &k);
    if (idx == 0) {
        KlcConst *item = mm_alloc_obj(item);
        item->type = _type;
        item->len = len;
        item->sval = _s;
        idx = __append(klc, type, item);
    }
    return idx;
}

uint16_t klc_add_none(KlcFile *klc) { return __add_none(klc, ITEM_CONST); }

uint16_t klc_add_int(KlcFile *klc, uint64_t val, int sign, int width)
{
    return __add_int(klc, val, sign, width, ITEM_CONST);
}

uint16_t klc_add_float(KlcFile *klc, double val, int width)
{
    return __add_float(klc, val, width, ITEM_CONST);
}

uint16_t klc_add_str(KlcFile *klc, char *s, int len) { return __add_str(klc, s, len, ITEM_CONST); }

uint16_t klc_add_utf8(KlcFile *klc, char *s, int len)
{
    if (len == 0) return 0;

    int type = UTF8_TYPE(len);
    char *_s = atom(s);
    KlcConst k = { .type = type, .len = len, .sval = _s };
    uint16_t idx = __index(klc, ITEM_CONST, &k);
    if (idx == 0) {
        KlcConst *item = mm_alloc_obj(item);
        item->type = type;
        item->len = len;
        item->sval = _s;
        idx = __append(klc, ITEM_CONST, item);
    }
    return idx;
}

uint16_t klc_add_code(KlcFile *klc, char *name, uint16_t num_locals, uint16_t max_call_args,
                      uint32_t start_pc, uint32_t num_insns)
{
    KlcCode *code = mm_alloc_obj(code);
    uint32_t name_index = klc_add_rt_str(klc, name, strlen(name));
    code->name_index = name_index;
    code->nlocals = num_locals;
    code->max_call_args = max_call_args;
    code->start_pc = start_pc;
    code->num_insns = num_insns;
    vector_push_back(klc->objs + ITEM_CODE, &code);
    return vector_size(klc->objs + ITEM_CODE) - 1;
}

uint16_t klc_add_rt_int(KlcFile *klc, uint64_t val, int sign, int width)
{
    return __add_int(klc, val, sign, width, ITEM_RT_CONST);
}

uint16_t klc_add_rt_float(KlcFile *klc, double val, int width)
{
    return __add_float(klc, val, width, ITEM_RT_CONST);
}

uint16_t klc_add_rt_str(KlcFile *klc, char *s, int len)
{
    return __add_str(klc, s, len, ITEM_RT_CONST);
}

uint16_t klc_add_rt_tuple(KlcFile *klc, Vector *list)
{
    KlcConst *item = mm_alloc_obj(item);
    int size = vector_size(list);
    if (size <= 255) {
        item->type = KLC_CONST_SHORT_TUPLE;
    } else {
        item->type = KLC_CONST_TUPLE;
    }
    item->len = size;
    item->val = list;

    Vector *objs = klc->objs + ITEM_RT_CONST;
    vector_push_back(objs, &item);
    uint16_t index = vector_size(objs) - 1;
    ASSERT(index > 0);

    return index;
}

void klc_add_import(KlcFile *klc, int kind, char *ns, char *sym)
{
    KlcImport *imp = mm_alloc_obj(imp);
    imp->kind = kind;
    imp->ns_index = klc_add_rt_str(klc, ns, strlen(ns));
    imp->sym_index = klc_add_rt_str(klc, sym, strlen(sym));
    vector_push_back(klc->objs + ITEM_IMPORT, &imp);
}

void klc_add_bytecodes(KlcFile *klc, uint32_t size, uint8_t *codes)
{
    KlcByteCode *bc = mm_alloc_obj(bc);
    bc->size = size;
    bc->codes = codes;
    vector_push_back(klc->objs + ITEM_BYTECODE, &bc);
}

KlcVar *klc_add_var(KlcFile *klc, char *name, char *desc, uint16_t index, int flags)
{
    int len = strlen(name);
    uint16_t name_index = klc_add_str(klc, name, len);
    len = strlen(desc);
    uint16_t desc_index = klc_add_str(klc, desc, len);
    KlcVar *var = mm_alloc_obj(var);
    var->flags = flags;
    var->name_index = name_index;
    var->type_index = desc_index;
    var->const_index = index;
    vector_push_back(klc->objs + ITEM_VAR, &var);
    return var;
}

static KlcFunc *new_func(KlcFile *klc, char *name, char *ret_desc, int flags)
{
    int len = strlen(name);
    uint16_t name_index = klc_add_str(klc, name, len);
    len = ret_desc ? strlen(ret_desc) : 0;
    uint16_t ret_index = klc_add_str(klc, ret_desc, len);
    KlcFunc *fn = mm_alloc_obj(fn);
    fn->filp = klc;
    fn->flags = flags;
    fn->name_index = name_index;
    fn->ret_type_index = ret_index;
    fn->code_index = 0;
    vector_init_ptr(&fn->args);
    vector_init_ptr(&fn->tps);
    vector_init_ptr(&fn->anns);
    void *empty = NULL;
    vector_push_back(&fn->args, &empty);
    vector_push_back(&fn->tps, &empty);
    vector_push_back(&fn->anns, &empty);
    return fn;
}

KlcFunc *klc_add_func(KlcFile *klc, char *name, char *ret_desc, int flags)
{
    KlcFunc *fn = new_func(klc, name, ret_desc, flags);
    vector_push_back(klc->objs + ITEM_FUNC, &fn);
    return fn;
}

int klc_func_add_arg(KlcFunc *fn, char *name, char *desc, uint16_t index)
{
    KlcFile *klc = fn->filp;
    int len = strlen(name);
    uint16_t name_index = klc_add_str(klc, name, len);
    len = strlen(desc);
    uint16_t desc_index = klc_add_str(klc, desc, len);
    KlcArgument *arg = mm_alloc_obj(arg);
    arg->name_index = name_index;
    arg->type_index = desc_index;
    arg->const_index = index;
    vector_push_back(&fn->args, &arg);
    return 0;
}

KlcTypeParam *klc_func_add_tp(KlcFunc *fn, char *name)
{
    KlcFile *klc = fn->filp;
    int len = strlen(name);
    uint16_t name_index = klc_add_str(klc, name, len);
    KlcTypeParam *tp = mm_alloc_obj(tp);
    tp->name_index = name_index;

    vector_init(&tp->bounds, sizeof(uint16_t));
    uint16_t empty_index = 0;
    vector_push_back(&tp->bounds, &empty_index);

    vector_push_back(&fn->tps, &tp);
    return tp;
}

int klc_func_add_ann(KlcFunc *fn, char *name, char *key, char *value)
{
    KlcFile *klc = fn->filp;
    int len = strlen(name);
    uint16_t name_index = klc_add_str(klc, name, len);
    len = key ? strlen(key) : 0;
    uint16_t key_index = klc_add_str(klc, key, len);
    len = value ? strlen(value) : 0;
    uint16_t value_index = klc_add_str(klc, value, len);
    KlcAnnot *ann = mm_alloc_obj(ann);
    ann->name_index = name_index;
    ann->key_index = key_index;
    ann->value_index = value_index;
    vector_push_back(&fn->anns, &ann);
    return 0;
}

KlcKlass *klc_add_klass(KlcFile *klc, char *name, int flags)
{
    int len = strlen(name);
    uint16_t name_index = klc_add_str(klc, name, len);
    KlcKlass *kls = mm_alloc_obj(kls);
    kls->filp = klc;
    kls->flags = flags;
    kls->name_index = name_index;
    vector_init_ptr(&kls->tps);
    vector_init_ptr(&kls->anns);
    vector_init(&kls->bases, sizeof(uint16_t));
    vector_init(&kls->pip, sizeof(uint16_t));
    vector_init(&kls->lro, sizeof(uint16_t));
    vector_init_ptr(&kls->fields);
    vector_init_ptr(&kls->methods);
    void *empty = NULL;
    vector_push_back(&kls->tps, &empty);
    vector_push_back(&kls->anns, &empty);
    vector_push_back(&kls->fields, &empty);
    vector_push_back(&kls->methods, &empty);
    uint16_t _empty = 0;
    vector_push_back(&kls->bases, &_empty);
    vector_push_back(&kls->pip, &_empty);
    vector_push_back(&kls->lro, &_empty);
    vector_push_back(klc->objs + ITEM_CLASS, &kls);
    return kls;
}

KlcTypeParam *klc_klass_add_tp(KlcKlass *kls, char *name)
{
    KlcFile *klc = kls->filp;
    KlcTypeParam *tp = mm_alloc_obj(tp);
    int len = strlen(name);
    tp->name_index = klc_add_str(klc, name, len);

    vector_init(&tp->bounds, sizeof(uint16_t));
    uint16_t empty_index = 0;
    vector_push_back(&tp->bounds, &empty_index);

    vector_push_back(&kls->tps, &tp);
    return tp;
}

KlcFunc *klc_klass_add_func(KlcKlass *kls, char *name, char *ret_desc, int flags)
{
    KlcFile *klc = kls->filp;
    KlcFunc *fn = new_func(klc, name, ret_desc, flags);
    vector_push_back(&kls->methods, &fn);
    return fn;
}

KlcVar *klc_klass_add_field(KlcKlass *kls, char *name, char *type, int flags)
{
    KlcFile *klc = kls->filp;
    int len = strlen(name);
    uint16_t name_index = klc_add_str(klc, name, len);
    len = strlen(type);
    uint16_t type_index = klc_add_str(klc, type, len);
    KlcVar *var = mm_alloc_obj(var);
    var->flags = flags;
    var->name_index = name_index;
    var->type_index = type_index;
    var->const_index = 0;
    vector_push_back(&kls->fields, &var);
    return var;
}

static FILE *open_klc_file(const char *path, char *mode)
{
    FILE *fp = fopen(path, mode);
    if (fp == NULL) {
        char *end = strrchr(path, '/');
        if (!end) return NULL;

        /* path contains directories, create them */
        BUF(cmd);
        buf_write_str(&cmd, "mkdir -p ");
        buf_write_nstr(&cmd, (char *)path, end - path);
        int status = system(BUF_STR(cmd));
        ASSERT(status == 0);
        FINI_BUF(cmd);

        /* reopen */
        fp = fopen(path, mode);
        if (fp == NULL) return NULL;
    }
    return fp;
}

static void write_bytes(KlcFile *klc, uint8_t *data, int len) { fwrite(data, 1, len, klc->filp); }
static void write_uint8(KlcFile *klc, uint8_t val) { fwrite(&val, 1, 1, klc->filp); }
static void write_uint16(KlcFile *klc, uint16_t val) { fwrite(&val, 1, 2, klc->filp); }
static void write_uint32(KlcFile *klc, uint32_t val) { fwrite(&val, 1, 4, klc->filp); }
static void write_uint64(KlcFile *klc, uint64_t val) { fwrite(&val, 1, 8, klc->filp); }
static void write_float(KlcFile *klc, double val) { fwrite(&val, 1, 8, klc->filp); }

static void write_const(KlcFile *klc, KlcConst *item)
{
    write_uint8(klc, (uint8_t)item->type);

    switch (item->type) {
        case KLC_CONST_NONE: {
            break;
        }
        case KLC_CONST_INT: {
            write_uint8(klc, (uint8_t)item->len);
            write_uint8(klc, (uint8_t)item->sign);
            write_uint8(klc, (uint8_t)item->type_info);
            if (item->len == 1) {
                write_uint8(klc, (uint8_t)item->ival);
            } else if (item->len == 2) {
                write_uint16(klc, (uint16_t)item->ival);
            } else if (item->len == 4) {
                write_uint32(klc, (uint32_t)item->ival);
            } else if (item->len == 8) {
                write_uint64(klc, (uint64_t)item->ival);
            } else {
                UNREACHABLE();
            }
            break;
        }
        case KLC_CONST_FLT: {
            write_uint8(klc, (uint8_t)item->len);
            write_uint8(klc, (uint8_t)item->type_info);
            write_float(klc, item->fval);
            break;
        }
        case KLC_CONST_SHORT_ASCII:
        case KLC_CONST_SHORT_UTF8: {
            write_uint8(klc, (uint8_t)item->len);
            write_bytes(klc, item->sval, item->len);
            break;
        }
        case KLC_CONST_ASCII:
        case KLC_CONST_UTF8: {
            write_uint32(klc, (uint32_t)item->len);
            write_bytes(klc, item->sval, item->len);
            break;
        }
        case KLC_CONST_SHORT_TUPLE: {
            Vector *vec = item->val;
            int size = vector_size(vec);
            write_uint8(klc, (uint8_t)size);

            uint16_t item;
            vector_foreach(item, vec) {
                if (!item) continue;
                write_uint16(klc, item);
            }
            break;
        }
        case KLC_CONST_TUPLE: {
            Vector *vec = item->val;
            int size = vector_size(vec);
            write_uint32(klc, (uint32_t)size);

            uint16_t item;
            vector_foreach(item, vec) {
                if (!item) continue;
                write_uint16(klc, item);
            }
            break;
        }
        case KLC_CONST_OBJECT: {
            break;
        }
        default: {
            break;
        }
    }
}

static void write_consts(KlcFile *klc, Vector *vec)
{
    size_t size = vector_size(vec) - 1;
    write_uint16(klc, (uint16_t)size);

    KlcConst *item;
    vector_foreach(item, vec) {
        if (!item) continue;
        write_const(klc, item);
    }
}

static void write_vars(KlcFile *klc, Vector *vec)
{
    size_t size = vector_size(vec) - 1;
    write_uint16(klc, (uint16_t)size);
    KlcVar *item;
    vector_foreach(item, vec) {
        if (!item) continue;
        write_uint16(klc, item->flags);
        write_uint16(klc, item->name_index);
        write_uint16(klc, item->type_index);
        write_uint16(klc, item->const_index);
    }
}

static void write_args(KlcFile *klc, Vector *vec)
{
    size_t size = vector_size(vec) - 1;
    write_uint8(klc, (uint8_t)size);
    KlcArgument *item;
    vector_foreach(item, vec) {
        if (!item) continue;
        write_uint16(klc, item->name_index);
        write_uint16(klc, item->type_index);
        write_uint16(klc, item->const_index);
    }
}

static void write_tps(KlcFile *klc, Vector *vec)
{
    size_t size = vector_size(vec) - 1;
    write_uint8(klc, (uint8_t)size);

    KlcTypeParam *item;
    vector_foreach(item, vec) {
        if (!item) continue;

        write_uint16(klc, item->name_index);
        write_uint8(klc, item->which);

        size_t bsize = vector_size(&item->bounds) - 1;
        write_uint8(klc, (uint8_t)bsize);

        uint16_t bitem;
        vector_foreach(bitem, &item->bounds) {
            if (!bitem) continue;
            write_uint16(klc, bitem);
        }
    }
}

static void write_anns(KlcFile *klc, Vector *vec)
{
    size_t size = vector_size(vec) - 1;
    write_uint8(klc, (uint8_t)size);
    KlcAnnot *item;
    vector_foreach(item, vec) {
        if (!item) continue;
        write_uint16(klc, item->name_index);
        write_uint16(klc, item->key_index);
        write_uint16(klc, item->value_index);
    }
}

static void write_bases(KlcFile *klc, Vector *vec)
{
    size_t size = vector_size(vec) - 1;
    write_uint8(klc, (uint8_t)size);
    uint16_t item;
    vector_foreach(item, vec) {
        if (!item) continue;
        write_uint16(klc, item);
    }
}

static void write_pip(KlcFile *klc, Vector *vec)
{
    size_t size = vector_size(vec) - 1;
    write_uint8(klc, (uint8_t)size);
    uint16_t item;
    vector_foreach(item, vec) {
        if (!item) continue;
        write_uint16(klc, item);
    }
}

static void write_lro(KlcFile *klc, Vector *vec)
{
    size_t size = vector_size(vec) - 1;
    write_uint8(klc, (uint8_t)size);
    uint16_t item;
    vector_foreach(item, vec) {
        if (!item) continue;
        write_uint16(klc, item);
    }
}

static void write_funcs(KlcFile *klc, Vector *vec)
{
    size_t size = vector_size(vec) - 1;
    write_uint16(klc, (uint16_t)size);

    KlcFunc *item;
    vector_foreach(item, vec) {
        if (!item) continue;
        write_uint16(klc, item->flags);
        write_uint16(klc, item->name_index);
        write_uint16(klc, item->ret_type_index);
        write_uint16(klc, item->code_index);
        write_args(klc, &item->args);
        write_tps(klc, &item->tps);
        write_anns(klc, &item->anns);
    }
}

static void write_classes(KlcFile *klc, Vector *vec)
{
    size_t size = vector_size(vec) - 1;
    write_uint16(klc, (uint16_t)size);
    KlcKlass *item;
    vector_foreach(item, vec) {
        if (!item) continue;
        write_uint16(klc, item->flags);
        write_uint16(klc, item->name_index);
        write_tps(klc, &item->tps);
        write_bases(klc, &item->bases);
        write_pip(klc, &item->pip);
        write_lro(klc, &item->lro);
        write_vars(klc, &item->fields);
        write_funcs(klc, &item->methods);
    }
}

static void write_codes(KlcFile *klc, Vector *vec)
{
    uint32_t size = vector_size(vec) - 1;
    write_uint16(klc, (uint16_t)size);

    KlcCode *item;
    vector_foreach(item, vec) {
        if (!item) continue;
        write_uint16(klc, item->name_index);
        write_uint16(klc, item->nlocals);
        write_uint16(klc, item->max_call_args);
        write_uint32(klc, item->start_pc);
        write_uint32(klc, item->num_insns);
    }
}

static void write_imports(KlcFile *klc, Vector *vec)
{
    uint32_t size = vector_size(vec) - 1;
    write_uint16(klc, (uint16_t)size);

    KlcImport *item;
    vector_foreach(item, vec) {
        if (!item) continue;
        write_uint8(klc, item->kind);
        write_uint16(klc, item->ns_index);
        write_uint16(klc, item->sym_index);
    }
}

static void write_bytecodes(KlcFile *klc, Vector *vec)
{
    uint32_t size = vector_size(vec) - 1;
    write_uint16(klc, (uint16_t)size);

    KlcByteCode *item;
    vector_foreach(item, vec) {
        if (!item) continue;
        write_uint32(klc, item->size);
        fwrite(item->codes, 1, item->size, klc->filp);
    }
}

int write_klc_file(KlcFile *klc)
{
    FILE *fp = open_klc_file(klc->path, "w");
    klc->filp = fp;

    write_bytes(klc, klc->magic, 4);
    write_uint32(klc, klc->version);
    write_uint16(klc, klc->num_rt_consts);
    write_uint8(klc, klc->endian);
    write_uint8(klc, klc->padding);

    write_consts(klc, klc->objs + ITEM_RT_CONST);
    write_imports(klc, klc->objs + ITEM_IMPORT);
    write_codes(klc, klc->objs + ITEM_CODE);
    write_bytecodes(klc, klc->objs + ITEM_BYTECODE);
    write_consts(klc, klc->objs + ITEM_CONST);
    write_vars(klc, klc->objs + ITEM_VAR);
    write_funcs(klc, klc->objs + ITEM_FUNC);
    write_classes(klc, klc->objs + ITEM_CLASS);
    fclose(fp);
    return 0;
}

static void read_bytes(KlcFile *klc, uint8_t *data, int len) { fread(data, 1, len, klc->filp); }
static void read_uint8(KlcFile *klc, uint8_t *val) { fread(val, 1, 1, klc->filp); }
static void read_uint16(KlcFile *klc, uint16_t *val) { fread(val, 1, 2, klc->filp); }
static void read_uint32(KlcFile *klc, uint32_t *val) { fread(val, 1, 4, klc->filp); }
static void read_uint64(KlcFile *klc, uint64_t *val) { fread(val, 1, 8, klc->filp); }
static void read_float(KlcFile *klc, double *val) { fread(val, 1, 8, klc->filp); }

static void read_const(KlcFile *klc, Vector *vec)
{
    int type;
    int len;
    int sign = 0;
    uint64_t ival;
    char *sval;
    double fval;
    KlcConst *item;

    type = 0;
    read_uint8(klc, (uint8_t *)&type);
    item = mm_alloc_obj(item);
    item->type = type;
    vector_push_back(vec, &item);

    switch (type) {
        case KLC_CONST_NONE: {
            break;
        }
        case KLC_CONST_INT: {
            len = 0;
            ival = 0;
            read_uint8(klc, (uint8_t *)&len);
            item->len = len;
            read_uint8(klc, (uint8_t *)&sign);
            item->sign = sign;
            int type_info = 0;
            read_uint8(klc, (uint8_t *)&type_info);
            item->type_info = type_info;
            if (len == 1) {
                read_uint8(klc, (uint8_t *)&ival);
            } else if (len == 2) {
                read_uint16(klc, (uint16_t *)&ival);
            } else if (len == 4) {
                read_uint32(klc, (uint32_t *)&ival);
            } else if (len == 8) {
                read_uint64(klc, (uint64_t *)&ival);
            } else {
                UNREACHABLE();
            }
            item->ival = ival;
            break;
        }
        case KLC_CONST_FLT: {
            fval = 0.0;
            len = 0;
            read_uint8(klc, (uint8_t *)&len);
            item->len = len;
            int type_info = 0;
            read_uint8(klc, (uint8_t *)&type_info);
            item->type_info = type_info;
            item->sign = 1; /* float is always signed */
            read_float(klc, &fval);
            item->fval = fval;
            break;
        }
        case KLC_CONST_SHORT_ASCII:
        case KLC_CONST_SHORT_UTF8: {
            len = 0;
            read_uint8(klc, (uint8_t *)&len);
            char mem[256];
            read_bytes(klc, mem, len);
            item->len = len;
            item->sval = atom_nstr(mem, len);
            break;
        }
        case KLC_CONST_ASCII:
        case KLC_CONST_UTF8: {
            len = 0;
            read_uint32(klc, (uint32_t *)&len);
            sval = mm_alloc_fast(len + 1);
            read_bytes(klc, sval, len);
            sval[len] = 0;
            item->len = len;
            item->sval = atom_nstr(sval, len);
            mm_free(sval);
            break;
        }
        case KLC_CONST_SHORT_TUPLE: {
            len = 0;
            read_uint8(klc, (uint8_t *)&len);
            Vector *_vec = vector_create_ptr();
            for (int i = 0; i < len; i++) {
                int index = 0;
                read_uint16(klc, (uint16_t *)&index);
                KlcConst *kc = klc_get_rt_const(klc, index);
                vector_push_back(_vec, &kc);
            }
            item->len = len;
            item->val = _vec;
            break;
        }
        case KLC_CONST_TUPLE: {
            len = 0;
            read_uint32(klc, (uint32_t *)&len);
            Vector *_vec = vector_create_ptr();
            for (int i = 0; i < len; i++) {
                int index = 0;
                read_uint16(klc, (uint16_t *)&index);
                KlcConst *kc = klc_get_rt_const(klc, index);
                vector_push_back(_vec, &kc);
            }
            item->len = len;
            item->val = _vec;
            break;
        }
        default: {
            break;
        }
    }
}

static void read_consts(KlcFile *klc, Vector *vec)
{
    int size = 0;
    read_uint16(klc, (uint16_t *)&size);

    for (int i = 0; i < size; i++) {
        read_const(klc, vec);
    }
}

static void read_vars(KlcFile *klc, Vector *vec)
{
    int size = 0;
    read_uint16(klc, (uint16_t *)&size);

    KlcVar *var;
    for (int i = 0; i < size; i++) {
        var = mm_alloc_obj(var);
        vector_push_back(vec, &var);
        read_uint16(klc, &var->flags);
        read_uint16(klc, &var->name_index);
        read_uint16(klc, &var->type_index);
        read_uint16(klc, &var->const_index);
    }
}

static void read_args(KlcFile *klc, Vector *vec)
{
    int size = 0;
    read_uint8(klc, (uint8_t *)&size);

    KlcArgument *arg;
    for (int i = 0; i < size; i++) {
        arg = mm_alloc_obj(arg);
        vector_push_back(vec, &arg);
        read_uint16(klc, &arg->name_index);
        read_uint16(klc, &arg->type_index);
        read_uint16(klc, &arg->const_index);
    }
}

static void read_tps(KlcFile *klc, Vector *vec)
{
    int size = 0;
    read_uint8(klc, (uint8_t *)&size);

    int bsize = 0;
    KlcTypeParam *tp;
    for (int i = 0; i < size; i++) {
        tp = mm_alloc_obj(tp);
        vector_init(&tp->bounds, sizeof(uint16_t));
        uint16_t empty_index = 0;
        vector_push_back(&tp->bounds, &empty_index);
        vector_push_back(vec, &tp);

        read_uint16(klc, &tp->name_index);
        read_uint8(klc, &tp->which);
        read_uint8(klc, (uint8_t *)&bsize);
        for (int j = 0; j < bsize; j++) {
            uint16_t bitem = 0;
            read_uint16(klc, &bitem);
            vector_push_back(&tp->bounds, &bitem);
        }
    }
}

static void read_anns(KlcFile *klc, Vector *vec)
{
    int size = 0;
    read_uint8(klc, (uint8_t *)&size);

    KlcAnnot *ann;
    for (int i = 0; i < size; i++) {
        ann = mm_alloc_obj(ann);
        vector_push_back(vec, &ann);
        read_uint16(klc, &ann->name_index);
        read_uint16(klc, &ann->key_index);
        read_uint16(klc, &ann->value_index);
    }
}

static void read_bases(KlcFile *klc, Vector *vec)
{
    int size = 0;
    read_uint8(klc, (uint8_t *)&size);

    uint16_t item;
    for (int i = 0; i < size; i++) {
        read_uint16(klc, &item);
        vector_push_back(vec, &item);
    }
}

static void read_pip(KlcFile *klc, Vector *vec)
{
    int size = 0;
    read_uint8(klc, (uint8_t *)&size);

    uint16_t item;
    for (int i = 0; i < size; i++) {
        read_uint16(klc, &item);
        vector_push_back(vec, &item);
    }
}

static void read_lro(KlcFile *klc, Vector *vec)
{
    int size = 0;
    read_uint8(klc, (uint8_t *)&size);

    uint16_t item;
    for (int i = 0; i < size; i++) {
        read_uint16(klc, &item);
        vector_push_back(vec, &item);
    }
}

static void read_funcs(KlcFile *klc, Vector *vec)
{
    int size = 0;
    read_uint16(klc, (uint16_t *)&size);

    KlcFunc *fn;
    for (int i = 0; i < size; i++) {
        fn = mm_alloc_obj(fn);
        vector_init_ptr(&fn->args);
        vector_init_ptr(&fn->tps);
        vector_init_ptr(&fn->anns);
        vector_push_back(vec, &fn);
        read_uint16(klc, &fn->flags);
        read_uint16(klc, &fn->name_index);
        read_uint16(klc, &fn->ret_type_index);
        read_uint16(klc, &fn->code_index);
        void *empty = NULL;
        vector_push_back(&fn->args, &empty);
        vector_push_back(&fn->tps, &empty);
        vector_push_back(&fn->anns, &empty);
        read_args(klc, &fn->args);
        read_tps(klc, &fn->tps);
        read_anns(klc, &fn->anns);
    }
}

static void read_classes(KlcFile *klc, Vector *vec)
{
    int size = 0;
    read_uint16(klc, (uint16_t *)&size);

    KlcKlass *kls;
    for (int i = 0; i < size; i++) {
        kls = mm_alloc_obj(kls);
        vector_init_ptr(&kls->tps);
        vector_init_ptr(&kls->anns);
        vector_init(&kls->bases, sizeof(uint16_t));
        vector_init(&kls->pip, sizeof(uint16_t));
        vector_init(&kls->lro, sizeof(uint16_t));
        vector_init_ptr(&kls->fields);
        vector_init_ptr(&kls->methods);
        vector_push_back(vec, &kls);

        read_uint16(klc, &kls->flags);
        read_uint16(klc, &kls->name_index);

        void *empty = NULL;
        vector_push_back(&kls->tps, &empty);
        vector_push_back(&kls->anns, &empty);
        vector_push_back(&kls->fields, &empty);
        vector_push_back(&kls->methods, &empty);

        uint16_t _empty = 0;
        vector_push_back(&kls->bases, &_empty);
        vector_push_back(&kls->pip, &_empty);
        vector_push_back(&kls->lro, &_empty);

        read_tps(klc, &kls->tps);
        // read_anns(klc, &kls->anns);
        read_bases(klc, &kls->bases);
        read_pip(klc, &kls->pip);
        read_lro(klc, &kls->lro);
        read_vars(klc, &kls->fields);
        read_funcs(klc, &kls->methods);
    }
}

static void read_imports(KlcFile *klc, Vector *vec)
{
    int size = 0;
    read_uint16(klc, (uint16_t *)&size);

    KlcImport *imp;
    for (int i = 0; i < size; i++) {
        imp = mm_alloc_obj(imp);
        vector_push_back(vec, &imp);
        read_uint8(klc, &imp->kind);
        read_uint16(klc, &imp->ns_index);
        read_uint16(klc, &imp->sym_index);
    }
}

static void read_codes(KlcFile *klc, Vector *vec)
{
    int size = 0;
    read_uint16(klc, (uint16_t *)&size);

    KlcCode *code;
    for (int i = 0; i < size; i++) {
        code = mm_alloc_obj(code);
        vector_push_back(vec, &code);
        read_uint16(klc, &code->name_index);
        read_uint16(klc, &code->nlocals);
        read_uint16(klc, &code->max_call_args);
        read_uint32(klc, &code->start_pc);
        read_uint32(klc, &code->num_insns);
    }
}

static void read_bytecodes(KlcFile *klc, Vector *vec)
{
    int size = 0;
    read_uint16(klc, (uint16_t *)&size);

    KlcByteCode *bc;
    for (int i = 0; i < size; i++) {
        bc = mm_alloc_obj(bc);
        vector_push_back(vec, &bc);
        read_uint32(klc, &bc->size);
        uint8_t *data = mm_alloc(bc->size);
        fread(data, 1, bc->size, klc->filp);
        bc->codes = data;
    }
}

static int check_header(KlcFile *klc)
{
    if (memcmp(klc->magic, "klc", 4) != 0) {
        fprintf(stderr, "Invalid klc file: magic mismatch\n");
        return -1;
    }

    if (klc->version != KOALA_VERSION) {
        fprintf(stderr, "klc version %u is not supported by this Koala version %u\n", klc->version,
                KOALA_VERSION);
        return -1;
    }

    return 0;
}

KlcFile *read_klc_file(char *path, int rt)
{
    FILE *fp = fopen(path, "r");
    if (!fp) return NULL;

    KlcFile *klc = mm_alloc_obj(klc);
    klc->filp = fp;
    hashmap_init(&klc->map, __item_entry_equal);
    void *empty = NULL;
    for (int i = 0; i < ITEM_MAX; i++) {
        vector_init_ptr(klc->objs + i);
        vector_push_back(klc->objs + i, &empty);
    }

    read_bytes(klc, klc->magic, 4);
    read_uint32(klc, &klc->version);
    read_uint16(klc, &klc->num_rt_consts);
    read_uint8(klc, &klc->endian);
    read_uint8(klc, &klc->padding);

    if (check_header(klc)) {
        fclose(fp);
        free_klc_file(klc);
        return NULL;
    }

    read_consts(klc, klc->objs + ITEM_RT_CONST);
    read_imports(klc, klc->objs + ITEM_IMPORT);
    read_codes(klc, klc->objs + ITEM_CODE);
    read_bytecodes(klc, klc->objs + ITEM_BYTECODE);

    if (!rt) {
        read_consts(klc, klc->objs + ITEM_CONST);
        read_vars(klc, klc->objs + ITEM_VAR);
        read_funcs(klc, klc->objs + ITEM_FUNC);
        read_classes(klc, klc->objs + ITEM_CLASS);
    }

    fclose(fp);

    return klc;
}

void init_klc_file(KlcFile *klc, const char *path)
{
    memcpy(klc->magic, "klc", 4);
    klc->version = KOALA_VERSION;
    klc->num_rt_consts = 0;
    klc->endian = 0;
    klc->padding = 0;
    klc->path = path;
    klc->filp = NULL;
    hashmap_init(&klc->map, __item_entry_equal);
    void *empty = NULL;
    for (int i = 0; i < ITEM_MAX; i++) {
        vector_init_ptr(klc->objs + i);
        vector_push_back(klc->objs + i, &empty);
    }
}

static void fini_consts(Vector *vec)
{
    KlcConst *item;
    vector_foreach(item, vec) {
        if (!item) continue;
        mm_free(item);
    }

    vector_fini(vec);
}

static void fini_vars(Vector *vec)
{
    KlcVar *item;
    vector_foreach(item, vec) {
        if (!item) continue;
        mm_free(item);
    }

    vector_fini(vec);
}

static void fini_func(KlcFunc *fn)
{
    KlcArgument *arg;
    vector_foreach(arg, &fn->args) {
        if (!arg) continue;
        mm_free(arg);
    }
    vector_fini(&fn->args);

    KlcTypeParam *tp;
    vector_foreach(tp, &fn->tps) {
        if (!tp) continue;
        vector_fini(&tp->bounds);
        mm_free(tp);
    }
    vector_fini(&fn->tps);

    KlcAnnot *ann;
    vector_foreach(ann, &fn->anns) {
        if (!ann) continue;
        mm_free(ann);
    }
    vector_fini(&fn->anns);

    mm_free(fn);
}

static void fini_funcs(Vector *vec)
{
    KlcFunc *item;
    vector_foreach(item, vec) {
        if (!item) continue;
        fini_func(item);
    }

    vector_fini(vec);
}

static void fini_klass(KlcKlass *kls)
{
    KlcTypeParam *tp;
    vector_foreach(tp, &kls->tps) {
        if (!tp) continue;
        vector_fini(&tp->bounds);
        mm_free(tp);
    }
    vector_fini(&kls->tps);

    KlcAnnot *ann;
    vector_foreach(ann, &kls->anns) {
        if (!ann) continue;
        mm_free(ann);
    }
    vector_fini(&kls->anns);

    vector_fini(&kls->bases);
    vector_fini(&kls->pip);
    vector_fini(&kls->lro);

    KlcVar *field;
    vector_foreach(field, &kls->fields) {
        if (!field) continue;
        mm_free(field);
    }
    vector_fini(&kls->fields);

    KlcFunc *method;
    vector_foreach(method, &kls->methods) {
        if (!method) continue;
        fini_func(method);
    }
    vector_fini(&kls->methods);

    mm_free(kls);
}

static void fini_klasses(Vector *vec)
{
    KlcKlass *item;
    vector_foreach(item, vec) {
        if (!item) continue;
        fini_klass(item);
    }

    vector_fini(vec);
}

static void fini_imports(Vector *vec)
{
    KlcImport *item;
    vector_foreach(item, vec) {
        if (!item) continue;
        mm_free(item);
    }

    vector_fini(vec);
}

static void fini_codes(Vector *vec)
{
    KlcCode *item;
    vector_foreach(item, vec) {
        if (!item) continue;
        mm_free(item);
    }

    vector_fini(vec);
}

static void fini_bytecodes(Vector *vec)
{
    KlcByteCode *item;
    vector_foreach(item, vec) {
        if (!item) continue;
        mm_free(item->codes);
        mm_free(item);
    }

    vector_fini(vec);
}

void fini_klc_file(KlcFile *klc)
{
    hashmap_fini(&klc->map, __item_entry_free, NULL);

    fini_consts(klc->objs + ITEM_RT_CONST);
    fini_imports(klc->objs + ITEM_IMPORT);
    fini_codes(klc->objs + ITEM_CODE);
    fini_bytecodes(klc->objs + ITEM_BYTECODE);

    fini_consts(klc->objs + ITEM_CONST);
    fini_vars(klc->objs + ITEM_VAR);
    fini_funcs(klc->objs + ITEM_FUNC);
    fini_klasses(klc->objs + ITEM_CLASS);
}

void free_klc_file(KlcFile *klc)
{
    fini_klc_file(klc);
    mm_free(klc);
}

KlcConst *klc_get_const(KlcFile *klc, uint16_t index)
{
    Vector *consts = klc->objs + ITEM_CONST;
    KlcConst *k = vector_get(consts, index);
    return k;
}

KlcConst *klc_get_rt_const(KlcFile *klc, uint16_t index)
{
    Vector *consts = klc->objs + ITEM_RT_CONST;
    KlcConst *k = vector_get(consts, index);
    return k;
}

uint32_t klc_get_bytecodes(KlcFile *klc, uint8_t **codes)
{
    Vector *vec = klc->objs + ITEM_BYTECODE;
    KlcByteCode *item;
    vector_foreach(item, vec) {
        if (!item) continue;
        *codes = item->codes;
        return item->size;
    }
    return 0;
}

#ifdef __cplusplus
}
#endif

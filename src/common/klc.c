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

/* hash node for data is unique */
typedef struct _ItemEntry {
    /* hash entry */
    HashMapEntry entry;
    /* item type */
    int type;
    /* item data */
    void *data;
    /* index of vector */
    uint16_t index;
} ItemEntry;

static int klc_const_equal(KlcConst *k1, KlcConst *k2)
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
            return k1->fval == k2->fval;
        }
        case KLC_CONST_ASCII: // fall-through
        case KLC_CONST_UTF8: // fall-through
        case KLC_CONST_SHORT_UTF8: // fall-through
        case KLC_CONST_SHORT_ASCII: {
            if (k1->len != k2->len) return 0;
            return !strncmp(k1->sval, k2->sval, k1->len);
        }
        default: {
            UNREACHABLE();
        }
    }
}

static int klc_reloc_equal(KlcReloc *r1, KlcReloc *r2)
{
    return (r1->ns_index == r2->ns_index) && (r1->sym_index == r2->sym_index);
}

static int __item_entry_equal(void *n1, void *n2)
{
    ItemEntry *e1 = (ItemEntry *)n1;
    ItemEntry *e2 = (ItemEntry *)n2;
    if (e1->type != e2->type) return 0;
    if (e1->type == ITEM_CONST) {
        return klc_const_equal(e1->data, e2->data);
    } else if (e1->type == ITEM_RELOC) {
        return klc_reloc_equal(e1->data, e2->data);
    } else {
        UNREACHABLE();
    }
}

static unsigned int init_item_entry(ItemEntry *item, int type, void *data)
{
    unsigned int hash;
    int size = 0;

    if (type == ITEM_CONST) {
        size = sizeof(KlcConst);
    } else if (type == ITEM_RELOC) {
        size = sizeof(KlcReloc);
    } else {
        UNREACHABLE();
    }

    hash = mem_hash(data, size);
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

static uint16_t __append(KlcFile *klc, int type, void *data)
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

static uint16_t __const_get(KlcFile *klc, KlcConst *item)
{
    return __index(klc, ITEM_CONST, item);
}

uint16_t klc_add_none(KlcFile *klc)
{
    KlcConst k = { .type = KLC_CONST_NONE };
    uint16_t idx = __const_get(klc, &k);
    if (idx == 0) {
        KlcConst *item = mm_alloc_obj(item);
        item->type = KLC_CONST_NONE;
        idx = __append(klc, ITEM_CONST, item);
    }
    return idx;
}

uint16_t klc_add_int(KlcFile *klc, uint64_t val, int sign, int width)
{
    KlcConst k = { .type = KLC_CONST_INT, .sign = sign, .len = width, .ival = val };
    uint16_t idx = __const_get(klc, &k);
    if (idx == 0) {
        KlcConst *item = mm_alloc_obj(item);
        item->type = KLC_CONST_INT;
        item->sign = sign;
        item->len = width;
        item->ival = val;
        idx = __append(klc, ITEM_CONST, item);
    }
    return idx;
}

uint16_t klc_add_float(KlcFile *klc, double val)
{
    KlcConst k = { .type = KLC_CONST_FLT, .len = 0, .fval = val };
    uint16_t idx = __const_get(klc, &k);
    if (idx == 0) {
        KlcConst *item = mm_alloc_obj(item);
        item->type = KLC_CONST_FLT;
        item->len = 0;
        item->fval = val;
        idx = __append(klc, ITEM_CONST, item);
    }
    return idx;
}

uint16_t klc_add_str(KlcFile *klc, char *s, int len)
{
    if (len == 0) return 0;

    int type = STR_TYPE(len);
    char *_s = atom(s);
    KlcConst k = { .type = type, .len = len, .sval = _s };
    uint16_t idx = __const_get(klc, &k);
    if (idx == 0) {
        KlcConst *item = mm_alloc_obj(item);
        item->type = type;
        item->len = len;
        item->sval = _s;
        idx = __append(klc, ITEM_CONST, item);
    }
    return idx;
}

uint16_t klc_add_utf8(KlcFile *klc, char *s, int len)
{
    if (len == 0) return 0;

    int type = UTF8_TYPE(len);
    char *_s = atom(s);
    KlcConst k = { .type = type, .len = len, .sval = _s };
    uint16_t idx = __const_get(klc, &k);
    if (idx == 0) {
        KlcConst *item = mm_alloc_obj(item);
        item->type = type;
        item->len = len;
        item->sval = _s;
        idx = __append(klc, ITEM_CONST, item);
    }
    return idx;
}

uint16_t klc_add_code(KlcFile *klc, int num_locals, int code_size, char *codes)
{
    KlcCode *code = mm_alloc_obj(code);
    code->num_locals = num_locals;
    code->code_size = code_size;
    code->codes = mm_alloc(code_size);
    memcpy(code->codes, codes, code_size);
    vector_push_back(klc->objs + ITEM_CODE, &code);
    return vector_size(klc->objs + ITEM_CODE) - 1;
}

uint16_t klc_add_reloc(KlcFile *klc, char *ns, char *sym)
{
    uint16_t ns_index = 0;
    if (ns && ns[0]) {
        ns_index = klc_add_str(klc, ns, strlen(ns));
    }
    uint16_t sym_index = klc_add_str(klc, sym, strlen(sym));

    KlcReloc k = { ns_index, sym_index };
    uint16_t idx = __index(klc, ITEM_RELOC, &k);
    if (idx == 0) {
        KlcReloc *reloc = mm_alloc_obj(reloc);
        reloc->ns_index = ns_index;
        reloc->sym_index = sym_index;
        idx = __append(klc, ITEM_RELOC, reloc);
    }
    return idx;
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

int klc_func_add_tp(KlcFunc *fn, char *name, char *desc)
{
    KlcFile *klc = fn->filp;
    int len = strlen(name);
    uint16_t name_index = klc_add_str(klc, name, len);
    len = strlen(desc);
    uint16_t desc_index = klc_add_str(klc, desc, len);

    KlcTypeParam *tp = mm_alloc_obj(tp);
    tp->name_index = name_index;
    vector_init(&tp->bounds, sizeof(uint16_t));

    uint16_t empty_index = 0;
    vector_push_back(&tp->bounds, &empty_index);

    vector_push_back(&fn->tps, &tp);
    return 0;
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
    vector_init_ptr(&kls->bases);
    vector_init_ptr(&kls->fields);
    vector_init_ptr(&kls->methods);
    void *empty = NULL;
    vector_push_back(&kls->tps, &empty);
    vector_push_back(&kls->anns, &empty);
    vector_push_back(&kls->bases, &empty);
    vector_push_back(&kls->fields, &empty);
    vector_push_back(&kls->methods, &empty);
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

static FILE *open_klc_file(const char *path, char *mode)
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

static void write_bytes(KlcFile *klc, uint8_t *data, int len)
{
    fwrite(data, 1, len, klc->filp);
}
static void write_uint8(KlcFile *klc, uint8_t val) { fwrite(&val, 1, 1, klc->filp); }
static void write_uint16(KlcFile *klc, uint16_t val) { fwrite(&val, 2, 1, klc->filp); }
static void write_uint32(KlcFile *klc, uint32_t val) { fwrite(&val, 4, 1, klc->filp); }
static void write_uint64(KlcFile *klc, uint64_t val) { fwrite(&val, 8, 1, klc->filp); }
static void write_float(KlcFile *klc, double val) { fwrite(&val, 8, 1, klc->filp); }
static void write_opcodes(KlcFile *klc, uint16_t size, char *opcodes)
{
    fwrite(opcodes, size, 1, klc->filp);
}

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

            KlcConst *item;
            vector_foreach(item, vec) {
                if (!item) continue;
                write_const(klc, item);
            }
            break;
        }
        case KLC_CONST_TUPLE: {
            Vector *vec = item->val;
            int size = vector_size(vec);
            write_uint32(klc, (uint32_t)size);

            KlcConst *item;
            vector_foreach(item, vec) {
                if (!item) continue;
                write_const(klc, item);
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
    vector_foreach_object(item, vec) {
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
    vector_foreach_object(item, vec) {
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
    vector_foreach_object(item, vec) {
        write_uint16(klc, item->name_index);

        size_t bsize = vector_size(&item->bounds) - 1;
        write_uint8(klc, (uint8_t)bsize);

        uint16_t bitem;
        vector_foreach_object(bitem, &item->bounds) {
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
    vector_foreach_object(item, vec) {
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
    vector_foreach_object(item, vec) {
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
    KlcKlass **item_p;
    KlcKlass *item;
    vector_foreach_object(item, vec) {
        write_uint16(klc, item->flags);
        write_uint16(klc, item->name_index);
        write_tps(klc, &item->tps);
        write_bases(klc, &item->bases);
        write_vars(klc, &item->fields);
        write_funcs(klc, &item->methods);
    }
}

static void write_relocs(KlcFile *klc, Vector *vec)
{
    size_t size = vector_size(vec) - 1;
    write_uint16(klc, (uint16_t)size);

    KlcReloc *item;
    vector_foreach(item, vec) {
        if (!item) continue;
        write_uint16(klc, item->ns_index);
        write_uint16(klc, item->sym_index);
    }
}

static void write_codes(KlcFile *klc, Vector *vec)
{
    size_t size = vector_size(vec) - 1;
    write_uint16(klc, (uint16_t)size);

    KlcCode *item;
    vector_foreach(item, vec) {
        if (!item) continue;
        write_uint16(klc, item->num_locals);
        write_uint16(klc, item->code_size);
        write_opcodes(klc, item->code_size, item->codes);
    }
}

int write_klc_file(KlcFile *klc)
{
    FILE *fp = open_klc_file(klc->path, "w");
    klc->filp = fp;
    write_bytes(klc, klc->magic, 4);
    write_uint32(klc, klc->version);
    write_consts(klc, klc->objs + ITEM_CONST);
    write_vars(klc, klc->objs + ITEM_VAR);
    write_funcs(klc, klc->objs + ITEM_FUNC);
    write_classes(klc, klc->objs + ITEM_CLASS);
    write_relocs(klc, klc->objs + ITEM_RELOC);
    write_codes(klc, klc->objs + ITEM_CODE);
    fclose(fp);
    return 0;
}

static void read_bytes(KlcFile *klc, uint8_t *data, int len)
{
    fread(data, 1, len, klc->filp);
}
static void read_uint8(KlcFile *klc, uint8_t *val) { fread(val, 1, 1, klc->filp); }
static void read_uint16(KlcFile *klc, uint16_t *val) { fread(val, 2, 1, klc->filp); }
static void read_uint32(KlcFile *klc, uint32_t *val) { fread(val, 4, 1, klc->filp); }
static void read_uint64(KlcFile *klc, uint64_t *val) { fread(val, 8, 1, klc->filp); }
static void read_float(KlcFile *klc, double *val) { fread(val, 8, 1, klc->filp); }
static void read_opcodes(KlcFile *klc, uint16_t size, char *opcodes)
{
    fread(opcodes, size, 1, klc->filp);
}

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
            read_float(klc, &fval);
            item->fval = fval;
            break;
        }
        case KLC_CONST_SHORT_ASCII:
        case KLC_CONST_SHORT_UTF8: {
            len = 0;
            read_uint8(klc, (uint8_t *)&len);
            sval = mm_alloc_fast(len + 1);
            read_bytes(klc, sval, len);
            sval[len] = 0;
            item->len = len;
            item->sval = sval;
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
            item->sval = sval;
            break;
        }
        case KLC_CONST_SHORT_TUPLE: {
            len = 0;
            read_uint8(klc, (uint8_t *)&len);
            Vector *vec2 = vector_create_ptr();
            for (int i = 0; i < len; i++) {
                read_const(klc, vec2);
            }
            item->len = len;
            item->val = vec2;
            break;
        }
        case KLC_CONST_TUPLE: {
            len = 0;
            read_uint32(klc, (uint32_t *)&len);
            Vector *vec2 = vector_create_ptr();
            for (int i = 0; i < len; i++) {
                read_const(klc, vec2);
            }
            item->len = len;
            item->val = vec2;
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
        vector_init_ptr(&kls->bases);
        vector_init_ptr(&kls->fields);
        vector_init_ptr(&kls->methods);
        vector_push_back(vec, &kls);
        read_uint16(klc, &kls->flags);
        read_uint16(klc, &kls->name_index);
        void *empty = NULL;
        vector_push_back(&kls->tps, &empty);
        vector_push_back(&kls->anns, &empty);
        vector_push_back(&kls->bases, &empty);
        vector_push_back(&kls->fields, &empty);
        vector_push_back(&kls->methods, &empty);
        read_tps(klc, &kls->tps);
        // read_anns(klc, &kls->anns);
        read_bases(klc, &kls->bases);
        read_vars(klc, &kls->fields);
        read_funcs(klc, &kls->methods);
    }
}

static void read_relocs(KlcFile *klc, Vector *vec)
{
    int size = 0;
    read_uint16(klc, (uint16_t *)&size);
    KlcReloc *reloc;
    for (int i = 0; i < size; i++) {
        reloc = mm_alloc_obj(reloc);
        vector_push_back(vec, &reloc);
        read_uint16(klc, &reloc->ns_index);
        read_uint16(klc, &reloc->sym_index);
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
        read_uint16(klc, &code->num_locals);
        read_uint16(klc, &code->code_size);
        code->codes = mm_alloc(code->code_size);
        read_opcodes(klc, code->code_size, code->codes);
    }
}

int read_klc_file(KlcFile *klc, int all)
{
    FILE *fp = open_klc_file(klc->path, "r");
    klc->filp = fp;
    read_bytes(klc, klc->magic, 4);
    read_uint32(klc, &klc->version);
    read_consts(klc, klc->objs + ITEM_CONST);
    read_vars(klc, klc->objs + ITEM_VAR);
    read_funcs(klc, klc->objs + ITEM_FUNC);
    read_classes(klc, klc->objs + ITEM_CLASS);
    if (all) {
        read_relocs(klc, klc->objs + ITEM_RELOC);
        read_codes(klc, klc->objs + ITEM_CODE);
    }
    fclose(fp);
    return 0;
}

void init_klc_file(KlcFile *klc, const char *path)
{
    memcpy(klc->magic, "klc", 4);
    klc->version = KOALA_VERSION;
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

static void fini_funcs(Vector *vec)
{
    KlcConst *item;
    vector_foreach(item, vec) {
        if (!item) continue;
    }

    vector_fini(vec);
}

static void fini_class(Vector *vec)
{
    KlcConst *item;
    vector_foreach(item, vec) {
        if (!item) continue;
    }

    vector_fini(vec);
}

static void fini_relocs(Vector *vec)
{
    KlcConst *item;
    vector_foreach(item, vec) {
        if (!item) continue;
    }

    vector_fini(vec);
}

static void fini_codes(Vector *vec)
{
    KlcConst *item;
    vector_foreach(item, vec) {
        if (!item) continue;
    }

    vector_fini(vec);
}

void fini_klc_file(KlcFile *klc)
{
    hashmap_fini(&klc->map, NULL, NULL);
    fini_consts(klc->objs + ITEM_CONST);
    fini_vars(klc->objs + ITEM_VAR);
    fini_funcs(klc->objs + ITEM_FUNC);
    fini_class(klc->objs + ITEM_CLASS);
    fini_relocs(klc->objs + ITEM_RELOC);
    fini_codes(klc->objs + ITEM_CODE);
}

KlcConst *klc_get_const(KlcFile *klc, uint16_t index)
{
    Vector *consts = klc->objs + ITEM_CONST;
    KlcConst **k = vector_get(consts, index);
    return (k != NULL) ? *k : NULL;
}

#ifdef __cplusplus
}
#endif

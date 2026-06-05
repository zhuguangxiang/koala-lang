/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "cgen.h"
#include "cmd.h"
#include "codebuffer.h"
#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif

static void dump_const(KlMachConst *kc, int index, int indent)
{
    switch (kc->tag) {
        case KL_MACH_CONST_NONE:
            printf("none\n");
            break;

        case KL_MACH_CONST_INT:
            printf("int%d   = %ld\n", kc->len * 8, kc->i64);
            break;

        case KL_MACH_CONST_UINT:
            printf("uint%d  = %lu\n", kc->len * 8, kc->u64);
            break;

        case KL_MACH_CONST_FLOAT:
            printf("float%d = %.17g\n", kc->len * 8, kc->f64);
            break;

        case KL_MACH_CONST_BOOL:
            printf("bool    = %s\n", kc->bval ? "true" : "false");
            break;

        case KL_MACH_CONST_STR: {
            printf("str     = \"");
            if (kc->len == 0) {
                printf("\"\n");
                break;
            }
            BUF(buf);
            escape_str(kc->str, &buf);
            printf("%s\"\n", BUF_STR(buf));
            FINI_BUF(buf);
            break;
        }

        case KL_MACH_CONST_TUPLE: {
            Vector *list = kc->list;
            int count = vector_size(list);

            printf("tuple(%d):\n", count);

            for (int i = 0; i < count; i++) {
                KlMachConst *elem = vector_at(list, i);
                printf("%*s[%d] ", indent + 4, "", i);
                dump_const(elem, i, indent + 8);
            }
            break;
        }

        case KL_MACH_CONST_RANGE: {
            Vector *list = kc->list;
            int count = vector_size(list);

            printf("range: (");

            for (int i = 0; i < count; i++) {
                KlMachConst *elem = vector_at(list, i);
                if (i > 0) printf(", ");
                printf("%ld", elem->i64);
            }

            printf(")\n");
            break;
        }

        case KL_MACH_CONST_LIST: {
            Vector *list = kc->list;
            int count = vector_size(list);

            printf("list(%d): \n", count);

            for (int i = 0; i < count; i++) {
                KlMachConst *elem = vector_at(list, i);
                printf("%*s[%d] ", indent + 4, "", i);
                dump_const(elem, i, indent + 8);
            }

            break;
        }

        default: {
            printf("unknown\n");
            break;
        }
    }
}

static void dump_const_pool(KlMachModule *m)
{
    printf("Constant Pool:\n");
    KlMachConst *kc;
    vector_foreach(kc, &m->const_pool) {
        printf("%*s[%03d] ", 2, "", i__);
        dump_const(kc, i__, 2);
    }
}

static void dump_import_table(KlMachModule *m)
{
    printf("Import Table:\n");
    KlMachImport *imp;
    vector_foreach(imp, &m->import_table) {
        if (imp->kind == IMPORT_FUNC) {
            printf("  #%d: func   %s::%s\n", i__, imp->path, imp->name);
        } else if (imp->kind == IMPORT_GLOBAL) {
            printf("  #%d: global %s::%s\n", i__, imp->path, imp->name);
        } else if (imp->kind == IMPORT_TYPE) {
            printf("  #%d: type   %s::%s\n", i__, imp->path, imp->name);
        } else if (imp->kind == IMPORT_METHOD) {
            printf("  #%d: method %s::%s::%s\n", i__, imp->path, imp->klass, imp->name);
        } else if (imp->kind == IMPORT_FIELD) {
            printf("  #%d: field  %s::%s::%s\n", i__, imp->path, imp->klass, imp->name);
        } else {
            UNREACHABLE();
        }
    }
}

static int __mach_const_eq__(void *a, void *b)
{
    KlMachConst *ka = (KlMachConst *)a;
    KlMachConst *kb = (KlMachConst *)b;
    if (ka->tag != kb->tag) return 0;
    switch (ka->tag) {
        case KL_MACH_CONST_NONE:
            return 1;
        case KL_MACH_CONST_INT:
            return (ka->len == kb->len) && (ka->i64 == kb->i64);
        case KL_MACH_CONST_UINT:
            return (ka->len == kb->len) && (ka->u64 == kb->u64);
        case KL_MACH_CONST_FLOAT:
            return (ka->len == kb->len) && (ka->f64 == kb->f64);
        case KL_MACH_CONST_STR:
            return (ka->len == kb->len) && (strcmp(ka->str, kb->str) == 0);
        case KL_MACH_CONST_BOOL:
            return (ka->len == kb->len) && (ka->bval == kb->bval);
        case KL_MACH_CONST_RANGE: {
            KlMachConst **raw_a = VECTOR_RAW(ka->list, KlMachConst *);
            KlMachConst **raw_b = VECTOR_RAW(kb->list, KlMachConst *);
            return !memcmp(raw_a, raw_b, sizeof(void *) * 3);
        }
        case KL_MACH_CONST_TUPLE: {
            Vector *list_a = ka->list;
            Vector *list_b = kb->list;
            int count = vector_size(list_a);
            if (count != vector_size(list_b)) return 0;
            for (int i = 0; i < count; i++) {
                KlMachConst *elem_a = vector_at(list_a, i);
                KlMachConst *elem_b = vector_at(list_b, i);
                if (!__mach_const_eq__(elem_a, elem_b)) return 0;
            }
            return 1;
        }
        default:
            UNREACHABLE();
    }
}

static unsigned int mach_const_hash(void *key)
{
    KlMachConst *kc = (KlMachConst *)key;
    switch (kc->tag) {
        case KL_MACH_CONST_NONE:
            return 0;
        case KL_MACH_CONST_INT:
            return mem_hash(&kc->i64, sizeof(kc->i64));
        case KL_MACH_CONST_UINT:
            return mem_hash(&kc->u64, sizeof(kc->u64));
        case KL_MACH_CONST_FLOAT: {
            union {
                double f;
                uint64_t u;
            } u = { .f = kc->f64 };
            return mem_hash(&u, sizeof(u));
        }
        case KL_MACH_CONST_BOOL:
            return mem_hash(&kc->bval, sizeof(kc->bval));
        case KL_MACH_CONST_STR:
            return str_hash(kc->str);
        case KL_MACH_CONST_RANGE:
            KlMachConst **raw = VECTOR_RAW(kc->list, KlMachConst *);
            return mem_hash(raw, sizeof(void *) * 3);
        case KL_MACH_CONST_TUPLE: {
            Vector *list = kc->list;
            int count = vector_size(list);
            unsigned int h = 0;
            for (int i = 0; i < count; i++) {
                KlMachConst *elem = vector_at(list, i);
                h ^= mach_const_hash(elem) + 0x9e3779b9 + (h << 6) + (h >> 2);
            }
            return h;
        }
        default:
            UNREACHABLE();
    }
}

static KlMachConst *kl_mach_add_none(KlMachModule *m)
{
    KlMachConst key = { .tag = KL_MACH_CONST_NONE, .len = 0 };
    hashmap_entry_init(&key, mach_const_hash(&key));

    KlMachConst *entry = hashmap_get(&m->cp_map, &key);
    if (entry) {
        log_info("Found existing const entry for none (index: %d)", entry->index);
        return entry;
    }

    KlMachConst *new_entry = mm_alloc_obj(new_entry);
    new_entry->tag = KL_MACH_CONST_NONE;
    new_entry->len = 0;
    hashmap_entry_init(new_entry, mach_const_hash(new_entry));
    hashmap_put(&m->cp_map, new_entry);
    vector_push_back(&m->const_pool, &new_entry);
    int index = vector_size(&m->const_pool) - 1;
    new_entry->index = index;
    log_info("Added new const entry for none (index: %d)", new_entry->index);
    return new_entry;
}

static KlMachConst *kl_mach_add_int(KlMachModule *m, int64_t v, int width)
{
    KlMachConst key = { .tag = KL_MACH_CONST_INT, .len = width, .i64 = v };
    hashmap_entry_init(&key, mach_const_hash(&key));

    KlMachConst *entry = hashmap_get(&m->cp_map, &key);
    if (entry) {
        log_info("Found existing const entry for int: %ld (index: %d)", v, entry->index);
        return entry;
    }

    KlMachConst *new_entry = mm_alloc_obj(new_entry);
    new_entry->tag = KL_MACH_CONST_INT;
    new_entry->len = width;
    new_entry->i64 = v;
    hashmap_entry_init(new_entry, mach_const_hash(new_entry));
    hashmap_put(&m->cp_map, new_entry);

    vector_push_back(&m->const_pool, &new_entry);
    new_entry->index = vector_size(&m->const_pool) - 1;
    log_info("Added new const entry for int: %ld (index: %d)", v, new_entry->index);
    return new_entry;
}

static KlMachConst *kl_mach_add_uint(KlMachModule *m, uint64_t v, int width)
{
    KlMachConst key = { .tag = KL_MACH_CONST_UINT, .len = width, .u64 = v };
    hashmap_entry_init(&key, mach_const_hash(&key));

    KlMachConst *entry = hashmap_get(&m->cp_map, &key);
    if (entry) {
        log_info("Found existing const entry for uint: %lu (index: %d)", v, entry->index);
        return entry;
    }

    KlMachConst *new_entry = mm_alloc_obj(new_entry);
    new_entry->tag = KL_MACH_CONST_UINT;
    new_entry->len = width;
    new_entry->u64 = v;
    hashmap_entry_init(new_entry, mach_const_hash(new_entry));
    hashmap_put(&m->cp_map, new_entry);
    vector_push_back(&m->const_pool, &new_entry);
    new_entry->index = vector_size(&m->const_pool) - 1;
    log_info("Added new const entry for uint: %lu (index: %d)", v, new_entry->index);
    return new_entry;
}

static KlMachConst *kl_mach_add_float(KlMachModule *m, double v, int width)
{
    KlMachConst key = { .tag = KL_MACH_CONST_FLOAT, .len = width, .f64 = v };
    hashmap_entry_init(&key, mach_const_hash(&key));

    KlMachConst *entry = hashmap_get(&m->cp_map, &key);
    if (entry) {
        log_info("Found existing const entry for float: %f (index: %d)", v, entry->index);
        return entry;
    }

    KlMachConst *new_entry = mm_alloc_obj(new_entry);
    new_entry->tag = KL_MACH_CONST_FLOAT;
    new_entry->len = width;
    new_entry->f64 = v;
    hashmap_entry_init(new_entry, mach_const_hash(new_entry));
    hashmap_put(&m->cp_map, new_entry);
    vector_push_back(&m->const_pool, &new_entry);
    int index = vector_size(&m->const_pool) - 1;
    new_entry->index = index;
    log_info("Added new const entry for float: %f (index: %d)", v, new_entry->index);
    return new_entry;
}

static KlMachConst *kl_mach_add_bool(KlMachModule *m, int val)
{
    KlMachConst key = { .tag = KL_MACH_CONST_BOOL, .len = 1, .bval = val };
    hashmap_entry_init(&key, mach_const_hash(&key));

    KlMachConst *entry = hashmap_get(&m->cp_map, &key);
    if (entry) {
        log_info("Found existing const entry for bool: %d (index: %d)", val, entry->index);
        return entry;
    }

    KlMachConst *new_entry = mm_alloc_obj(new_entry);
    new_entry->tag = KL_MACH_CONST_BOOL;
    new_entry->len = 1;
    new_entry->bval = val;
    hashmap_entry_init(new_entry, mach_const_hash(new_entry));
    hashmap_put(&m->cp_map, new_entry);
    vector_push_back(&m->const_pool, &new_entry);
    int index = vector_size(&m->const_pool) - 1;
    new_entry->index = index;
    log_info("Added new const entry for bool: %d (index: %d)", val, new_entry->index);
    return new_entry;
}

static KlMachConst *kl_mach_add_str(KlMachModule *m, char *v)
{
    int len = strlen(v);
    KlMachConst key = { .tag = KL_MACH_CONST_STR, .len = len, .str = v };
    hashmap_entry_init(&key, mach_const_hash(&key));

    KlMachConst *entry = hashmap_get(&m->cp_map, &key);
    if (entry) {
        log_info("Found existing const entry for string: %s (index: %d)", v, entry->index);
        return entry;
    }

    KlMachConst *new_entry = mm_alloc_obj(new_entry);
    new_entry->tag = KL_MACH_CONST_STR;
    new_entry->len = len;
    new_entry->str = v;
    hashmap_entry_init(new_entry, mach_const_hash(new_entry));
    hashmap_put(&m->cp_map, new_entry);
    vector_push_back(&m->const_pool, &new_entry);
    int index = vector_size(&m->const_pool) - 1;
    new_entry->index = index;
    log_info("Added new const entry for string: %s (index: %d)", v, index);
    return new_entry;
}

static KlMachConst *kl_mach_add_tuple(KlMachModule *m, Vector *items)
{
    Vector *list = vector_create_ptr();
    KlrConst *elem;
    vector_foreach(elem, items) {
        KlMachConst *kc = kl_mach_add_const(elem, m);
        vector_push_back(list, &kc);
    }

    KlMachConst key = { .tag = KL_MACH_CONST_TUPLE, .list = list };
    hashmap_entry_init(&key, mach_const_hash(&key));

    KlMachConst *entry = hashmap_get(&m->cp_map, &key);
    if (entry) {
        vector_destroy(list);
        log_info("Found existing const entry for tuple (index: %d)", entry->index);
        return entry;
    }

    KlMachConst *new_entry = mm_alloc_obj(new_entry);
    new_entry->tag = KL_MACH_CONST_TUPLE;
    new_entry->list = list;

    hashmap_entry_init(new_entry, mach_const_hash(new_entry));
    hashmap_put(&m->cp_map, new_entry);

    vector_push_back(&m->const_pool, &new_entry);
    int index = vector_size(&m->const_pool) - 1;
    new_entry->index = index;
    log_info("Added new const entry for tuple (index: %d)", index);
    return new_entry;
}

static KlMachConst *kl_mach_add_range(KlMachModule *m, Vector *items)
{
    Vector *list = vector_create_ptr();
    KlrValue *elem;
    vector_foreach(elem, items) {
        KlrConst *_kc = (KlrConst *)elem;
        KlMachConst *kc = kl_mach_add_int(m, _kc->ival, _kc->len);
        vector_push_back(list, &kc);
    }

    KlMachConst key = { .tag = KL_MACH_CONST_RANGE, .list = list };
    hashmap_entry_init(&key, mach_const_hash(&key));

    KlMachConst *entry = hashmap_get(&m->cp_map, &key);
    if (entry) {
        vector_destroy(list);
        log_info("Found existing const entry for range (index: %d)", entry->index);
        return entry;
    }

    KlMachConst *new_entry = mm_alloc_obj(new_entry);
    new_entry->tag = KL_MACH_CONST_RANGE;
    new_entry->list = list;

    hashmap_entry_init(new_entry, mach_const_hash(new_entry));
    hashmap_put(&m->cp_map, new_entry);

    vector_push_back(&m->const_pool, &new_entry);
    int index = vector_size(&m->const_pool) - 1;
    new_entry->index = index;
    log_info("Added new const entry for range (index: %d)", index);
    return new_entry;
}

static KlMachConst *kl_mach_add_list(KlMachModule *m, Vector *items)
{
    Vector *list = vector_create_ptr();
    KlrValue *elem;
    vector_foreach(elem, items) {
        KlMachConst *kc = kl_mach_add_const((KlrConst *)elem, m);
        vector_push_back(list, &kc);
    }

    KlMachConst *entry = mm_alloc_obj(entry);
    entry->tag = KL_MACH_CONST_LIST;
    entry->list = list;

    vector_push_back(&m->const_pool, &entry);
    int index = vector_size(&m->const_pool) - 1;
    entry->index = index;
    log_info("Added new const entry for list (index: %d)", index);
    return entry;
}

KlMachConst *kl_mach_add_const(KlrConst *kc, KlMachModule *m)
{
    switch (kc->which) {
        case CONST_NONE: {
            return kl_mach_add_none(m);
        }
        case CONST_INT: {
            return kl_mach_add_int(m, kc->ival, kc->len);
        }
        case CONST_UINT: {
            return kl_mach_add_uint(m, kc->ival, kc->len);
        }
        case CONST_FLT: {
            return kl_mach_add_float(m, kc->fval, kc->len);
        }
        case CONST_BOOL: {
            return kl_mach_add_bool(m, kc->bval);
        }
        case CONST_STR: {
            return kl_mach_add_str(m, kc->sval);
        }
        case CONST_TUPLE: {
            return kl_mach_add_tuple(m, kc->list);
        }
        case CONST_RANGE: {
            return kl_mach_add_range(m, kc->list);
        }
        case CONST_LIST: {
            return kl_mach_add_list(m, kc->list);
        }
        default: {
            UNREACHABLE();
            break;
        }
    }
}

static int __mach_import_eq__(void *a, void *b)
{
    KlMachImport *ia = (KlMachImport *)a;
    KlMachImport *ib = (KlMachImport *)b;
    if (ia->kind != ib->kind) return 0;
    if (ia->kind == IMPORT_FUNC || ia->kind == IMPORT_GLOBAL || ia->kind == IMPORT_TYPE) {
        return str_equal(ia->path, ib->path) && str_equal(ia->name, ib->name);
    } else if (ia->kind == IMPORT_FIELD || ia->kind == IMPORT_METHOD) {
        return str_equal(ia->path, ib->path) && str_equal(ia->klass, ib->klass) &&
               str_equal(ia->name, ib->name);
    } else {
        UNREACHABLE();
    }
}

static unsigned int mach_import_hash(void *key)
{
    KlMachImport *imp = (KlMachImport *)key;
    unsigned int h1 = str_hash(imp->path);
    unsigned int h2 = str_hash(imp->name);
    return h1 ^ h2;
}

static int mach_import_add_func(KlMachModule *m, char *path, char *name)
{
    KlMachImport key = { .kind = IMPORT_FUNC, .path = path, .name = name };
    hashmap_entry_init(&key, mach_import_hash(&key));

    KlMachImport *entry = hashmap_get(&m->import_map, &key);
    if (entry) {
        log_info("Found existing import ext-func entry for %s.%s (index: %d)", path, name,
                 entry->index);
        return entry->index;
    }

    KlMachImport *new_entry = mm_alloc_obj(new_entry);
    new_entry->kind = IMPORT_FUNC;
    new_entry->path = path;
    new_entry->name = name;
    hashmap_entry_init(new_entry, mach_import_hash(new_entry));
    hashmap_put(&m->import_map, new_entry);
    vector_push_back(&m->import_table, &new_entry);
    int import_index = vector_size(&m->import_table) - 1;
    new_entry->index = import_index;
    log_info("Added new import ext-func entry for %s.%s (index: %d)", path, name, import_index);
    return import_index;
}

int mach_import_add_field(KlMachModule *m, char *path, char *klass, char *name)
{
    KlMachImport key = { .kind = IMPORT_FIELD, .path = path, .klass = klass, .name = name };
    hashmap_entry_init(&key, mach_import_hash(&key));

    KlMachImport *entry = hashmap_get(&m->import_map, &key);
    if (entry) {
        log_info("Found existing import ext-field entry for %s.%s (index: %d)", path, name,
                 entry->index);
        return entry->index;
    }

    KlMachImport *new_entry = mm_alloc_obj(new_entry);
    new_entry->kind = IMPORT_FIELD;
    new_entry->path = path;
    new_entry->klass = klass;
    new_entry->name = name;
    hashmap_entry_init(new_entry, mach_import_hash(new_entry));
    hashmap_put(&m->import_map, new_entry);
    vector_push_back(&m->import_table, &new_entry);
    int import_index = vector_size(&m->import_table) - 1;
    new_entry->index = import_index;
    log_info("Added new import ext-field entry for %s.%s (index: %d)", path, name, import_index);
    return import_index;
}

static int mach_import_add_method(KlMachModule *m, char *path, char *klass, char *name)
{
    KlMachImport key = { .kind = IMPORT_METHOD, .path = path, .klass = klass, .name = name };
    hashmap_entry_init(&key, mach_import_hash(&key));

    KlMachImport *entry = hashmap_get(&m->import_map, &key);
    if (entry) {
        log_info("Found existing import ext-method entry for %s.%s.%s (index: %d)", path, klass,
                 name, entry->index);
        return entry->index;
    }

    KlMachImport *new_entry = mm_alloc_obj(new_entry);
    new_entry->kind = IMPORT_METHOD;
    new_entry->path = path;
    new_entry->klass = klass;
    new_entry->name = name;
    hashmap_entry_init(new_entry, mach_import_hash(new_entry));
    hashmap_put(&m->import_map, new_entry);
    vector_push_back(&m->import_table, &new_entry);
    int import_index = vector_size(&m->import_table) - 1;
    new_entry->index = import_index;
    log_info("Added new import ext-method entry for %s.%s.%s (index: %d)", path, klass, name,
             import_index);
    return import_index;
}

int mach_import_add_klass(KlMachModule *m, char *path, char *name)
{
    KlMachImport key = { .kind = IMPORT_TYPE, .path = path, .name = name };
    hashmap_entry_init(&key, mach_import_hash(&key));

    KlMachImport *entry = hashmap_get(&m->import_map, &key);
    if (entry) {
        log_info("Found existing import ext-klass entry for %s.%s (index: %d)", path, name,
                 entry->index);
        return entry->index;
    }

    KlMachImport *new_entry = mm_alloc_obj(new_entry);
    new_entry->kind = IMPORT_TYPE;
    new_entry->path = path;
    new_entry->name = name;
    hashmap_entry_init(new_entry, mach_import_hash(new_entry));
    hashmap_put(&m->import_map, new_entry);
    vector_push_back(&m->import_table, &new_entry);
    int import_index = vector_size(&m->import_table) - 1;
    new_entry->index = import_index;
    log_info("Added new import ext-klass entry for %s.%s (index: %d)", path, name, import_index);
    return import_index;
}

static void dump_func_byte_code(KlMachFunc *mfn, const uint8_t *code)
{
    bytecode_print((uint8_t *)code, (size_t)mfn->start_pc, (size_t)mfn->total_insns);
}

static void dump_byte_code(KlMachModule *m)
{
    const CodeBuffer *cb = &m->codes;
    const uint8_t *ptr = cb->data;
    size_t len = cb->size;

    printf("\n====== Emitted Bytecode (insns: %zu) ======\n", len / 4);

    KlrKlass *kls;
    KlrFunc *fn;
    KlMachFunc *mfn;
    vector_foreach(mfn, &m->funcs) {
        fn = mfn->origin;
        kls = fn->klass;
        if (kls) {
            printf("\n@%s::%s:\n", kls->name, fn->name);
        } else {
            printf("\n@%s:\n", fn->name);
        }
        printf("[start_pc: %d, insns: %d, nlocals: %d, max_call_args: %d]\n", mfn->start_pc,
               mfn->total_insns, fn->nlocals, fn->max_call_args);
    }

    printf("\n");

    vector_foreach(mfn, &m->funcs) {
        fn = mfn->origin;
        kls = fn->klass;
        if (kls) {
            printf("@%s::%s[%d,%d]:\n", kls->name, fn->name, mfn->start_pc,
                   mfn->start_pc + mfn->total_insns - 1);
        } else {
            printf("@%s[%d,%d]:\n", fn->name, mfn->start_pc, mfn->start_pc + mfn->total_insns - 1);
        }
        dump_func_byte_code(mfn, ptr);
    }

    printf("\n");

    printf("=== End of Emitted Bytecode ====\n\n");
}

static void dump_mach_insn(KlMachInsn *mi)
{
    KlMachFunc *fn = mi->bb->fn;

    char *s = op_name(mi->op);
    int used = printf("%4d:  %s ", mi->pc, s);

    switch (mi->format) {
        case FORMAT_Rx: {
            used += printf("r%d", mi->opers[0]);
            break;
        }

        case FORMAT_ROff2: {
            used += printf("r%d, %d", mi->opers[0], mi->opers[1]);
            break;
        }

        case FORMAT_RTagImm: {
            used +=
                printf("r%d, @%s, #%d", mi->opers[0], intern_tag_name[mi->opers[1]], mi->opers[2]);
            break;
        }

        case FORMAT_RxTag: {
            used += printf("r%d, #%s", mi->opers[0], tag_mapping[mi->opers[1]]);
            break;
        }

        case FORMAT_RxIdx12: {
            used += printf("r%d, #%d", mi->opers[0], mi->opers[1]);
            break;
        }

        case FORMAT_RImm2:
        case FORMAT_RIdx2: {
            used += printf("r%d, #%d", mi->opers[0], mi->opers[1]);
            break;
        }

        case FORMAT_RImmOff: {
            used += printf("r%d, #%d, %d", mi->opers[0], mi->opers[1], mi->opers[2]);
            break;
        }

        case FORMAT_RxRx: {
            used += printf("r%d, r%d", mi->opers[0], mi->opers[1]);
            break;
        }

        case FORMAT_RRImm: {
            used += printf("r%d, r%d, #%d", mi->opers[0], mi->opers[1], mi->opers[2]);
            break;
        }

        case FORMAT_RROff: {
            used += printf("r%d, r%d, %d", mi->opers[0], mi->opers[1], mi->opers[2]);
            break;
        }

        case FORMAT_RRR: {
            used += printf("r%d, r%d, r%d", mi->opers[0], mi->opers[1], mi->opers[2]);
            break;
        }

        case FORMAT_JMP: {
            used += printf("%d", mi->opers[0]);
            break;
        }

        case FORMAT_Tag: {
            used += printf("#%s", tag_mapping[mi->opers[0]]);
            break;
        }

        case FORMAT_Imm2:
        case FORMAT_Idx2: {
            used += printf("#%d", mi->opers[0]);
            break;
        }

        case FORMAT_CALL: {
            printf("flg=%d, ", mi->opers[0]);
            int ret_reg = mi->opers[1];
            if (ret_reg != -1) used += printf("r%d, ", ret_reg);
            used += printf("#%d", mi->opers[2]);
            break;
        }

        case FORMAT_R_TI_Imm12: {
            printf("r%d, ti=0x%x, #%d", mi->opers[0], mi->opers[1], mi->opers[2]);
            break;
        }

        case FORMAT_TI_Imm2: {
            printf("ti=0x%x, #%d", mi->opers[0], mi->opers[1]);
            break;
        }

        case FORMAT_RR_TI_MODE: {
            printf("r%d, r%d, ti=0x%x, mode=%d", mi->opers[0], mi->opers[1],
                   (mi->opers[2] >> 2) & 0x3Fu, mi->opers[2] & 0x3u);
            break;
        }

        case FORMAT_Op:
            // no operand
            break;

        case FORMAT_DATA:
            // data payload, no fixed fields
            break;

        default:
            used += printf("(unknown format)");
            break;
    }

    if (mi->target) {
        KlrBasicBlock *origin = mi->target->origin;
        printf("%*s;; -> %%%s", 36 - used, "", klr_block_name(origin));
    }

    printf("\n");
}

static void dump_mach_block(KlMachBlock *mb)
{
    printf("%%%s:\n", klr_block_name(mb->origin));

    KlMachInsn *mi;
    vector_foreach(mi, &mb->insns) {
        dump_mach_insn(mi);
    }

    printf("\n");
}

static void dump_mach_func(KlMachFunc *fn)
{
    KlrFunc *origin = fn->origin;
    KlrKlass *kls = origin->klass;

    if (kls) {
        printf("\n====== Linearization @%s::%s(start_pc: %d, insns: %d) ======\n\n", kls->name,
               origin->name, fn->start_pc, fn->total_insns);
    } else {
        printf("\n====== Linearization @%s(start_pc: %d, insns: %d) ======\n\n", origin->name,
               fn->start_pc, fn->total_insns);
    }

    KlMachBlock *mb;
    list_foreach(mb, link, &fn->bb_list) {
        dump_mach_block(mb);
    }

    if (kls) {
        printf("=== End of Linearization @%s::%s ====\n\n", kls->name, origin->name);
    } else {
        printf("=== End of Linearization @%s ====\n\n", fn->origin->name);
    }
}

/* Extract integer value from a raw operand. */
static inline int get_mach_oper(KlrInsn *insn, KlrRawOper *op)
{
    switch (op->kind) {
        case RAW_OPER_REG: {
            /* Use the instruction's result vreg. */
            return op->vreg;
        }

        case RAW_OPER_IMM: {
            return op->imm;
        }

        case RAW_OPER_CONST: {
            return op->index;
        }

        default: {
            UNREACHABLE();
        }
    }
}

static inline void *get_mach_oper_ptr(KlrInsn *insn, KlrRawOper *op)
{
    switch (op->kind) {
        case RAW_OPER_BLOCK: {
            return op->ptr;
        }
        case RAW_OPER_FUNC: {
            return op->ptr;
        }
        default: {
            UNREACHABLE();
        }
    }
}

static KlMachInsn *build_mach_insn(OpCode op, KlrInsn *insn, KlMachBlock *mb)
{
    KlMachInsn *mi = mm_alloc_obj(mi);
    mi->op = op;
    mi->format = op_format(op);
    mi->pc = -1;
    mi->origin = insn;
    mi->bb = mb;
    return mi;
}

static KlMachInsn *build_data_mach_insn(KlMachBlock *mb)
{
    KlMachInsn *mi = mm_alloc_obj(mi);
    mi->op = OP_DATA;
    mi->format = op_format(mi->op);
    mi->pc = -1;
    mi->bb = mb;
    return mi;
}

static void emit_mach_insn(KlMachInsn *mi, CodeBuffer *buf)
{
    uint32_t bytecode = 0;
    OpCode op = mi->op;
    KlrInsn *insn = mi->origin;

    switch (mi->format) {
        case FORMAT_Op: {
            // | op:8 | ----:24 |
            bytecode |= (op & 0xFFu) << 24;
            break;
        }

        case FORMAT_Rx: {
            // | op:8 | ----:12 | Rx:12 |
            uint32_t Rx = mi->opers[0];
            bytecode |= (op & 0xFFu) << 24;
            bytecode |= (Rx & 0xFFFu);
            break;
        }

        case FORMAT_RTagImm: {
            // | op:8 | R:8 | tag:8 | imm:8 |
            uint32_t R = mi->opers[0];
            int tag = mi->opers[1];
            int imm8 = mi->opers[2];
            bytecode |= (op & 0xFFu) << 24;
            bytecode |= (R & 0xFFu) << 16;
            bytecode |= (tag & 0xFFu) << 8;
            bytecode |= (imm8 & 0xFFu);
            break;
        }

        case FORMAT_RxTag: {
            // | op:8 | ----:4 | Rx:12 | imm:8 |
            uint32_t Rx = mi->opers[0];
            int imm8 = mi->opers[1];
            bytecode |= (op & 0xFFu) << 24;
            bytecode |= (Rx & 0xFFFu) << 8;
            bytecode |= (imm8 & 0xFFu);
            break;
        }

        case FORMAT_RxIdx12: {
            // | op:8 | Rx:12 | idx12:12 |
            uint32_t Rx = mi->opers[0];
            uint32_t idx12 = mi->opers[1];
            bytecode |= (op & 0xFFu) << 24;
            bytecode |= (Rx & 0xFFFu) << 12;
            bytecode |= (idx12 & 0xFFFu);
            break;
        }

        case FORMAT_RImm2:
        case FORMAT_ROff2:
        case FORMAT_RIdx2: {
            // | op:8 | R:8 | imm2/off2/idx2:16 |
            uint32_t R = mi->opers[0];
            int data16 = mi->opers[1];
            bytecode |= (op & 0xFFu) << 24;
            bytecode |= (R & 0xFFu) << 16;
            bytecode |= (data16 & 0xFFFFu);
            break;
        }

        case FORMAT_RImmOff: {
            // | op:8 | R:8 | imm:8 | off:8 |
            uint32_t R = mi->opers[0];
            int imm8 = mi->opers[1];
            int off8 = mi->opers[2];
            bytecode |= (op & 0xFFu) << 24;
            bytecode |= (R & 0xFFu) << 16;
            bytecode |= (imm8 & 0xFFu) << 8;
            bytecode |= (off8 & 0xFFu);
            break;
        }

        case FORMAT_RxRx: {
            // | op:8 | Rx1:12 | Rx2:12 |
            uint32_t Rx1 = mi->opers[0];
            uint32_t Rx2 = mi->opers[1];
            bytecode |= (op & 0xFFu) << 24;
            bytecode |= (Rx1 & 0xFFFu) << 12;
            bytecode |= (Rx2 & 0xFFFu);
            break;
        }

        case FORMAT_RRR: {
            // | op:8 | R1:8 | R2:8 | R3:8 |
            uint32_t R1 = mi->opers[0];
            uint32_t R2 = mi->opers[1];
            uint32_t R3 = mi->opers[2];
            bytecode |= (op & 0xFFu) << 24;
            bytecode |= (R1 & 0xFFu) << 16;
            bytecode |= (R2 & 0xFFu) << 8;
            bytecode |= (R3 & 0xFFu);
            break;
        }

        case FORMAT_RRImm:
        case FORMAT_RROff: {
            // | op:8 | R1:8 | R2:8 | imm/off:8 |
            uint32_t R1 = mi->opers[0];
            uint32_t R2 = mi->opers[1];
            int data8 = mi->opers[2];
            bytecode |= (op & 0xFFu) << 24;
            bytecode |= (R1 & 0xFFu) << 16;
            bytecode |= (R2 & 0xFFu) << 8;
            bytecode |= (data8 & 0xFFu);
            break;
        }

        case FORMAT_Tag: {
            // | op:8 | ----:16 | imm:8 |
            int imm8 = mi->opers[0];
            bytecode |= (op & 0xFFu) << 24;
            bytecode |= (imm8 & 0xFFu);
            break;
        }

        case FORMAT_Imm2:
        case FORMAT_JMP:
        case FORMAT_Idx2: {
            // | op:8 | ----:8 | imm2/jmp/idx2:16 |
            int data16 = mi->opers[0];
            bytecode |= (op & 0xFFu) << 24;
            bytecode |= (data16 & 0xFFFFu);
            break;
        }

        case FORMAT_DATA: {
            bytecode = 0;
            break;
        }

        case FORMAT_CALL: {
            // | op:8 | flag:4 | Rx:12 | nargs:8 |
            uint32_t flag = mi->opers[0];
            uint32_t Rx = mi->opers[1];
            uint32_t nargs = mi->opers[2];

            bytecode |= (op & 0xFFu) << 24;
            bytecode |= (flag & 0xFu) << 20;
            bytecode |= (Rx & 0xFFFu) << 8;
            bytecode |= (nargs & 0xFFu);
            break;
        }

        case FORMAT_R_TI_Imm12: {
            // | op:8 | R:8 | tag:4 | imm:12 |
            uint32_t R = mi->opers[0];
            uint32_t ti = mi->opers[1];
            int imm8 = mi->opers[2];
            bytecode |= (op & 0xFFu) << 24;
            bytecode |= (R & 0xFFu) << 16;
            bytecode |= (ti & 0xFu) << 12;
            bytecode |= (imm8 & 0xFFFu);
            break;
        }

        case FORMAT_TI_Imm2: {
            // | op:8 | ---:5 | tag:3 | imm16:16 |
            uint32_t ti = mi->opers[0];
            int imm16 = mi->opers[1];
            bytecode |= (op & 0xFFu) << 24;
            bytecode |= (ti & 0xFu) << 16;
            bytecode |= (imm16 & 0xFFFFu);
            break;
        }

        case FORMAT_RR_TI_MODE: {
            // | op:8 | R1:8 | R2:8 | ---:2 | ti:4 | mode:2 |
            uint32_t R1 = mi->opers[0];
            uint32_t R2 = mi->opers[1];
            int ti = mi->opers[2];
            bytecode |= (op & 0xFFu) << 24;
            bytecode |= (R1 & 0xFFu) << 16;
            bytecode |= (R2 & 0xFFu) << 8;
            bytecode |= ti & 0xFFu;
            break;
        }

        default: {
            UNREACHABLE();
            break;
        }
    }

    // emit 4-byte instruction
    codebuf_put_u32(buf, bytecode);
}

/* Populate machine instruction fields from raw operands.
 * isel must fully define raw_opers[]; this function only
 * maps them into physical encoding fields based on format.
 */
static void fill_mach_insn(KlMachInsn *mi, KlrInsn *insn, KlMachModule *m)
{
    KlrRawOper *op0 = &insn->raws[0];
    KlrRawOper *op1 = &insn->raws[1];
    KlrRawOper *op2 = &insn->raws[2];

    /* Map raw operands into physical encoding fields. */
    switch (mi->format) {
        case FORMAT_Rx:
        case FORMAT_Tag:
        case FORMAT_Imm2:
        case FORMAT_Idx2: {
            mi->opers[0] = get_mach_oper(insn, op0);
            break;
        }

        case FORMAT_TI_Imm2:
        case FORMAT_RxTag:
        case FORMAT_RxIdx12:
        case FORMAT_RImm2:
        case FORMAT_ROff2:
        case FORMAT_RIdx2:
        case FORMAT_RxRx: {
            mi->opers[0] = get_mach_oper(insn, op0);
            mi->opers[1] = get_mach_oper(insn, op1);
            break;
        }

        case FORMAT_RTagImm:
        case FORMAT_R_TI_Imm12:
        case FORMAT_RR_TI_MODE:
        case FORMAT_RImmOff:
        case FORMAT_RRImm:
        case FORMAT_RROff:
        case FORMAT_RRR: {
            mi->opers[0] = get_mach_oper(insn, op0);
            mi->opers[1] = get_mach_oper(insn, op1);
            mi->opers[2] = get_mach_oper(insn, op2);
            break;
        }

        case FORMAT_JMP: {
            void *ptr = get_mach_oper_ptr(insn, op0);
            ASSERT(klr_is_block(ptr));
            KlrBasicBlock *target_bb = (KlrBasicBlock *)ptr;
            mi->target = target_bb->mach;
            break;
        }

        case FORMAT_CALL: {
            mi->opers[1] = get_mach_oper(insn, op0);
            mi->opers[2] = get_mach_oper(insn, op1);
            void *ptr = get_mach_oper_ptr(insn, op2);
            ASSERT(klr_is_func(ptr) || klr_is_extfunc(ptr) || klr_is_intf(ptr) ||
                   klr_is_ext_intf(ptr));
            KlrFunc *fn = (KlrFunc *)ptr;

            if (klr_is_extfunc(ptr)) {
                log_info("  call target: (external)");
                // create an import entry for this external function, and record the
                // import index in the call instruction's import_index field.
                KlrExtFunc *ext_fn = (KlrExtFunc *)fn;
                KlrExtModule *ext_mod = ext_fn->module;
                KlrExtKlass *ext_klass = ext_fn->klass;
                int index;
                if (ext_klass) {
                    index = mach_import_add_method(m, ext_mod->name, ext_klass->origin_name,
                                                   ext_fn->name);
                } else {
                    index = mach_import_add_func(m, ext_mod->name, ext_fn->name);
                }
                mi->import_index = index;
                // insert 4 bytes: import_index
                KlMachBlock *mb = mi->bb;
                KlMachInsn *data = build_data_mach_insn(mb);
                vector_push_back(&mb->insns, &data);
                vector_push_back(&m->fixups, &mi);
                mi->fixup_flag = KL_MACH_FIXUP_IMPORT;
                mi->opers[0] = 1; // set flag for external function
            } else if (klr_is_intf(ptr)) {
                log_info("  call target: (interface method)");
                mi->intf_index = ((KlrIntf *)ptr)->intf_index;
                // insert 4 bytes: slot-id or import-index
                KlMachBlock *mb = mi->bb;
                KlMachInsn *data = build_data_mach_insn(mb);
                vector_push_back(&mb->insns, &data);
                vector_push_back(&m->fixups, &mi);
                mi->fixup_flag = KL_MACH_FIXUP_INTFID;
                mi->opers[0] = 2; // set flag for local interface function
            } else if (klr_is_ext_intf(ptr)) {
                log_info("  call target: (ext interface method)");
                mi->intf_index = ((KlrExtIntf *)ptr)->intf_index;
                // insert 4 bytes: slot-id or import-index
                KlMachBlock *mb = mi->bb;
                KlMachInsn *data = build_data_mach_insn(mb);
                vector_push_back(&mb->insns, &data);
                vector_push_back(&m->fixups, &mi);
                mi->fixup_flag = KL_MACH_FIXUP_INTFID;
                mi->opers[0] = 2; // set flag for external interface function
            } else {
                mi->target_fn = fn->mach;
                ASSERT(fn->mach != NULL);
                log_info("  call target: %s(local)", fn->name);
                if (insn->code == OP_CALL) {
                    // insert 4 bytes: rel32 for call, tail-call needn't insert rel32, because
                    // tail-call always call self.
                    KlMachBlock *mb = mi->bb;
                    KlMachInsn *data = build_data_mach_insn(mb);
                    vector_push_back(&mb->insns, &data);
                    vector_push_back(&m->fixups, &mi);
                    mi->fixup_flag = KL_MACH_FIXUP_REL32;
                }
                mi->opers[0] = 0; // set flag for local function
            }
            break;
        }

        case FORMAT_NEW:
        case FORMAT_Op: {
            // no operand, nothing to fill
            break;
        }

        default: {
            UNREACHABLE();
            break;
        }
    }
}

static int fused_jmp(OpCode op) { return (op >= OP_JMP_INT_EQ && op <= OP_JMP_FLOAT_GE); }

static int ref_fused_jmp(OpCode op) { return (op >= OP_JMP_REF_EQ && op <= OP_JMP_REF_NE_NULL); }

static void linearize(KlMachFunc *mfn, KlMachModule *m)
{
    KlrFunc *fn = mfn->origin;

    // build machine blocks
    int index = 0;
    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        // build MachBlock for each basic block
        KlMachBlock *mb = mm_alloc_obj(mb);
        mb->origin = bb;
        mb->fn = mfn;
        init_list(&mb->link);
        vector_init_ptr(&mb->insns);
        list_push_back(&mfn->bb_list, &mb->link);
        bb->mach = mb;
        if (index == 0) mfn->entry = mb;
        ++index;
    }

    // build machine insns
    KlMachBlock *mb;
    list_foreach(mb, link, &mfn->bb_list) {
        KlMachInsn *mi;
        KlrBasicBlock *bb = mb->origin;
        KlrInsn *insn;
        insn_foreach(insn, bb) {
            if (klr_is_local((KlrValue *)insn)) {
                // pseudo instruction, skip it.
                continue;
            }

            if (insn->code == OP_IR_CAST) {
                KlrValue *src = insn_oper_value(insn, 0);
                ASSERT(insn->vreg >= 0 && src->vreg >= 0);
                if (insn->vreg != src->vreg) {
                    mi = build_mach_insn(OP_MOVE, insn, mb);
                    vector_push_back(&mb->insns, &mi);
                    fill_mach_insn(mi, insn, m);
                }
                continue;
            }

            if (insn_is(insn, OP_MOVE)) {
                KlrValue *dst = insn_oper_value(insn, 0);
                KlrValue *src = insn_oper_value(insn, 1);
                if (dst->vreg == src->vreg) continue;
            }

            if (insn_is(insn, OP_IR_JMP_COND) || fused_jmp(insn->code) ||
                ref_fused_jmp(insn->code)) {
                // don't do sel for jmp_cond here, linearize() only does build linear
                // MachInsn, without any transformation. Do it in lower_branches()
                mi = build_mach_insn(insn->code, insn, mb);
                vector_push_back(&mb->insns, &mi);
                vector_push_back(&mfn->branches, &mi);
                continue;
            }

            // general case
            mi = build_mach_insn(insn->code, insn, mb);
            vector_push_back(&mb->insns, &mi);
            fill_mach_insn(mi, insn, m);
        }
    }
}

/**
 * Lower OP_IR_JMP_COND into concrete machine-level jumps.
 * This pseudo instruction cannot be assigned a PC or encoded
 * directly. We must eliminate it here based solely on block layout
 * (fallthrough).
 *
 *   br cond, bb_true, bb_false
 *
 * If bb_true is the fallthrough block, we invert the condition:
 *
 *   if (!cond) goto bb_false
 *
 * Otherwise:
 *
 *   if (cond) goto bb_true
 *   goto bb_false
 *
 * After this lowering, the instruction stream contains only real,
 * PC-carrying jump instructions, allowing patch_pc() and
 * patch_branches() to compute correct offsets.
 */
static void lower_jmp_cond(KlMachInsn *mi)
{
    KlMachBlock *mb = mi->bb;
    KlrInsn *insn = mi->origin;

    KlrValue *cond = insn_oper_value(insn, 0);

    KlrValue *val = insn_oper_value(insn, 2);
    ASSERT(klr_is_block(val));
    KlrBasicBlock *bb_true = (KlrBasicBlock *)val;

    val = insn_oper_value(insn, 3);
    ASSERT(klr_is_block(val));
    KlrBasicBlock *bb_false = (KlrBasicBlock *)val;

    // next block is fallthrough (may be NULL)
    KlMachBlock *next = NEXT_BLOCK(mb);
    int fallthrough = (next && next->origin == bb_true);

    if (fallthrough) {
        // if (!cond) goto false
        // inplace update
        mi->op = OP_JMP_FALSE;
        mi->format = FORMAT_ROff2;
        mi->origin = insn;
        mi->opers[0] = cond->vreg;
        mi->target = bb_false->mach;
    } else {
        // if (cond) goto true
        // inplace update
        mi->op = OP_JMP_TRUE;
        mi->format = FORMAT_ROff2;
        mi->origin = insn;
        mi->opers[0] = cond->vreg;
        mi->target = bb_true->mach;

        // goto false
        if (next && next->origin != bb_false) {
            KlMachInsn *mi_false = build_mach_insn(OP_JMP, insn, mb);
            mi_false->target = bb_false->mach;
            vector_push_back(&mb->insns, &mi_false);
        }
    }
}

struct jmp_invert {
    OpCode op;
    OpFormat fmt;
};

static struct jmp_invert jmp_invert_map[] = {
    { OP_JMP_INT_NE, FORMAT_RROff },   { OP_JMP_INT_NE_IMM, FORMAT_RImmOff },
    { OP_JMP_INT_EQ, FORMAT_RROff },   { OP_JMP_INT_EQ_IMM, FORMAT_RImmOff },
    { OP_JMP_INT_GE, FORMAT_RROff },   { OP_JMP_INT_GE_IMM, FORMAT_RImmOff },
    { OP_JMP_INT_GT, FORMAT_RROff },   { OP_JMP_INT_GT_IMM, FORMAT_RImmOff },
    { OP_JMP_INT_LE, FORMAT_RROff },   { OP_JMP_INT_LE_IMM, FORMAT_RImmOff },
    { OP_JMP_INT_LT, FORMAT_RROff },   { OP_JMP_INT_LT_IMM, FORMAT_RImmOff },
    { OP_JMP_UINT_GE, FORMAT_RROff },  { OP_JMP_UINT_GE_IMM, FORMAT_RImmOff },
    { OP_JMP_UINT_GT, FORMAT_RROff },  { OP_JMP_UINT_GT_IMM, FORMAT_RImmOff },
    { OP_JMP_UINT_LE, FORMAT_RROff },  { OP_JMP_UINT_LE_IMM, FORMAT_RImmOff },
    { OP_JMP_UINT_LT, FORMAT_RROff },  { OP_JMP_UINT_LT_IMM, FORMAT_RImmOff },
    { OP_JMP_FLOAT_NE, FORMAT_RROff }, { OP_JMP_FLOAT_EQ, FORMAT_RROff },
    { OP_JMP_FLOAT_GE, FORMAT_RROff }, { OP_JMP_FLOAT_GT, FORMAT_RROff },
    { OP_JMP_FLOAT_LE, FORMAT_RROff }, { OP_JMP_FLOAT_LT, FORMAT_RROff },
};

static void lower_fused_jmp(KlMachInsn *mi)
{
    KlMachBlock *mb = mi->bb;
    KlrInsn *insn = mi->origin;
    OpCode op = mi->op;

    KlrValue *lhs = insn_oper_value(insn, 0);
    KlrValue *rhs = insn_oper_value(insn, 1);

    // next block is fallthrough (may be NULL)
    KlMachBlock *next = NEXT_BLOCK(mb);

    KlrValue *val = insn_oper_value(insn, 2);
    ASSERT(klr_is_block(val));
    KlrBasicBlock *bb_true = (KlrBasicBlock *)val;

    val = insn_oper_value(insn, 3);
    ASSERT(klr_is_block(val));
    KlrBasicBlock *bb_false = (KlrBasicBlock *)val;

    int fallthrough = (next && next->origin == bb_true);

    if (fallthrough) {
        // if (!cond) goto false(invert jmp-op)
        // inplace update
        struct jmp_invert *inv = &jmp_invert_map[op - OP_JMP_INT_EQ];
        mi->op = inv->op;
        mi->format = inv->fmt;
        mi->origin = insn;
        mi->opers[0] = lhs->vreg;

        if (op_format(mi->op) == FORMAT_RROff) {
            mi->opers[1] = rhs->vreg;
        } else {
            ASSERT(op_format(mi->op) == FORMAT_RImmOff);
            mi->opers[1] = ((KlrConst *)rhs)->ival;
        }

        mi->target = bb_false->mach;
    } else {
        // if (cond) goto true
        // inplace update
        mi->origin = insn;
        mi->opers[0] = lhs->vreg;

        if (op_format(mi->op) == FORMAT_RROff) {
            mi->opers[1] = rhs->vreg;
        } else {
            ASSERT(op_format(mi->op) == FORMAT_RImmOff);
            mi->opers[1] = ((KlrConst *)rhs)->ival;
        }

        mi->target = bb_true->mach;

        // goto false
        KlMachBlock *next = NEXT_BLOCK(mb);
        if (next && next->origin != bb_false) {
            KlMachInsn *mi_false = build_mach_insn(OP_JMP, insn, mb);
            mi_false->target = bb_false->mach;
            vector_push_back(&mb->insns, &mi_false);
        } else {
            log_info("Fallthrough to false block detected, no jump-zero inserted");
            // printf("Fallthrough to false block detected, no jump-zero inserted\n");
        }
    }
}

static struct jmp_invert ref_jmp_invert_map[] = {
    { OP_JMP_REF_NE, FORMAT_RROff },
    { OP_JMP_REF_EQ, FORMAT_RROff },
    { OP_JMP_REF_NE_NULL, FORMAT_ROff2 },
    { OP_JMP_REF_EQ_NULL, FORMAT_ROff2 },
};

static void lower_ref_fused_jmp(KlMachInsn *mi)
{
    KlMachBlock *mb = mi->bb;
    KlrInsn *insn = mi->origin;
    OpCode op = mi->op;

    KlrValue *lhs = insn_oper_value(insn, 0);
    KlrValue *rhs = insn_oper_value(insn, 1);

    // next block is fallthrough (may be NULL)
    KlMachBlock *next = NEXT_BLOCK(mb);

    KlrValue *val = insn_oper_value(insn, 2);
    ASSERT(klr_is_block(val));
    KlrBasicBlock *bb_true = (KlrBasicBlock *)val;

    val = insn_oper_value(insn, 3);
    ASSERT(klr_is_block(val));
    KlrBasicBlock *bb_false = (KlrBasicBlock *)val;

    int fallthrough = (next && next->origin == bb_true);

    if (fallthrough) {
        // if (!cond) goto false(invert jmp-op)
        // inplace update
        struct jmp_invert *inv = &ref_jmp_invert_map[op - OP_JMP_REF_EQ];
        mi->op = inv->op;
        mi->format = inv->fmt;
        mi->origin = insn;
        mi->opers[0] = lhs->vreg;

        if (op_format(mi->op) == FORMAT_RROff) {
            mi->opers[1] = rhs->vreg;
        }

        mi->target = bb_false->mach;
    } else {
        // if (cond) goto true
        // inplace update
        mi->origin = insn;
        mi->opers[0] = lhs->vreg;

        if (op_format(mi->op) == FORMAT_RROff) {
            mi->opers[1] = rhs->vreg;
        }

        mi->target = bb_true->mach;

        // goto false
        KlMachBlock *next = NEXT_BLOCK(mb);
        if (next && next->origin != bb_false) {
            KlMachInsn *mi_false = build_mach_insn(OP_JMP, insn, mb);
            mi_false->target = bb_false->mach;
            vector_push_back(&mb->insns, &mi_false);
        } else {
            log_info("Fallthrough to false block detected, no jump-zero inserted");
            // printf("Fallthrough to false block detected, no jump-zero inserted\n");
        }
    }
}

static void lower_branches(KlMachFunc *mfn)
{
    KlMachInsn *mi;
    vector_foreach(mi, &mfn->branches) {
        if (mach_insn_is(mi, OP_IR_JMP_COND)) {
            /* Lower jmp_cond into concrete machine-level jumps. */
            lower_jmp_cond(mi);
        } else if (fused_jmp(mi->op)) {
            /* Lower fused jump into concrete machine-level jumps. */
            lower_fused_jmp(mi);
        } else {
            ASSERT(ref_fused_jmp(mi->op));
            lower_ref_fused_jmp(mi);
        }
    }
}

/**
 * Phase 1:
 * Computes the initial PC layout for all MachInsn in the function.
 * This pass assigns insn->pc and block->start_pc based on the current
 * instruction sequence, without modifying the structure or patching
 * any branches. All instructions must be assigned their maximum
 * possible encoded size so that later passes can safely compute
 * relative offsets.
 */
static void assign_pc(KlMachFunc *mfn)
{
    // assign pc

    KlMachModule *m = mfn->m;
    mfn->start_pc = m->pc;
    int pc = mfn->start_pc;

    KlMachBlock *mb;
    list_foreach(mb, link, &mfn->bb_list) {
        // Record the starting PC of this block.
        mb->start_pc = pc;

        // Assign PC to each instruction in this block.
        KlMachInsn *mi;
        vector_foreach(mi, &mb->insns) {
            mi->pc = pc;
            pc++;
        }

        // TODO: remove it
        mb->end_pc = pc;
    }

    mfn->total_insns = pc - mfn->start_pc;
    m->pc += mfn->total_insns;
}

/**
 * Phase 2:
 * Processes fused-jump instructions by computing their relative offsets
 * using the current PC layout. If an offset does not fit the fused-jump
 * encoding (e.g., 8-bit short form), this function may rewrite the
 * MachInsn structure (fallback expansion, long-branch insertion, etc.).
 * When structural changes occur, mfn->changed must be set to true so
 * that the final layout can be recomputed before patching.
 *
 * In the initial implementation, only offset checking is performed and
 * no structural modifications are made.
 */
static void process_fused_jumps(KlMachFunc *mfn)
{
    // TODO: Implement fused-jump processing
}

static void patch_branches(KlMachFunc *mfn)
{
    // patch branch/jmp

    KlMachBlock *mb;
    list_foreach(mb, link, &mfn->bb_list) {
        KlMachInsn *mi;
        vector_foreach(mi, &mb->insns) {
            if (mach_insn_is(mi, OP_JMP)) {
                ASSERT(mi->format == FORMAT_JMP);
                ASSERT(mi->target);
                int target_pc = mi->target->start_pc;
                ASSERT(target_pc >= 0);
                /* Relative offset: target - (current + 1) */
                mi->opers[0] = target_pc - (mi->pc + 1);
            } else if (mach_insn_or(mi, OP_JMP_TRUE, OP_JMP_FALSE)) {
                ASSERT(mi->format == FORMAT_ROff2);
                ASSERT(mi->target);
                int target_pc = mi->target->start_pc;
                ASSERT(target_pc >= 0);
                /* Relative offset: target - (current + 1) */
                mi->opers[1] = target_pc - (mi->pc + 1);
            } else if (fused_jmp(mi->op)) {
                ASSERT(mi->format == FORMAT_RROff || mi->format == FORMAT_RImmOff);
                ASSERT(mi->target);
                int target_pc = mi->target->start_pc;
                ASSERT(target_pc >= 0);
                /* Relative offset: target - (current + 1) */
                mi->opers[2] = target_pc - (mi->pc + 1);
            } else if (ref_fused_jmp(mi->op)) {
                ASSERT(mi->format == FORMAT_RROff || mi->format == FORMAT_ROff2);
                ASSERT(mi->target);
                int target_pc = mi->target->start_pc;
                ASSERT(target_pc >= 0);
                /* Relative offset: target - (current + 1) */
                if (mi->format == FORMAT_RROff) {
                    mi->opers[2] = target_pc - (mi->pc + 1);
                } else {
                    mi->opers[1] = target_pc - (mi->pc + 1);
                }
            } else {
                // do nothing for non-jump instructions
            }
        }
    }
}

/**
 * Phase 3: final layout + branch patching.
 *
 * If process_fused_jumps() modified the MachInsn structure (mfn->changed = true),
 * we must recompute the final PC layout before patching. Otherwise, we only patch.
 */
static void assign_pc_and_patch_branches(KlMachFunc *mfn)
{
    KlMachBlock *mb;
    KlMachModule *m = mfn->m;

    if (mfn->changed) {
        assign_pc(mfn);
        mfn->changed = 0;
    }

    patch_branches(mfn);
}

static void emit_mach_func(KlMachFunc *mfn)
{
    KlMachModule *m = mfn->m;

    KlMachBlock *mb;
    list_foreach(mb, link, &mfn->bb_list) {
        KlMachInsn *mi;
        vector_foreach(mi, &mb->insns) {
            emit_mach_insn(mi, &m->codes);
        }
    }
}

static void patch_fixups(KlMachModule *m)
{
    CodeBuffer *codes = &m->codes;
    KlMachInsn *mi;
    vector_foreach(mi, &m->fixups) {
        if (mach_insn_is(mi, OP_CALL)) {
            if (mi->fixup_flag == KL_MACH_FIXUP_REL32) {
                ASSERT(mi->format == FORMAT_CALL);
                ASSERT(mi->target_fn);
                int target_pc = mi->target_fn->start_pc;
                // the following DATA insn
                int payload_pc = mi->pc + 1;

                log_info("fixup call '%s' at pc %d(relative), to 'target %s' at pc %d",
                         mi->origin->bb->func->name, payload_pc, mi->target_fn->origin->name,
                         target_pc);

                ASSERT(target_pc >= 0);
                /* Relative offset: target - (payload + 1) */
                // int rel32 = target_pc - (payload_pc + 1);
                int local_index = mi->target_fn->index;
                /* Patch the placeholder data instruction following the call. */
                int *patch = (int *)codes->data + payload_pc;
                *patch = local_index;

                log_info("  patched position at pc %d with local index %d", payload_pc,
                         local_index);
            } else if (mi->fixup_flag == KL_MACH_FIXUP_IMPORT) {
                ASSERT(mi->format == FORMAT_CALL);
                ASSERT(mi->import_index >= 0);
                // the following DATA insn
                int payload_pc = mi->pc + 1;
                int *patch = (int *)codes->data + payload_pc;
                *patch = mi->import_index;
                log_info("fixup call '%s' at pc %d(import), with import index %d",
                         mi->origin->bb->func->name, payload_pc, mi->import_index);
            } else {
                ASSERT(mi->fixup_flag == KL_MACH_FIXUP_INTFID);
                ASSERT(mi->format == FORMAT_CALL);
                // the following DATA insn
                int payload_pc = mi->pc + 1;
                int *patch = (int *)codes->data + payload_pc;
                *patch = mi->intf_index; // slot-id or import-index
            }
        } else {
            UNREACHABLE();
        }
    }
}

static void peephole(KlMachFunc *mfn)
{
    KlMachBlock *mb;
    list_foreach(mb, link, &mfn->bb_list) {
        KlMachInsn **codes = VECTOR_RAW(&mb->insns, KlMachInsn *);
        int n = vector_size(&mb->insns);

        // jmp → fallthrough
        // mark unconditional jumps to the immediately following block as dead
        for (int i = 0; i < n; ++i) {
            KlMachInsn *mi = codes[i];
            if (mi->dead) continue;
            if (mi->op == OP_JMP) {
                KlMachBlock *next = NEXT_BLOCK(mb);
                if (next && next == mi->target) {
                    mi->dead = 1;
                }
            }
        }

        // reverse pass for other peephole optimizations
        for (int i = n - 1; i > 0; i--) {
            KlMachInsn *a = codes[i];
            if (a->dead) continue;

            KlMachInsn *b = codes[i - 1];
            if (b->dead) continue;

            if (a->op != OP_MOVE) continue;

            if ((b->op >= OP_INT_ADD && b->op <= OP_INT_SHR_IMM) ||
                (b->op >= OP_FLOAT_ADD && b->op <= OP_FLOAT_MOD) ||
                (b->op >= OP_UINT_ADD_IMM && b->op <= OP_UINT_SHR_IMM)) {
                if ((a->opers[0] == b->opers[1]) && (a->opers[1] == b->opers[0])) {
                    KlrInsn *b_insn = b->origin;
                    if (b_insn->use_count == 1) {
                        KlrInsn *a_ref = (KlrInsn *)insn_oper_value(a->origin, 1);
                        if (a_ref == b_insn) {
                            a->dead = 1;
                            b->opers[0] = a->opers[0];
                        }
                    }
                }
            }
        }

        for (int i = 0; i < n; i++) {
            KlMachInsn *mi = codes[i];
            if (mi->dead) continue;
            if (mi->op == OP_MOVE) {
                if (mi->opers[0] == mi->opers[1]) {
                    mi->dead = 1;
                }
            }
        }

        // compact the instruction array by removing dead instructions
        int w = 0;
        for (int r = 0; r < n; r++) {
            KlMachInsn *mi = codes[r];
            if (!mi->dead) {
                if (w != r) codes[w] = codes[r];
                w++;
            }
        }
        mb->insns.size = w;
    }
}

static void init_mach_context(KlMachModule *m, KlrModule *origin)
{
    m->origin = origin;
    vector_init_ptr(&m->funcs);
    vector_init_ptr(&m->fixups);
    vector_init_ptr(&m->import_table);
    vector_init_ptr(&m->const_pool);
    codebuf_init(&m->codes);
    hashmap_init(&m->cp_map, __mach_const_eq__);
    hashmap_init(&m->import_map, __mach_import_eq__);
    m->pc = 0;
    origin->mach = m;
}

static void _add_mach_func(KlrFunc *fn, KlMachModule *m)
{
    KlMachFunc *mfn = mm_alloc_obj(mfn);
    mfn->origin = fn;
    mfn->m = m;
    init_list(&mfn->bb_list);
    vector_init_ptr(&mfn->branches);
    vector_push_back(&m->funcs, &mfn);
    mfn->index = vector_size(&m->funcs) - 1;
    fn->mach = mfn;
}

void kl_do_codegen(KlrModule *origin)
{
    if (!origin || origin->errors > 0) return;

    KlMachModule *m = mm_alloc_obj(m);
    init_mach_context(m, origin);

    KlrFunc *fn;
    func_foreach(fn, origin) {
        kl_lower_operands(fn, m);
        _add_mach_func(fn, m);
    }

    KlrKlass *kls;
    vector_foreach(kls, &origin->klasses) {
        ASSERT(kls);
        KlrFunc *fn;
        func_foreach(fn, kls) {
            kl_lower_operands(fn, m);
            _add_mach_func(fn, m);
        }
    }

    KlMachFunc *mfn;
    vector_foreach(mfn, &m->funcs) {
        linearize(mfn, m);
        peephole(mfn);
        lower_branches(mfn);
        assign_pc(mfn);
        process_fused_jumps(mfn);
        assign_pc_and_patch_branches(mfn);
        emit_mach_func(mfn);
        if (dump_code_enabled()) dump_mach_func(mfn);
    }

    patch_fixups(m);

    if (dump_code_enabled()) {
        dump_byte_code(m);
        dump_const_pool(m);
        dump_import_table(m);
    }
}

#ifdef __cplusplus
}
#endif

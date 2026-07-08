/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "vm.h"
#include <dlfcn.h>
#include <unistd.h>
#include "atom.h"
#include "buffer.h"
#include "klc.h"
#include "log.h"
#include "mm.h"
#include "modobj.h"

#ifdef __cplusplus
extern "C" {
#endif

void init_tag_mappings(void);

/* max stack size */
#define MAX_STACK_SIZE (2 * 64 * 1024)

/* pthread */
__thread ThreadState *__ts;

KoalaState *kl_new_ks(void)
{
    KoalaState *ks = mm_alloc_obj(ks);
    ks->ts = __ts;
    ks->stack_base = aligned_alloc(16, sizeof(TValue) * MAX_STACK_SIZE);
    ks->stack_top = ks->stack_base;
    ks->stack_size = MAX_STACK_SIZE;
    return ks;
}

void kl_free_ks(KoalaState *ks)
{
    if (!ks) return;
    ASSERT(!ks->cf);
    free(ks->stack_base);
    mm_free(ks);
}

void koala_initialize(void)
{
    /* init logger */
    init_log(LOG_INFO, NULL, 0);

    /* init atom string table */
    init_atom();

    /* init global module table */
    kl_init_gm_stbl();

    /* init builtin & sys module */
    init_builtin_module();
    // init_sys_module(ks);

    /* initialize main thread as koala thread */
    ThreadState *ts = mm_alloc_obj(ts);
    ts->current = kl_new_ks();
    __ts = ts;

    /* initialize tag mappings */
    init_tag_mappings();
}

static void __load_const(Object *m, KlcConst *item)
{
    switch (item->type) {
        case KLC_CONST_NONE: {
            kl_mo_add_none(m);
            break;
        }

        case KLC_CONST_INT: {
            if (item->sign)
                kl_mo_add_int(m, (int64_t)item->ival, item->type_info);
            else
                kl_mo_add_uint(m, item->ival, item->type_info);
            break;
        }

        case KLC_CONST_FLT: {
            kl_mo_add_float(m, item->fval, item->type_info);
            break;
        }

        case KLC_CONST_BOOL: {
            kl_mo_add_bool(m, (int)item->ival);
            break;
        }

        case KLC_CONST_SHORT_ASCII:
        case KLC_CONST_SHORT_UTF8:
        case KLC_CONST_ASCII:
        case KLC_CONST_UTF8: {
            kl_mo_add_str(m, item->sval);
            break;
        }

        case KLC_CONST_SHORT_TUPLE:
        case KLC_CONST_TUPLE: {
            Vector *vec = item->val;
            kl_mo_add_tuple(m, vec);
            break;
        }

        case KLC_CONST_RANGE: {
            Vector *vec = item->val;
            kl_mo_add_range(m, vec);
            break;
        }

        case KLC_CONST_SHORT_LIST:
        case KLC_CONST_LIST: {
            Vector *vec = item->val;
            kl_mo_add_list(m, vec);
            break;
        }

        default: {
            NYI();
            break;
        }
    }
}

Object *kl_get_native(Object *m, char *name)
{
    ModuleObject *mo = (ModuleObject *)m;
    NativeModule *native;
    vector_foreach_ptr(native, &mo->natives) {
        Object *obj = stbl_find_obj(&native->symbols, name);
        if (obj) return obj;
    }
    return NULL;
}

int kl_register_func(NativeModule *m, char *name, NativeFunc fn)
{
    Object *obj = kl_new_cfunc(name, fn, NULL);
    stbl_add_obj(&m->symbols, name, obj);
    return 0;
}

int kl_register_method(NativeModule *m, char *cls, char *meth, NativeFunc fn)
{
    char full_name[256];
    snprintf(full_name, sizeof(full_name), "%s$%s", cls, meth);
    Object *obj = kl_new_cfunc(full_name, fn, NULL);
    stbl_add_obj(&m->symbols, full_name, obj);
    return 0;
}

static const char *native_suffix(void)
{
#if defined(__APPLE__)
    return ".dylib";
#elif defined(_WIN32)
    return ".dll";
#else
    return ".so";
#endif
}

static void _load_native(ModuleObject *mo, char *name)
{
    char buf[256];
    snprintf(buf, sizeof(buf), "lib%s%s", name, native_suffix());

    void *handle = dlopen(buf, RTLD_LAZY | RTLD_LOCAL);
    if (!handle) {
        fprintf(stderr, "Failed to load %s: %s\n", buf, dlerror());
        return;
    }

    snprintf(buf, sizeof(buf), "%s_module_init", name);

    typedef void (*InitFunc)(NativeModule *);
    InitFunc init_func = dlsym(handle, buf);

    if (!init_func) {
        fprintf(stderr, "Cannot find init function %s: %s\n", buf, dlerror());
        dlclose(handle);
        return;
    }

    NativeModule native_module;
    native_module.index = vector_size(&mo->natives);
    native_module.handle = handle;
    stbl_init(&native_module.symbols);
    vector_push_back(&mo->natives, &native_module);

    init_func(&native_module);
}

static Object *_load_module(char *path, char *pkg_path)
{
    KlcFile *klc = read_klc_file(path, 1);
    if (!klc) return NULL;

    Object *m = kl_new_module(pkg_path);

    uint8_t *codes = NULL;
    uint32_t size = klc_get_bytecodes(klc, &codes);
    kl_mo_set_code(m, (uint32_t *)codes, size / 4);

    int num_rt_consts = klc->num_rt_consts;
    Vector *rt_consts = klc->objs + ITEM_RT_CONST;
    KlcConst *kc;
    vector_foreach(kc, rt_consts) {
        if (!kc) continue;
        if (num_rt_consts <= 0) break;
        __load_const(m, kc);
        --num_rt_consts;
    }

    Vector *imports = klc->objs + ITEM_IMPORT;
    KlcImport *imp;
    vector_foreach(imp, imports) {
        if (!imp) continue;
        KlcConst *ns = klc_get_rt_const(klc, imp->ns_index);
        KlcConst *kls = klc_get_rt_const(klc, imp->kls_index);
        KlcConst *sym = klc_get_rt_const(klc, imp->sym_index);
        char *kls_name = kls ? kls->sval : NULL;
        kl_mo_add_import(m, imp->kind, ns->sval, kls_name, sym->sval);
    }

    Vector *links = klc->objs + ITEM_LINK;
    uint16_t link_index;
    vector_foreach(link_index, links) {
        if (link_index == 0) continue;
        KlcConst *kc = klc_get_rt_const(klc, link_index);
        _load_native((ModuleObject *)m, kc->sval);
    }

    Vector *code_objs = klc->objs + ITEM_CODE;
    KlcCode *item;
    vector_foreach(item, code_objs) {
        if (!item) continue;
        kc = klc_get_rt_const(klc, item->name_index);
        Object *_co;
        if (item->flags & KLC_FLAGS_NATIVE) {
            _co = kl_get_native(m, kc->sval);
            ASSERT(_co && IS_CFUNC(_co));
            CFuncObject *cfn = (CFuncObject *)_co;
            ASSERT(cfn->owner == NULL);
            cfn->owner = m;
        } else {
            _co = kl_new_code(kc->sval, m);
            CodeObject *co = (CodeObject *)_co;
            if (item->flags & KLC_FLAGS_PUB) co->flags |= CODE_FLAG_PUB;
            if (item->flags & KLC_FLAGS_METH) co->flags |= CODE_FLAG_METH;
            co->cs.nlocals = item->nlocals;
            co->cs.max_call_args = item->max_call_args;
            co->cs.start_pc = item->start_pc;
            co->cs.num_insns = item->num_insns;
        }
        kl_mo_add_func(m, _co);
    }

    ModuleObject *mo = (ModuleObject *)m;
    Vector *cls_objs = klc->objs + ITEM_CLASS;
    KlcKlass *cls;
    vector_foreach(cls, cls_objs) {
        if (!cls) continue;
        KlcConst *kls_kc = klc_get_const(klc, cls->name_index);
        TypeObject *tp = kl_new_type(kls_kc->sval, cls->flags);

        if (cls->flags & KLC_FLAGS_TRAIT) continue;

        KlcVar *var;
        vector_foreach(var, &cls->fields) {
            if (!var) continue;
            kc = klc_get_const(klc, var->name_index);
            Object *field = kl_new_index_field(kc->sval, 0, i__, (Object *)tp);
            vector_push_back(&tp->fields, &field);
            stbl_add_obj(&tp->members, kc->sval, field);
        }

        KlcFunc *meth;
        vector_foreach(meth, &cls->methods) {
            if (!meth) continue;
            kc = klc_get_const(klc, meth->name_index);
            Object *_co = vector_get(&mo->funcs, meth->code_index);
            ASSERT(_co);
            if (IS_CODE(_co)) {
                CodeObject *co = (CodeObject *)_co;
                co->owner = (Object *)tp;
                ASSERT(co->flags & CODE_FLAG_METH);
            } else {
                ASSERT(IS_CFUNC(_co));
                CFuncObject *cfn = (CFuncObject *)_co;
                cfn->owner = (Object *)tp;
            }
            vector_push_back(&tp->methods, &_co);
            stbl_add_obj(&tp->members, kc->sval, _co);
        }

        KlcIntfEntry *intf_entry;
        vector_foreach(intf_entry, &cls->intf_table) {
            if (!intf_entry) continue;

            IntfTable itable;
            kc = klc_get_const(klc, intf_entry->name_index);
            itable.name = atom(kc->sval);
            itable.num_funcs = vector_size(&intf_entry->methods);
            itable.num_parents = vector_size(&intf_entry->parents);
            itable.methods = mm_alloc(sizeof(Object *) * itable.num_funcs);
            itable.parents = mm_alloc(sizeof(IntfTable *) * itable.num_parents);

            uint16_t _idx = 0;
            vector_foreach(_idx, &intf_entry->methods) {
                if (_idx == 0xFFFFu) {
                    ASSERT(0); // should not happen, but just in case
                    ASSERT(i__ < itable.num_funcs);
                    itable.methods[i__] = ((ModuleObject *)m)->not_impl;
                    continue;
                }

                Object *co = vector_get(&mo->funcs, _idx);
                ASSERT(co);
                // CodeObject *co_obj = (CodeObject *)co;
                // ASSERT(co_obj->flags & CODE_FLAG_METH);
                ASSERT(i__ < itable.num_funcs);
                itable.methods[i__] = co;
            }

            vector_push_back(&tp->itables, &itable);
        }

        vector_foreach(intf_entry, &cls->intf_table) {
            if (!intf_entry) continue;

            IntfTable *itable = vector_get_ptr(&tp->itables, i__ - 1);

            uint16_t _idx = 0;
            vector_foreach(_idx, &intf_entry->parents) {
                IntfTable *parent = vector_get_ptr(&tp->itables, _idx);
                ASSERT(parent);
                ASSERT(i__ < itable->num_parents);
                itable->parents[i__] = parent;
            }
        }

        vector_push_back(&mo->types, &tp);
        stbl_add_obj(&mo->symbols, kls_kc->sval, (Object *)tp);
        tp->flags |= TP_FLAGS_READY;
        tp->module = m;
    }

    kl_init_module(m);
    kl_resolve_import(m);
    free_klc_file(klc);
    return m;
}

static int isdotklc(char *filename)
{
    char *dot = strrchr(filename, '.');
    if (dot == NULL || strlen(dot) != 4) return 0;
    if (dot[1] == 'k' && dot[2] == 'l' && dot[3] == 'c') return 1;
    return 0;
}

Object *kl_load_module(char *path)
{
    Object *m = NULL;

    if (path[0] == '/') {
        log_info("loading module '%s'", path);
        m = _load_module(path, path);
        goto done;
    }

    char *koala_path = getenv("KOALA_PATH");
    if (!koala_path) {
        log_info("KOALA_PATH is not set");
        m = _load_module(path, path);
        goto done;
    }

    log_info("KOALA_PATH: %s", koala_path);

    BUF(buf);
    char *prefix = NULL;
    int count = str_sep(&koala_path, ':', &prefix);
    while (count > 0) {
        buf_write_nstr(&buf, prefix, count);
        buf_write_str(&buf, path);
        if (!isdotklc(path)) buf_write_str(&buf, ".klc");
        m = _load_module(BUF_STR(buf), path);
        if (m) {
            log_info("found package '%s' in KOALA_PATH: %s", path, prefix);
            break;
        }

        RESET_BUF(buf);
        count = str_sep(&koala_path, ':', &prefix);
    }

    FINI_BUF(buf);

done:
    ModuleObject *mo = (ModuleObject *)m;
    if (mo && mo->__init__) {
        log_info("initializing module '%s'", path);
        kl_run_init(m);
    }
    return m;
}

Object *kl_get_intf_func(TValue *intf, int func_idx)
{
    ASSERT(is_intf(intf));
    IntfTable *itab = intf->itab;
    ASSERT(itab);
    ASSERT(func_idx >= 0 && func_idx < itab->num_funcs);
    return itab->methods[func_idx];
}

void koala_run_file(char *path)
{
    Object *m = kl_load_module(path);
    if (m) kl_run_main(m);
}

void koala_finalize(void) { /* finalize atom string table */ fini_atom(); }

#ifdef __cplusplus
}
#endif

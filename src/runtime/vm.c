/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "vm.h"
#include <dlfcn.h>
#include <unistd.h>
#include "atom.h"
#include "buffer.h"
#include "excobj.h"
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

static TValue kl_not_impl_func(TValue *self, TValue *args, int nargs)
{
    Object *obj = to_obj(self);
    ASSERT(IS_CFUNC(obj));

    CFuncObject *cfunc = (CFuncObject *)obj;
    Object *owner = cfunc->owner;

    if (cfunc->priv) {
        raise_exc_str((char *)cfunc->priv);
        return error_value;
    }

    if (IS_MODULE(owner)) {
        ModuleObject *m = (ModuleObject *)owner;
        raise_exc_fmt("func '%s' in '%s' is not implemented!", cfunc->name, m->path);
        return error_value;
    }

    ASSERT(IS_TYPE(owner, &type_type));
    TypeObject *tp = (TypeObject *)owner;
    char *dollar = strchr(cfunc->name, '$') + 1;
    raise_exc_fmt("func '%s' of '%s' is not implemented!", dollar, tp->name);
    return error_value;
}

static Object *new_not_impl_func(Object *m, char *func_name)
{
    Object *cfunc = kl_new_cfunc(func_name, kl_not_impl_func, m);
    ((CFuncObject *)cfunc)->not_impl = 1;
    return cfunc;
}

static Object *new_not_impl_trait_func(char *kls_name, char *trait_name, char *fn_name, Object *m)
{
    BUF(name);
    BUF(msg);

    buf_write_fmt(&name, "%s_%s_%s", trait_name, kls_name, fn_name);
    buf_write_fmt(&msg, "func '%s::%s' for '%s' is not implemented!", kls_name, fn_name,
                  trait_name);

    Object *cfunc = kl_new_cfunc(BUF_STR(name), kl_not_impl_func, m);

    char *_msg = mm_alloc(BUF_LEN(msg) + 1);
    memcpy(_msg, BUF_STR(msg), BUF_LEN(msg));
    _msg[BUF_LEN(msg)] = '\0';

    ((CFuncObject *)cfunc)->not_impl = 1;
    ((CFuncObject *)cfunc)->priv = _msg;

    FINI_BUF(name);
    FINI_BUF(msg);
    return cfunc;
}

static TValue *get_global_var(Object *m, char *name)
{
    ModuleObject *mo = (ModuleObject *)m;
    Object *ob = kl_mo_find(m, name);
    if (!ob) return NULL;
    ASSERT(IS_GLOBAL(ob));
    GlobalObject *gobj = (GlobalObject *)ob;
    int index = gobj->index;
    ASSERT(index >= 0 && index < mo->num_values);
    TValue *val = mo->values + index;
    return val;
}

static Object *load_module(char *path);

static Object *find_or_load_module(char *path)
{
    Object *m = kl_get_module(path);
    if (m) {
        ModuleObject *mo = (ModuleObject *)m;
        if (!(mo->flags & MOD_FLAGS_READY)) {
            fprintf(stderr, "warning: circular dependency detected: %s\n", path);
        }
        return m;
    }
    return load_module(path);
}

static void resolve_import(Object *_m)
{
    ModuleObject *m = (ModuleObject *)_m;

    ImportEntry *e;
    vector_foreach_ptr(e, &m->import_table) {
        Object *obj = NULL;

        if (e->kind == IMPORT_KIND_FUNC || e->kind == IMPORT_KIND_GLOBAL ||
            e->kind == IMPORT_KIND_TYPE) {
            Object *mod = find_or_load_module(e->path);
            if (!mod) {
                panic("failed to resolve import: module '%s' is not found", e->path);
                return;
            }

            obj = kl_mo_find(mod, e->name);
            if (!obj) {
                panic("failed to resolve import: symbol '%s::%s' is not found", e->path, e->name);
                return;
            }
        } else {
            ASSERT(e->kind == IMPORT_KIND_METHOD || e->kind == IMPORT_KIND_FIELD);
            Object *mod = find_or_load_module(e->path);
            if (!mod) {
                panic("failed to resolve import: module '%s' is not found", e->path);
                return;
            }

            Object *cls = kl_mo_find(mod, e->kls);
            if (!cls || !IS_TYPE(cls, &type_type)) {
                panic("failed to resolve import: class '%s::%s' is not found", e->path, e->kls);
                return;
            }

            obj = kl_type_find((TypeObject *)cls, e->name);
            if (!obj) {
                panic("failed to resolve import: symbol '%s::%s' is not found", e->kls, e->name);
                return;
            }
        }

        e->address = obj;
        if (e->kind == IMPORT_KIND_TYPE) {
            ASSERT(vector_size(&m->types) == e->slot_index);
            vector_push_back(&m->types, &obj);
        } else if (e->kind == IMPORT_KIND_GLOBAL) {
            ASSERT(vector_size(&m->globals) == e->slot_index);
            vector_push_back(&m->globals, &obj);
        } else if (e->kind == IMPORT_KIND_FUNC || e->kind == IMPORT_KIND_METHOD) {
            ASSERT(vector_size(&m->funcs) == e->slot_index);
            vector_push_back(&m->funcs, &obj);
        }
    }
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

        case KLC_CONST_SLICE: {
            Vector *vec = item->val;
            kl_mo_add_slice(m, vec);
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

static void _load_native(ModuleObject *mo, char *name, char *pkg_name)
{
    char buf[256];
    snprintf(buf, sizeof(buf), "lib%s%s", name, native_suffix());

    char *filename = NULL;
    if (!str_equal(name, "koala")) filename = buf;

    void *handle = dlopen(filename, RTLD_LAZY | RTLD_LOCAL);
    if (!handle) {
        fprintf(stderr, "Failed to load %s: %s\n", buf, dlerror());
        return;
    }

    char *slash = strrchr(pkg_name, '/');
    snprintf(buf, sizeof(buf), "%s_native_lib_init", slash ? slash + 1 : pkg_name);

    typedef void (*InitFunc)(NativeLib *);
    InitFunc init_func = dlsym(handle, buf);

    if (!init_func) {
        fprintf(stderr, "Cannot find init function %s: %s\n", buf, dlerror());
        dlclose(handle);
        return;
    }

    NativeLib lib = { .name = name, .handle = handle };
    stbl_init(&lib.symbols);
    init_func(&lib);
    vector_push_back(&mo->libs, &lib);
}

static TypeObject *find_tp_from_native(Object *m, char *name)
{
    Object *obj = kl_get_native(m, name);
    if (obj) {
        ASSERT(IS_TYPE(obj, &type_type));
        TypeObject *tp = (TypeObject *)obj;
        return tp;
    }
    return NULL;
}

static Object *_load_module(char *path)
{
    KlcFile *klc = read_klc_file(path, 1);
    if (!klc) return NULL;

    char *pkg_name = klc->pkg_path;
    Object *m = kl_new_module(pkg_name);
    ModuleObject *mo = (ModuleObject *)m;

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

    // process imports

    Vector *imports = klc->objs + ITEM_IMPORT;
    KlcImport *imp;
    vector_foreach(imp, imports) {
        if (!imp) continue;
        KlcConst *ns = klc_get_rt_const(klc, imp->ns_index);
        KlcConst *kls = klc_get_rt_const(klc, imp->kls_index);
        KlcConst *sym = klc_get_rt_const(klc, imp->sym_index);
        char *kls_name = kls ? kls->sval : NULL;
        kl_mo_add_import(m, imp->kind, ns->sval, kls_name, sym->sval, imp->slot_index);
    }

    // process native libraries

    Vector *links = klc->objs + ITEM_LINK;
    uint16_t link_index;
    vector_foreach(link_index, links) {
        if (link_index == 0) continue;
        KlcConst *kc = klc_get_rt_const(klc, link_index);
        _load_native(mo, kc->sval, pkg_name);
    }

    // process code objects

    Vector *code_objs = klc->objs + ITEM_CODE;
    KlcCode *item;
    vector_foreach(item, code_objs) {
        if (!item) continue;
        kc = klc_get_rt_const(klc, item->name_index);
        Object *_co;
        if (item->flags & KLC_FLAGS_NATIVE) {
            // if koala's function is marked as native, it must be implemented by a native function
            // in the module's native library.
            _co = kl_get_native(m, kc->sval);
            if (!_co) _co = new_not_impl_func(m, kc->sval);
            ASSERT(_co && IS_CFUNC(_co));
            CFuncObject *cfn = (CFuncObject *)_co;
            if (cfn->owner == NULL) {
                cfn->owner = m;
            }
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
        kl_mo_add_func(m, kc->sval, _co);
    }

    // set local function count
    mo->num_funcs = vector_size(&mo->funcs);

    // process classes

    Vector *cls_objs = klc->objs + ITEM_CLASS;
    KlcKlass *cls;
    vector_foreach(cls, cls_objs) {
        if (!cls) continue;
        if (cls->flags & KLC_FLAGS_TRAIT) continue;

        KlcConst *kls_kc = klc_get_const(klc, cls->name_index);

        TypeObject *tp = find_tp_from_native(m, kls_kc->sval);
        if (!tp) {
            tp = kl_new_type(kls_kc->sval, TP_FLAGS_CLASS);
        }

        // ASSERT(tp->flags & TP_FLAGS_CLASS);

        KlcVar *var;
        vector_foreach(var, &cls->fields) {
            if (!var) continue;
            kc = klc_get_const(klc, var->name_index);
            Object *field = kl_new_index_field(kc->sval, 0, i__ - 1, (Object *)tp);
            kl_tp_add_field(tp, kc->sval, field);
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
            kl_tp_add_method(tp, kc->sval, meth->slot_id, _co);
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
            itable.tp = tp;

            uint16_t _idx = 0;
            vector_foreach(_idx, &intf_entry->methods) {
                if (_idx > 0x8000u) {
                    log_warn(
                        "[_load_module] class '%s' does not implement method '%s' of interface "
                        "'%s'",
                        tp->name, kc->sval, itable.name);
                    ASSERT(i__ < itable.num_funcs);
                    _idx -= 0x8000u;
                    KlcConst *fn_kc = klc_get_rt_const(klc, _idx);
                    itable.methods[i__] =
                        new_not_impl_trait_func(kls_kc->sval, kc->sval, fn_kc->sval, m);
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

        tp->flags |= TP_FLAGS_READY;

        kl_mo_add_type(m, tp);
    }

    // set the number of local classes in the module
    mo->num_klasses = vector_size(&mo->types);

    // process test functions

    Vector *func_objs = klc->objs + ITEM_FUNC;
    if (vector_size(func_objs) > 1) {
        // slot 0 is reserved for null.
        KlcFunc *fn_item;
        vector_foreach(fn_item, func_objs) {
            if (!fn_item) continue;
            KlcAnnot *ann = vector_get(&fn_item->anns, 1);
            if (!ann) continue;
            char *_name = klc_get_str(klc, ann->name_index);
            if (!match_prefix(_name, "test")) continue;
            Object *_co = vector_get(&mo->funcs, fn_item->code_index);
            ASSERT(_co && IS_CODE(_co));
            int expect_panic = match_prefix(_name, "test_expect_panic");
            char *msg = NULL;
            if (expect_panic) {
                log_info("test '%s' expects panic", _name);
                msg = klc_get_str(klc, ann->value_index);
            }
            _name = klc_get_str(klc, fn_item->name_index);
            kl_mo_add_test(m, _name, (CodeObject *)_co, expect_panic, msg);
        }
    }

    // process lineinfo for testing(traceback)

    Vector *lineinfos = klc->objs + ITEM_LINEINFO;
    KlcLineInfo *_line;
    vector_foreach(_line, lineinfos) {
        if (!_line) continue;
        // handle lineinfo as needed for testing(traceback)
        LineInfo line;
        line.pc = _line->pc;
        line.filename = klc_get_str(klc, _line->name_index);
        line.lineno = _line->line;
        vector_push_back(&mo->lineinfos, &line);
        // printf("  PC: %u, File \"%s\", line %d\n", line.pc, line.filename, line.lineno);
    }

    // process global variables

    Vector *globals = klc->objs + ITEM_VAR;
    mo->num_values = vector_size(globals) - 1;
    ASSERT(mo->num_values >= 0);
    KlcVar *var;
    vector_foreach(var, globals) {
        if (!var) continue;
        kc = klc_get_const(klc, var->name_index);
        Object *val = kl_new_global(kc->sval, i__ - 1, m);
        vector_push_back(&mo->globals, &val);
        stbl_add_obj(&mo->symbols, kc->sval, val);
        mo->num_globals++;
    }

    // allocate global variables space

    if (mo->num_values > 0) {
        mo->values = mm_alloc(sizeof(TValue) * mo->num_values);
        for (uint32_t i = 0; i < mo->num_values; i++) {
            mo->values[i] = nil_value;
        }
    }

    // resolve imports
    resolve_import(m);

    mo->flags |= MOD_FLAGS_READY;

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

static Object *load_module(char *path)
{
    Object *m = NULL;

    if (path[0] == '/') {
        log_info("loading module '%s'", path);
        m = _load_module(path);
        goto done;
    }

    char *koala_path = getenv("KOALA_PATH");
    if (!koala_path) {
        log_info("KOALA_PATH is not set");
        m = _load_module(path);
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
        m = _load_module(BUF_STR(buf));
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

KOALA_EXPORT void koala_run_file(char *path)
{
    Object *m = load_module(path);
    if (m) kl_run_main(m);
}

KOALA_EXPORT int koala_test_file(char *path)
{
    Object *m = load_module(path);
    if (m) return kl_run_tests(m);
    return 0;
}

static void load_modules(void)
{
    // It's not necessary to load standard builtin module here.
    // because it will be loaded automatically when needed.
    // kl_load_module("std/builtin");
}

KOALA_EXPORT void koala_initialize(void)
{
    /* init logger */

#ifdef DEBUG_TEST
    init_log(LOG_WARN, NULL, 0);
#else
    init_log(LOG_TRACE, NULL, 0);
#endif

    /* init atom string table */
    init_atom();

    /* init global module table */
    kl_init_gm_stbl();

    /* initialize main thread as koala thread */
    ThreadState *ts = mm_alloc_obj(ts);
    ts->current = kl_new_ks();
    __ts = ts;

    /* initialize tag mappings */
    init_tag_mappings();

    load_modules();
}

KOALA_EXPORT void koala_finalize(void) { /* finalize atom string table */ fini_atom(); }

#ifdef __cplusplus
}
#endif

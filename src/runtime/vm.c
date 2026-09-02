/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "vm.h"
#include <dlfcn.h>
#include <unistd.h>
#include "args.h"
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

KoalaOptions kl_cmd_opt = { 0 };

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

TValue kl_not_impl_func(TValue *self, TValue *args, int nargs)
{
    Object *obj = to_obj(self);
    if (!IS_CFUNC(obj)) {
        fprintf(stderr, "function not implemented!\n");
        return error_value;
    }

    ASSERT(IS_CFUNC(obj));
    CFuncObject *cfunc = (CFuncObject *)obj;
    Object *owner = cfunc->owner;
    if (IS_MODULE(owner)) {
        ModuleObject *m = (ModuleObject *)owner;
        // raise_exc_str("function not implemented: %s::%s!", m->path, cfunc->name);
        fprintf(stderr, "function '%s' in '%s' is not implemented!\n", cfunc->name, m->path);
    } else {
        ASSERT(IS_TYPE(owner, &type_type));
        TypeObject *tp = (TypeObject *)owner;
        // raise_exc_str("function not implemented: %s!", tp->name);
        fprintf(stderr, "method '%s' of '%s' is not implemented!\n", cfunc->name, tp->name);
    }
    return error_value;
}

static Object *new_not_impl_func(Object *m, char *func_name)
{
    return kl_new_cfunc(func_name, kl_not_impl_func, m);
}

static Object *new_not_impl_trait_func(char *kls_name, char *trait_name, char *fn_name, Object *m)
{
    BUF(buf);
    buf_write_fmt(&buf, "%s_%s_%s", trait_name, kls_name, fn_name);
    return kl_new_cfunc(BUF_STR(buf), kl_not_impl_func, m);
}

static TValue _default___str__(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    TypeObject *tp = kl_typeof(self);
    ASSERT(tp);
    unsigned int hash = kl_hash(self);
    ModuleObject *m = (ModuleObject *)tp->module;

    Object *s = kl_new_fmt_str("<%s.%s object at 0x%x>", m->path, tp->name, hash);
    return obj_value(s);
}

static TValue _default___hash__(TValue *self, TValue *args, int nargs)
{
    unsigned int hash = mem_hash(self, sizeof(TValue));
    return int64_value(hash);
}

static TValue _default___eq__(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    int r = memcmp(self, args, sizeof(TValue));
    return bool_value(r == 0);
}

static TValue _default___ne__(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    int r = memcmp(self, args, sizeof(TValue));
    return bool_value(r != 0);
}

Object *builtin_module;
Object *fs_module;
Object *io_module;
Object *sys_module;

TValue *kl_stdin;
TValue *kl_stdout;
TValue *kl_stderr;
Object *buf_write_str_func;
Object *buf_flush_func;

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

static void load_modules(void)
{
    builtin_module = kl_load_module("std/builtin");
    fs_module = kl_load_module("std/fs");
    io_module = kl_load_module("std/io");
    sys_module = kl_load_module("std/sys");

    kl_stdin = get_global_var(sys_module, "stdin");
    kl_stdout = get_global_var(sys_module, "stdout");
    kl_stderr = get_global_var(sys_module, "stderr");
    ASSERT(kl_stdin && kl_stdout && kl_stderr);

    buf_write_str_func = kl_mo_find(io_module, "buf_write_str");
    buf_flush_func = kl_mo_find(io_module, "buf_flush");
    ASSERT(buf_write_str_func && buf_flush_func);
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

    /* init builtin & sys module */
    // init_builtin_module();

    /* initialize main thread as koala thread */
    ThreadState *ts = mm_alloc_obj(ts);
    ts->current = kl_new_ks();
    __ts = ts;

    /* initialize tag mappings */
    init_tag_mappings();

    load_modules();
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

Object *kl_get_native(Object *m, char *name)
{
    ModuleObject *mo = (ModuleObject *)m;
    NativeLib *lib;
    vector_foreach_ptr(lib, &mo->libs) {
        Object *ob = stbl_find_obj(&lib->symbols, name);
        if (ob) return ob;
    }

    if (match_suffix(name, "__str__")) {
        Object *ob = kl_new_cfunc("__str__", _default___str__, NULL);
        return ob;
    } else if (match_suffix(name, "__hash__")) {
        Object *ob = kl_new_cfunc("__hash__", _default___hash__, NULL);
        return ob;
    } else if (match_suffix(name, "__eq__")) {
        Object *ob = kl_new_cfunc("__eq__", _default___eq__, NULL);
        return ob;
    } else if (match_suffix(name, "__ne__")) {
        Object *ob = kl_new_cfunc("__ne__", _default___ne__, NULL);
        return ob;
    }

    return NULL;
}

int kl_reg_func(NativeLib *lib, char *name, NativeFunc fn)
{
    Object *obj = kl_new_cfunc(name, fn, NULL);
    stbl_add_obj(&lib->symbols, name, obj);
    return 0;
}

int kl_reg_meth(NativeLib *lib, char *cls, char *meth, NativeFunc fn)
{
    char full_name[256];
    snprintf(full_name, sizeof(full_name), "%s$%s", cls, meth);
    Object *obj = kl_new_cfunc(full_name, fn, NULL);
    stbl_add_obj(&lib->symbols, full_name, obj);
    return 0;
}

int kl_reg_type(NativeLib *lib, TypeObject *tp)
{
    kl_init_type(tp);

    MethodDef *def = tp->methdefs;
    while (def && def->name) {
        if (def->cfunc) {
            kl_reg_meth(lib, tp->name, def->name, def->cfunc);
        }
        ++def;
    }

    stbl_add_obj(&lib->symbols, tp->name, (Object *)tp);

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
        _load_native(mo, kc->sval, pkg_name);
    }

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

    Vector *globals = klc->objs + ITEM_VAR;
    mo->num_values = vector_size(globals) - 1;
    ASSERT(mo->num_values >= 0);
    KlcVar *var;
    vector_foreach(var, globals) {
        if (!var) continue;
        kc = klc_get_const(klc, var->name_index);
        Object *val = kl_new_global(kc->sval, i__ - 1, m);
        stbl_add_obj(&mo->symbols, kc->sval, val);
    }

    // bind cfunc/code to module
    Object *fn;
    vector_foreach(fn, &mo->funcs) {
        kl_bind_func(m, fn);
    }

    // allocate global variables space
    if (mo->num_values > 0) {
        mo->values = mm_alloc(sizeof(TValue) * mo->num_values);
        for (uint32_t i = 0; i < mo->num_values; i++) {
            mo->values[i] = none_value;
        }
    }

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
    Object *m = kl_load_module(path);
    if (m) kl_run_main(m);
}

KOALA_EXPORT void koala_finalize(void) { /* finalize atom string table */ fini_atom(); }

#ifdef __cplusplus
}
#endif

/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "atom.h"
#include "codeobject.h"
#include "klc.h"
#include "log.h"
#include "moduleobject.h"

#ifdef __cplusplus
extern "C" {
#endif

static void load_func(KlcFunc *fn, Object *owner, KlcFile *klc)
{
    KlcConst *k = klc_get_const(klc, fn->name_index);
    char *name = k->sval;

    if (IS_MODULE(owner)) {
        // module level function
        Object *fn = kl_new_code(name, owner, NULL);
        module_add_obj(owner, name, fn);
    } else if (IS_KLASS(owner)) {
        // method
        Object *fn = kl_new_code(name, NULL, (TypeObject *)owner);
        type_add_method(owner, name, fn);
    } else {
        // function in function, not supported yet
        UNREACHABLE();
    }
}

static void load_funcs(Object *m, KlcFile *klc)
{
    KlcFunc *item;
    vector_foreach(item, klc->objs + ITEM_FUNC) {
        if (!item) continue;
        load_func(item, m, klc);
    }
}

static void load_const(KlcConst *k, Object *m, KlcFile *klc)
{
    switch (k->type) {
        case KLC_CONST_INT: {
            cp_add_int(m, k->ival);
            break;
        }
        case KLC_CONST_ASCII: // fall-through
        case KLC_CONST_UTF8: {
            cp_add_str(m, k->sval);
            break;
        }
        default: {
            UNREACHABLE();
        }
    }
}

static void load_consts(Object *m, KlcFile *klc)
{
    KlcConst *item;
    vector_foreach(item, klc->objs + ITEM_CONST) {
        if (!item) continue;
        load_const(item, m, klc);
    }
}

static void load_relocs(Object *m, KlcFile *klc)
{
    KlcReloc *item;
    vector_foreach(item, klc->objs + ITEM_RELOC) {
        if (!item) continue;
        char *ns = NULL;
        if (item->ns_index) {
            KlcConst *ns_k = klc_get_const(klc, item->ns_index);
            ns = ns_k->sval;
        }
        KlcConst *sym_k = klc_get_const(klc, item->sym_index);
        char *sym = sym_k->sval;
        int mod_id = kl_add_rel(m, REL_TYPE_MODULE, ns, 0);
        // kl_add_rel(m, REL_TYPE_FUNC, sym, mod_id);
    }
}

// absolute path to klc file
static Object *__load(char *path, char *mod_path)
{
    log_info("read klc file: %s", path);

    KlcFile *klc = read_klc_file(path, 1);
    if (!klc) {
        log_error("failed to read klc file: %s", path);
        return NULL;
    }

    Object *m = kl_new_module(mod_path);

    load_funcs(m, klc);
    load_consts(m, klc);
    load_relocs(m, klc);

    free_klc_file(klc);

    return m;
}

// path without .klc suffix, e.g. "foo/bar/baz" for "foo/bar/baz.klc"
// path is package path not file path
Object *kl_load_module(char *path)
{
    char *koala_path = getenv("KOALA_PATH");
    if (!koala_path) {
        log_info("KOALA_PATH is not set");
        return __load(path, path);
    }

    log_info("KOALA_PATH: %s", koala_path);

    BUF(buf);
    Object *m = NULL;
    char *prefix = NULL;
    int count = str_sep(&koala_path, ':', &prefix);
    while (count > 0) {
        buf_write_nstr(&buf, prefix, count);
        buf_write_str(&buf, path);
        buf_write_str(&buf, ".klc");
        m = __load(BUF_STR(buf), path);
        if (m) {
            log_info("found module '%s' in KOALA_PATH: %s", path, prefix);
            break;
        }

        RESET_BUF(buf);
        count = str_sep(NULL, ':', &prefix);
    }

    FINI_BUF(buf);

    return m;
}

#ifdef __cplusplus
}
#endif

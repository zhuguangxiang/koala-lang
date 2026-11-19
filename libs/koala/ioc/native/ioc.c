/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * A minimal IoC container: register a class as a component, resolve it as a
 * singleton. The first get creates the instance (running its __init__) and
 * caches it; later gets return the same object.
 *
 * Lifecycle is handled entirely by the GC, so the package needs no runtime
 * init/fini hooks. The container is a perm GC root whose _mark keeps every
 * cached instance alive across collections. Each bean is a perm (non-root) GC
 * node: never swept, and freed by gc_fini along with the container. Nothing is
 * mm_alloc'd, so there is nothing to leak and nothing to tear down by hand.
 */

#include "excobj.h"
#include "gc.h"
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

/* A registered component: its type plus the cached singleton (NULL until first
 * get). Perm GC node, linked into the container's list.
 */
typedef struct _Bean {
    OBJECT_HEAD
    TypeObject *tp;
    Object *instance;
    struct _Bean *next;
} Bean;

static TypeObject ioc_bean_type = {
    ._type = &type_type,
    .name = "ioc_bean",
    .flags = TP_FLAGS_CLASS,
};

/* The container is a perm GC root; its _mark keeps every cached instance alive. */
typedef struct _IocContainer {
    OBJECT_HEAD
    Bean *head;
} IocContainer;

static void ioc_container_mark(Object *self)
{
    IocContainer *c = (IocContainer *)self;
    for (Bean *b = c->head; b; b = b->next) {
        if (b->instance) gc_mark(b->instance);
    }
}

static TypeObject ioc_container_type = {
    ._type = &type_type,
    .name = "ioc_container",
    .flags = TP_FLAGS_CLASS,
    ._mark = ioc_container_mark,
};

static IocContainer *_ioc;

static Bean *_ioc_find(TypeObject *tp)
{
    for (Bean *b = _ioc->head; b; b = b->next) {
        if (b->tp == tp) return b;
    }
    return NULL;
}

/* A component must be a concrete, non-generic class with a no-argument
 * constructor. Traits never reach the runtime as type values, so they cannot
 * be registered in the first place. Raises and returns -1 on failure.
 */
static int _ioc_validate(TypeObject *tp)
{
    if (tp->flags & TP_FLAGS_GENERIC) {
        raise_exc_fmt("ioc: generic class '%s' is not supported as a component", tp->name);
        return -1;
    }

    Object *init = stbl_find_obj(&tp->members, "__init__");
    if (init && IS_CODE(init) && ((CodeObject *)init)->cs.nargs > 0) {
        raise_exc_fmt("ioc: component '%s' must have a no-argument __init__", tp->name);
        return -1;
    }
    return 0;
}

static int _ioc_register(TypeObject *tp)
{
    if (_ioc_find(tp)) return 0;
    if (_ioc_validate(tp) < 0) return -1;
    Bean *b = gc_alloc_perm_obj(b, 0);
    INIT_OBJECT_HEAD(b, &ioc_bean_type, 0);
    b->tp = tp;
    b->instance = NULL;
    b->next = _ioc->head;
    _ioc->head = b;
    return 0;
}

static Object *_ioc_get(TypeObject *tp)
{
    Bean *b = _ioc_find(tp);
    if (!b) {
        raise_exc_fmt("ioc: type '%s' is not a registered component", tp->name);
        return NULL;
    }

    if (!b->instance) {
        b->instance = kl_new_instance(tp);
        Object *init = stbl_find_obj(&tp->members, "__init__");
        if (init) {
            TValue self = obj_value(b->instance);
            kl_call_code(init, &self, 1);
        }
    }
    return b->instance;
}

// func register(tp type) {}
static TValue _register(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    TypeObject *tp = kl_arg_obj_as(0, type_type);
    if (_ioc_register(tp) < 0) return error_value;
    return no_value;
}

// func get(tp type) any {}
static TValue _get(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    TypeObject *tp = kl_arg_obj_as(0, type_type);
    Object *obj = _ioc_get(tp);
    if (!obj) return error_value;
    return obj_value(obj);
}

/* Entry point called by the runtime after dlopen("libkoala_ioc.so"). The name
 * must be <last segment of the package path>_native_lib_init.
 */
void ioc_native_lib_init(NativeLib *lib)
{
    _ioc = gc_alloc_perm_obj(_ioc, 1);
    INIT_OBJECT_HEAD(_ioc, &ioc_container_type, 0);
    _ioc->head = NULL;

    kl_reg_func(lib, "register", _register);
    kl_reg_func(lib, "get", _get);
}

#ifdef __cplusplus
}
#endif

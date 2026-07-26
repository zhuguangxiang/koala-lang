/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "buffer.h"
#include "cfuncobject.h"
#include "exception.h"
#include "moduleobject.h"
#include "object.h"
#include "shadowstack.h"
#include "stringobject.h"
#include "tupleobject.h"

#ifdef __cplusplus
extern "C" {
#endif

static void init_types(Object *m)
{
    type_ready(&any_type);
    type_ready(&type_type);
    type_ready(&none_type);
    type_ready(&exc_type);
    type_ready(&int8_type);
    type_ready(&int16_type);
    type_ready(&int32_type);
    type_ready(&int64_type);
    type_ready(&uint8_type);
    type_ready(&uint16_type);
    type_ready(&uint32_type);
    type_ready(&uint64_type);
    type_ready(&str_type);
    type_ready(&tuple_type);
    type_ready(&cfunc_type);
    type_ready(&code_type);
    type_ready(&Iterable_type);
    type_ready(&Iterator_type);
    type_ready(&Collection_type);
    type_ready(&Sequence_type);
    type_ready(&MutableSequence_type);
}

static void builtin_print_impl(Value *args, int nargs, Value *_sep, Value *_end, Value *_file)
{
    const char *sep = " ";
    const char *end = "\n";
    Object *file = NULL;

    if (!is_none(_sep)) {
        Object *obj = to_obj(_sep);
        ASSERT(IS_STR(obj));
        sep = STR_BUF(obj);
    }

    if (!is_none(_end)) {
        Object *obj = to_obj(_end);
        ASSERT(IS_STR(obj));
        end = STR_BUF(obj);
    }

    if (is_none(_file)) {
        // TODO: sys.stdout
        file = NULL;
    }

    BUF(buf);

    for (int i = 0; i < nargs; i++) {
        if (i != 0) {
            buf_write_str(&buf, sep);
        }

        Value *arg = args + i;
        TypeObject *tp = object_typeof(arg);
        if (tp->str) {
            Value s = tp->str(arg);
            buf_write_str(&buf, STR_BUF(to_obj(&s)));
        } else {
            /* fallback to type name */
            buf_write_str(&buf, tp->name);
        }
    }

    buf_write_str(&buf, end);

    // TODO: sys.stdout
    printf("%s", BUF_STR(buf));

    FINI_BUF(buf);
}

/*
func print(objs ..., sep = ' ', end = '\n', file io.Writer? = none)
*/
static Value builtin_print(Value *m, Value *args, int nargs, Object *names)
{
    Value _sep = none_value;
    Value _end = none_value;
    Value _file = none_value;
    // const char *_kws[] = { "sep", "end", "file", NULL };
    // kl_parse_kwargs(args, nargs, names, nargs, _kws, &_sep, &_end, &_file);

    builtin_print_impl(args, nargs, &_sep, &_end, &_file);
    return none_value;
}

static MethodDef builtin_methods[] = {
    { "print", builtin_print, METH_VAR_NAMES },
    // { "format", builtin_format, METH_VAR_NAMES },
    { NULL },
};

static int builtin_module_init(Object *m) { init_types(m); }

static ModuleDef builtin_module = {
    .name = "std/builtin",
    .size = 0,
    .methods = builtin_methods,
    .init = builtin_module_init,
    .fini = NULL,
};

void init_builtin_module(void) { kl_module_from_moddef(&builtin_module); }

#ifdef __cplusplus
}
#endif

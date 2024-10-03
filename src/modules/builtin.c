/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "buffer.h"
#include "exception.h"
#include "moduleobject.h"
#include "object.h"
#include "shadowstack.h"
#include "stringobject.h"
#include "tupleobject.h"

#ifdef __cplusplus
extern "C" {
#endif

extern TypeObject int_type;

static void init_types(Object *m)
{
    type_ready(&base_type, m);
    type_ready(&type_type, m);
    type_ready(&void_type, m);
    type_ready(&none_type, m);
    type_ready(&exc_type, m);
    type_ready(&int_type, m);
    type_ready(&str_type, m);
    type_ready(&tuple_type, m);
}

static void builtin_print_impl(Value *args, int nargs, Value *_sep, Value *_end,
                               Value *_file)
{
    const char *sep = " ";
    const char *end = "\n";
    Object *file = NULL;

    if (!IS_UNDEF(_sep)) {
        Object *obj = as_obj(_sep);
        ASSERT(IS_STR(obj));
        sep = STR_BUF(obj);
    }

    if (!IS_UNDEF(_end)) {
        Object *obj = as_obj(_end);
        ASSERT(IS_STR(obj));
        end = STR_BUF(obj);
    }

    if (IS_NONE(_file)) {
        // TODO: sys.stdout
        file = NULL;
    }

    BUF(buf);

    for (int i = 0; i < nargs; i++) {
        if (i != 0) {
            buf_write_str(&buf, sep);
        }

        Value *arg = args + i;
        if (IS_NONE(arg)) {
            buf_write_str(&buf, "none");
        } else if (IS_INT(arg)) {
            buf_write_int64(&buf, arg->ival);
        } else if (IS_FLOAT(arg)) {
            buf_write_double(&buf, arg->fval);
        } else if (IS_OBJ(arg)) {
            Object *obj = to_obj(arg);
            const char *s;
            int len;
            if (IS_STR(obj)) {
                s = STR_BUF(obj);
                len = STR_LEN(obj);
                buf_write_char(&buf, '\'');
                buf_write_nstr(&buf, s, len);
                buf_write_char(&buf, '\'');
            } else {
                Value r = object_str(arg);
                obj = as_obj(&r);
                s = STR_BUF(obj);
                len = STR_LEN(obj);
                buf_write_nstr(&buf, s, len);
            }
        } else {
            UNREACHABLE();
        }
    }

    buf_write_str(&buf, end);

    // TODO: sys.stdout
    printf("%s", BUF_STR(buf));

    FINI_BUF(buf);
}

/*
public func print(objs ..., sep = ' ', end = '\n', file io.Writer? = none)
*/
static Value builtin_print(Value *module, Value *args, int nargs, Object *names)
{
    Value _sep = undef_value;
    Value _end = undef_value;
    Value _file = none_value;
    const char *_kws[] = { "sep", "end", "file", NULL };
    kl_parse_kwargs(args, nargs, names, nargs, _kws, &_sep, &_end, &_file);

    builtin_print_impl(args, nargs, &_sep, &_end, &_file);
    return none_value;
}

static MethodDef builtin_methods[] = {
    { "print", builtin_print, METH_VAR_NAMES, "...|sep:s,end:s,file:Lio.Writer;", "" },
    { NULL },
};

static int builtin_module_init(Object *module) { init_types(module); }

static ModuleDef builtin_module = {
    .name = "builtin",
    .size = 0,
    .methods = builtin_methods,
    .init = builtin_module_init,
    .fini = NULL,
};

void init_builtin_module(void) { kl_module_def_init(&builtin_module); }

#ifdef __cplusplus
}
#endif

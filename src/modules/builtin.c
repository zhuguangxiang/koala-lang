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

/*
public func print(objs ..., sep = ' ', end = '\n', file io.Writer = none)
*/
static Value builtin_print(Value *module, Value *args, int nargs, Object *names)
{
    char *sep = " ";
    char *end = "\n";
    Object *file = NULL;
    const char *kws[] = { "sep", "end", "file", NULL };
    kl_parse_kwargs(args, nargs, names, nargs, kws, &sep, &end, &file);

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
        } else if (IS_OBJECT(arg)) {
            Object *obj = arg->obj;
            const char *s;
            int len;
            if (IS_STR(obj)) {
                s = STR_BUF(obj);
                len = STR_LEN(obj);
                buf_write_char(&buf, '\'');
                buf_write_nstr(&buf, (char *)s, len);
                buf_write_char(&buf, '\'');
            } else {
                Value self = object_value(obj);
                Value r = object_str(&self);
                obj = value_as_object(&r);
                s = STR_BUF(obj);
                len = STR_LEN(obj);
                buf_write_nstr(&buf, (char *)s, len);
            }
        } else {
            UNREACHABLE();
        }
    }

    buf_write_str(&buf, end);

    // TODO:
    printf("%s", BUF_STR(buf));

    FINI_BUF(buf);

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

/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "buffer.h"
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

// func pretty(s str) str {}
static TValue kl_pretty(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    const char *p = kl_arg_str(0);
    int len = strlen(p);

    BUF(out);

    int indent = 0;
    int at_line_start = 1;

#define WRITE_INDENT() \
    if (at_line_start) { \
        for (int _i = 0; _i < indent; _i++) buf_write_byte(&out, ' '); \
        at_line_start = 0; \
    }

    for (int i = 0; i < len; i++) {
        char ch = p[i];

        // Skip ALL leading spaces at line start
        if (at_line_start && ch == ' ') {
            continue;
        }

        if (ch == '{' || ch == '[') {
            WRITE_INDENT();
            buf_write_byte(&out, ch);
            buf_write_byte(&out, '\n');
            indent += 4;
            at_line_start = 1;
        } else if (ch == '}' || ch == ']') {
            buf_write_byte(&out, '\n');
            indent -= 4;
            at_line_start = 1;
            WRITE_INDENT();
            buf_write_byte(&out, ch);
        } else if (ch == ',') {
            buf_write_byte(&out, ch);
            buf_write_byte(&out, '\n');
            at_line_start = 1;
        } else if (ch == '\n' || ch == '\t') {
            continue;
        } else {
            WRITE_INDENT();
            buf_write_byte(&out, ch);
        }
    }

    Object *so = kl_new_nstr(BUF_STR(out), BUF_LEN(out));
    FINI_BUF(out);
    return obj_value(so);
}

void pretty_native_lib_init(NativeLib *lib) { kl_reg_func(lib, "pretty", kl_pretty); }

#ifdef __cplusplus
}
#endif

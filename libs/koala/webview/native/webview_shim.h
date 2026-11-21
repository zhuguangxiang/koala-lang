/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * Shared helpers for the koala/webview native module.
 *
 * The window engine is implemented from scratch in pure C (webview_impl.c)
 * on top of the system backend's C API: WebKitGTK on Linux, WebView2 on
 * Windows and WKWebView on macOS (the latter two not implemented yet). No
 * third-party code is vendored or linked and no browser engine is bundled.
 *
 * Files:
 *   - webview_shim.c: module init, the Window type, version()
 *   - webview_impl.c: the C window engine behind webview.h
 */

#ifndef KOALA_WEBVIEW_SHIM_H
#define KOALA_WEBVIEW_SHIM_H

#include <string.h>
#include "object.h"

/* webview.h selects the backend macro (WEBVIEW_GTK/COCOA/EDGE). */
#include "webview.h"

#if defined(WEBVIEW_GTK)
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#endif

/* Copy a `str` argument into a NUL-terminated C string the webview API can
 * read. The Koala declaration types the parameter `str`, so a non-string is
 * rejected by the compiler. The caller frees the result. */
static inline char *webview_arg_str(TValue *args, int index)
{
    Object *o = to_obj(args + index);
    ASSERT(IS_STR(o));
    return strndup(STR_BUF(o), STR_LEN(o));
}

/* Registration entry points. */
void webview_reg_funcs(NativeLib *lib);
void webview_reg_window(NativeLib *lib);

#endif /* KOALA_WEBVIEW_SHIM_H */

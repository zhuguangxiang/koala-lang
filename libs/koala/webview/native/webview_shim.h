/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * Shared helpers for the koala/webview native module.
 *
 * The window engine is the webview library (0.12.0) vendored unmodified in
 * `native/webview/` (see `native/webview/LICENSE` and `native/webview/VERSION`).
 * Upstream: https://github.com/webview/webview (tag 0.12.0)
 * Header:   https://raw.githubusercontent.com/webview/webview/0.12.0/core/include/webview/webview.h
 * Tarball:  https://codeload.github.com/webview/webview/tar.gz/refs/tags/0.12.0
 * Its public API is C ABI, but its implementation is C++, so this module is
 * compiled with a C++ compiler (`compiler = "c++"` in `__pkg__.toml`) and links
 * the system webview backend: WebKitGTK on Linux, WebView2 on Windows, WKWebView
 * on macOS. No browser engine is bundled.
 *
 * One C++ file does the binding:
 *   - webview_shim.cpp: module init, the Window type, version()
 */

#ifndef KOALA_WEBVIEW_SHIM_H
#define KOALA_WEBVIEW_SHIM_H

#include <stdlib.h>
#include <string.h>
#include "object.h"
#include "excobj.h"
#include "webview.h"

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
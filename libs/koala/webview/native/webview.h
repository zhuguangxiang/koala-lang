/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * koala/webview native engine: public C API.
 *
 * A small window engine implemented from scratch in C on top of the system
 * webview backend (WebKitGTK on Linux). The API shape follows the well-known
 * webview C API conventions (webview_create/set_html/eval/run/...), but no
 * third-party code is vendored or linked: the implementation lives in
 * webview_impl.c and the Koala binding in webview_shim.c.
 *
 * Only the owned-window mode is supported (webview_create(debug, NULL));
 * embedding into a foreign native window is not implemented.
 */

#ifndef KOALA_WEBVIEW_H
#define KOALA_WEBVIEW_H

/* Backend selection (one per platform). */
#if !defined(WEBVIEW_GTK) && !defined(WEBVIEW_COCOA) && !defined(WEBVIEW_EDGE)
#if defined(__APPLE__)
#define WEBVIEW_COCOA
#elif defined(__unix__)
#define WEBVIEW_GTK
#elif defined(_WIN32)
#define WEBVIEW_EDGE
#else
#error "webview: no backend selected"
#endif
#endif

typedef void *webview_t;

typedef enum {
    WEBVIEW_NATIVE_HANDLE_KIND_UI_WINDOW,
    WEBVIEW_NATIVE_HANDLE_KIND_UI_WIDGET,
    WEBVIEW_NATIVE_HANDLE_KIND_BROWSER_CONTROLLER
} webview_native_handle_kind_t;

typedef enum {
    WEBVIEW_HINT_NONE,
    WEBVIEW_HINT_MIN,
    WEBVIEW_HINT_MAX,
    WEBVIEW_HINT_FIXED
} webview_hint_t;

typedef enum {
    WEBVIEW_ERROR_MISSING_DEPENDENCY = -5,
    WEBVIEW_ERROR_CANCELED = -4,
    WEBVIEW_ERROR_INVALID_STATE = -3,
    WEBVIEW_ERROR_INVALID_ARGUMENT = -2,
    WEBVIEW_ERROR_UNSPECIFIED = -1,
    WEBVIEW_ERROR_OK = 0,
    WEBVIEW_ERROR_DUPLICATE = 1,
    WEBVIEW_ERROR_NOT_FOUND = 2
} webview_error_t;

typedef struct {
    unsigned int major;
    unsigned int minor;
    unsigned int patch;
} webview_version_t;

typedef struct {
    webview_version_t version;
    char version_number[32];
    char pre_release[48];
    char build_metadata[48];
} webview_version_info_t;

/* Create a window (debug enables the WebKit inspector). Returns NULL on
 * failure. `window` must be NULL: foreign-window embedding is unsupported. */
webview_t webview_create(int debug, void *window);

/* Close the window and release the instance. */
webview_error_t webview_destroy(webview_t w);

/* Block in the event loop until terminate() or the window is closed. */
webview_error_t webview_run(webview_t w);

/* Ask run() to return (asynchronously, via the event loop). */
webview_error_t webview_terminate(webview_t w);

/* Queue fn(w, arg) to run on the event loop thread. */
webview_error_t webview_dispatch(webview_t w, void (*fn)(webview_t, void *),
                                 void *arg);

/* The native window handle, or NULL once the window is closed. */
void *webview_get_window(webview_t w);

/* UI_WINDOW -> GtkWindow*, UI_WIDGET / BROWSER_CONTROLLER -> WebKitWebView*.
 * NULL when unavailable. */
void *webview_get_native_handle(webview_t w, webview_native_handle_kind_t kind);

webview_error_t webview_set_title(webview_t w, const char *title);
webview_error_t webview_set_size(webview_t w, int width, int height,
                                 webview_hint_t hints);
webview_error_t webview_navigate(webview_t w, const char *url);
webview_error_t webview_set_html(webview_t w, const char *html);

/* Run js on every page load (injected at document start). */
webview_error_t webview_init(webview_t w, const char *js);

/* Run js now; silently skipped until content has begun loading. */
webview_error_t webview_eval(webview_t w, const char *js);

/* Engine version (the koala/webview package version). */
const webview_version_info_t *webview_version(void);

#endif /* KOALA_WEBVIEW_H */
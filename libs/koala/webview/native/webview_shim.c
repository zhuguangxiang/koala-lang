/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * koala/webview: a native desktop window backed by the system webview.
 *
 * The Window type wraps one `webview_t` handle. The member names mirror the
 * webview C API (`webview_set_title` -> `set_title`, `webview_init` ->
 * `init_js`, and so on) so the upstream documentation applies directly.
 *
 * `destroy()` must be called to release the window: the runtime does not run
 * `TypeObject._free` yet (see `kl_free_instance()`), the same constraint the
 * zlib stream objects document. `_free` is still provided for when it does.
 *
 * Binding JS calls back into Koala (`webview_bind`/`webview_dispatch`) is not
 * exposed yet: it needs a native->Koala callback with GC rooting. Instead,
 * the GTK backend offers poll()/eval_sync(): a Koala-side pump loop drains a
 * JS outbox queue via eval_sync and pushes updates back with eval(), which
 * covers the dashboard/bridge use cases without any native->Koala call.
 */

#include "webview_shim.h"
#include "tupleobj.h"

typedef struct _WindowObject {
    OBJECT_HEAD
    webview_t w;
    /* 1 once webview_create() succeeded and destroy() has not run. */
    int created;
} WindowObject;

static TypeObject window_type;

#define WIN(self) ((WindowObject *)SELF_AS(window_type))

static int _win_ready(WindowObject *wo)
{
    if (!wo->created) {
        raise_exc_str("webview: window is destroyed");
        return 0;
    }
    return 1;
}

// __init__(debug bool)
static TValue _window_init(TValue *self, TValue *args, int nargs)
{
    WindowObject *wo = WIN(self);
    int debug;

    ASSERT(nargs == 1);
    debug = kl_arg_bool(0) ? 1 : 0;

    wo->w = webview_create(debug, NULL);
    wo->created = (wo->w != NULL) ? 1 : 0;
    if (!wo->created) {
        raise_exc_str("webview: failed to create window");
        return error_value;
    }
    return no_value;
}

// set_title(title str)
static TValue _window_set_title(TValue *self, TValue *args, int nargs)
{
    WindowObject *wo = WIN(self);
    char *title;

    ASSERT(nargs == 1);
    if (!_win_ready(wo)) return error_value;
    title = webview_arg_str(args, 0);
    webview_set_title(wo->w, title);
    free(title);
    return no_value;
}

// set_size(width int, height int, hints int)
static TValue _window_set_size(TValue *self, TValue *args, int nargs)
{
    WindowObject *wo = WIN(self);
    int width, height, hints;

    ASSERT(nargs == 3);
    if (!_win_ready(wo)) return error_value;
    width = (int)kl_arg_int(0);
    height = (int)kl_arg_int(1);
    hints = (int)kl_arg_int(2);
    webview_set_size(wo->w, width, height, (webview_hint_t)hints);
    return no_value;
}

// navigate(url str)
static TValue _window_navigate(TValue *self, TValue *args, int nargs)
{
    WindowObject *wo = WIN(self);
    char *url;

    ASSERT(nargs == 1);
    if (!_win_ready(wo)) return error_value;
    url = webview_arg_str(args, 0);
    webview_navigate(wo->w, url);
    free(url);
    return no_value;
}

// set_html(html str)
static TValue _window_set_html(TValue *self, TValue *args, int nargs)
{
    WindowObject *wo = WIN(self);
    char *html;

    ASSERT(nargs == 1);
    if (!_win_ready(wo)) return error_value;
    html = webview_arg_str(args, 0);
    webview_set_html(wo->w, html);
    free(html);
    return no_value;
}

// init_js(js str)
static TValue _window_init_js(TValue *self, TValue *args, int nargs)
{
    WindowObject *wo = WIN(self);
    char *js;

    ASSERT(nargs == 1);
    if (!_win_ready(wo)) return error_value;
    js = webview_arg_str(args, 0);
    webview_init(wo->w, js);
    free(js);
    return no_value;
}

// eval(js str)
static TValue _window_eval(TValue *self, TValue *args, int nargs)
{
    WindowObject *wo = WIN(self);
    char *js;

    ASSERT(nargs == 1);
    if (!_win_ready(wo)) return error_value;
    js = webview_arg_str(args, 0);
    webview_eval(wo->w, js);
    free(js);
    return no_value;
}

#if defined(WEBVIEW_GTK)
/* eval_sync runs the upstream evaluate API with a result callback and pumps
 * the GTK main context until it fires. The callback stays entirely in C++
 * (no native->Koala call), so it needs no GC rooting.
 *
 * The context is heap-allocated: on timeout (or window close mid-eval) it is
 * marked abandoned and ownership passes to the callback, which frees it
 * whenever it eventually fires. A JS script that never yields (sync infinite
 * loop) can therefore not write into a returned stack frame, nor hang the
 * caller past the timeout. */
typedef struct {
    int done;
    int failed;
    int abandoned;
    char *result;
} EvalSyncCtx;

static void _eval_sync_cb(GObject *src, GAsyncResult *res, gpointer data)
{
    EvalSyncCtx *ctx = (EvalSyncCtx *)data;
    GError *err = NULL;
    char *result = NULL;
#if (WEBKIT_MAJOR_VERSION == 2 && WEBKIT_MINOR_VERSION >= 40) || WEBKIT_MAJOR_VERSION > 2
    JSCValue *val = webkit_web_view_evaluate_javascript_finish(WEBKIT_WEB_VIEW(src), res, &err);
    if (val) {
        result = jsc_value_to_string(val);
        g_object_unref(val);
    }
#else
    WebKitJavascriptResult *jr =
        webkit_web_view_run_javascript_finish(WEBKIT_WEB_VIEW(src), res, &err);
    if (jr) {
        JSCValue *val = webkit_javascript_result_get_js_value(jr);
        result = jsc_value_to_string(val);
        webkit_javascript_result_unref(jr);
    }
#endif
    if (ctx->abandoned) {
        if (result) g_free(result);
        if (err) g_error_free(err);
        g_free(ctx);
        return;
    }
    ctx->result = result;
    if (err) {
        ctx->failed = 1;
        g_error_free(err);
    }
    ctx->done = 1;
}
#endif /* WEBVIEW_GTK */

// eval_sync(js str, timeout_ms int) -> str
static TValue _window_eval_sync(TValue *self, TValue *args, int nargs)
{
    WindowObject *wo = WIN(self);

    ASSERT(nargs == 2);
    if (!_win_ready(wo)) return error_value;
#if defined(WEBVIEW_GTK)
    char *js = webview_arg_str(args, 0);
    int64_t timeout_ms = kl_arg_int(1);
    WebKitWebView *wv = (WebKitWebView *)webview_get_native_handle(
        wo->w, WEBVIEW_NATIVE_HANDLE_KIND_BROWSER_CONTROLLER);
    if (!wv) {
        free(js);
        raise_exc_str("webview: window is closed");
        return error_value;
    }
    /* Upstream eval silently skips when no page has loaded; be explicit. */
    if (!webkit_web_view_get_uri(wv)) {
        free(js);
        raise_exc_str("webview: no page loaded; set_html/navigate first and poll until loaded");
        return error_value;
    }

    EvalSyncCtx *ctx = g_new0(EvalSyncCtx, 1);
    GCancellable *cancel = g_cancellable_new();
#if (WEBKIT_MAJOR_VERSION == 2 && WEBKIT_MINOR_VERSION >= 40) || WEBKIT_MAJOR_VERSION > 2
    webkit_web_view_evaluate_javascript(wv, js, (gssize)strlen(js), NULL, NULL, cancel,
                                        _eval_sync_cb, ctx);
#else
    webkit_web_view_run_javascript(wv, js, cancel, _eval_sync_cb, ctx);
#endif
    gint64 deadline = g_get_monotonic_time() + timeout_ms * 1000;
    gint64 abandon_deadline = 0;
    int timed_out = 0;
    int closed = 0;
    while (!ctx->done) {
        gint64 now = g_get_monotonic_time();
        if (!timed_out && now > deadline) {
            g_cancellable_cancel(cancel);
            timed_out = 1;
            /* Grace period for the cancellation callback, then abandon. */
            abandon_deadline = now + 200 * 1000;
        }
        if (timed_out && now > abandon_deadline) break;
        if (!webview_get_window(wo->w)) {
            g_cancellable_cancel(cancel);
            closed = 1;
            break;
        }
        g_main_context_iteration(NULL, TRUE);
    }
    g_object_unref(cancel);
    free(js);

    if (!ctx->done) {
        ctx->abandoned = 1; /* callback owns ctx from here on */
        if (closed) {
            /* Closing the window is a normal exit path for the pump loop,
               not an error: report an empty result and let poll() end it. */
            return kl_val_str((char *)"");
        }
        raise_exc_str((char *)"webview: eval_sync timed out");
        return error_value;
    }
    if (ctx->failed || !ctx->result) {
        if (ctx->result) g_free(ctx->result);
        g_free(ctx);
        if (timed_out) {
            /* Cancellation surfaces as a callback error; report the timeout. */
            raise_exc_str("webview: eval_sync timed out");
            return error_value;
        }
        if (!webview_get_window(wo->w)) {
            /* Window closed mid-eval: normal pump-loop exit, not an error. */
            return kl_val_str((char *)"");
        }
        raise_exc_str("webview: eval_sync failed (JS exception or no result)");
        return error_value;
    }
    TValue ret = kl_val_str(ctx->result);
    g_free(ctx->result);
    g_free(ctx);
    return ret;
#elif defined(WEBVIEW_EDGE)
    /* TODO: ICoreWebView2::ExecuteScript + message-loop pump. */
    raise_exc_str("webview: eval_sync is not implemented on the Windows (WebView2) backend yet");
    return error_value;
#elif defined(WEBVIEW_COCOA)
    /* TODO: WKWebView evaluateJavaScript:completionHandler: + CFRunLoop pump. */
    raise_exc_str("webview: eval_sync is not implemented on the macOS (WKWebView) backend yet");
    return error_value;
#else
#error "webview: no backend selected"
#endif
}

// run()
static TValue _window_run(TValue *self, TValue *args, int nargs)
{
    WindowObject *wo = WIN(self);

    ASSERT(nargs == 0);
    if (!_win_ready(wo)) return error_value;
    webview_run(wo->w);
    return no_value;
}

// poll() -> bool
// Step the event loop once without blocking, then report whether the window
// is still open. The GTK engine nulls its window handle on the "destroy"
// signal, so webview_get_window() doubles as the closed-window probe.
static TValue _window_poll(TValue *self, TValue *args, int nargs)
{
    WindowObject *wo = WIN(self);

    ASSERT(nargs == 0);
    if (!_win_ready(wo)) return error_value;
#if defined(WEBVIEW_GTK)
    g_main_context_iteration(NULL, FALSE);
    return bool_value(webview_get_window(wo->w) != NULL);
#elif defined(WEBVIEW_EDGE)
    /* TODO: PeekMessage/DispatchMessage pump + closed-window probe. */
    raise_exc_str("webview: poll is not implemented on the Windows (WebView2) backend yet");
    return error_value;
#elif defined(WEBVIEW_COCOA)
    /* TODO: CFRunLoopRunInMode / nextEventMatchingMask pump + closed probe. */
    raise_exc_str("webview: poll is not implemented on the macOS (WKWebView) backend yet");
    return error_value;
#else
#error "webview: no backend selected"
#endif
}

// is_open() -> bool
static TValue _window_is_open(TValue *self, TValue *args, int nargs)
{
    WindowObject *wo = WIN(self);

    ASSERT(nargs == 0);
    if (!_win_ready(wo)) return error_value;
    return bool_value(webview_get_window(wo->w) != NULL);
}

// set_fullscreen(on bool)
static TValue _window_set_fullscreen(TValue *self, TValue *args, int nargs)
{
    WindowObject *wo = WIN(self);

    ASSERT(nargs == 1);
    if (!_win_ready(wo)) return error_value;
#if defined(WEBVIEW_GTK)
    GtkWindow *win = (GtkWindow *)webview_get_window(wo->w);
    if (!win) {
        raise_exc_str("webview: window is closed");
        return error_value;
    }
    if (kl_arg_bool(0)) {
        gtk_window_fullscreen(win);
    } else {
        gtk_window_unfullscreen(win);
    }
    return no_value;
#elif defined(WEBVIEW_EDGE)
    /* TODO: Win32 fullscreen on the HWND from webview_get_window. */
    raise_exc_str(
        "webview: set_fullscreen is not implemented on the Windows (WebView2) backend yet");
    return error_value;
#elif defined(WEBVIEW_COCOA)
    /* TODO: [NSWindow toggleFullScreen:]. */
    raise_exc_str(
        "webview: set_fullscreen is not implemented on the macOS (WKWebView) backend yet");
    return error_value;
#else
#error "webview: no backend selected"
#endif
}

// get_size() -> (int, int)
static TValue _window_get_size(TValue *self, TValue *args, int nargs)
{
    WindowObject *wo = WIN(self);

    ASSERT(nargs == 0);
    if (!_win_ready(wo)) return error_value;
#if defined(WEBVIEW_GTK)
    GtkWindow *win = (GtkWindow *)webview_get_window(wo->w);
    if (!win) {
        raise_exc_str("webview: window is closed");
        return error_value;
    }
    gint width = 0, height = 0;
    gtk_window_get_size(win, &width, &height);
    TValue items[2] = { int64_value(width), int64_value(height) };
    return obj_value(kl_new_tuple(items, 2));
#elif defined(WEBVIEW_EDGE)
    /* TODO: GetClientRect on the HWND from webview_get_window. */
    raise_exc_str("webview: get_size is not implemented on the Windows (WebView2) backend yet");
    return error_value;
#elif defined(WEBVIEW_COCOA)
    /* TODO: NSWindow frame size. */
    raise_exc_str("webview: get_size is not implemented on the macOS (WKWebView) backend yet");
    return error_value;
#else
#error "webview: no backend selected"
#endif
}

// terminate()
static TValue _window_terminate(TValue *self, TValue *args, int nargs)
{
    WindowObject *wo = WIN(self);

    ASSERT(nargs == 0);
    if (!_win_ready(wo)) return error_value;
    webview_terminate(wo->w);
    return no_value;
}

// destroy()
static TValue _window_destroy(TValue *self, TValue *args, int nargs)
{
    WindowObject *wo = WIN(self);

    ASSERT(nargs == 0);
    if (wo->created) {
        webview_destroy(wo->w);
        wo->w = NULL;
        wo->created = 0;
    }
    return no_value;
}

static void _window_free(Object *self)
{
    WindowObject *wo = (WindowObject *)self;

    if (wo->created) {
        webview_destroy(wo->w);
        wo->w = NULL;
        wo->created = 0;
    }
}

static MethodDef window_methods[] = {
    { "__init__", _window_init },     { "set_title", _window_set_title },
    { "set_size", _window_set_size }, { "navigate", _window_navigate },
    { "set_html", _window_set_html }, { "init_js", _window_init_js },
    { "eval", _window_eval },         { "eval_sync", _window_eval_sync },
    { "run", _window_run },           { "poll", _window_poll },
    { "is_open", _window_is_open },   { "set_fullscreen", _window_set_fullscreen },
    { "get_size", _window_get_size }, { "terminate", _window_terminate },
    { "destroy", _window_destroy },   { NULL, NULL },
};

void webview_reg_window(NativeLib *lib)
{
    window_type._type = &type_type;
    window_type._nfields = 0;
    window_type.name = (char *)"Window";
    window_type.objsize = sizeof(WindowObject);
    window_type.flags = TP_FLAGS_CLASS;
    window_type.methdefs = window_methods;
    window_type._free = _window_free;
    kl_reg_type(lib, &window_type);
}

// _version() str
static TValue _version(TValue *self, TValue *args, int nargs)
{
    const webview_version_info_t *info;

    ASSERT(nargs == 0);
    info = webview_version();
    return kl_val_str((char *)info->version_number);
}

void webview_reg_funcs(NativeLib *lib) { kl_reg_func(lib, "_version", _version); }

/* Entry point called by the runtime after dlopen("libkoala_webview.so").
 * The name must be `<last segment of the package path>_native_lib_init`, so
 * `koala/webview` becomes `webview_native_lib_init`. Compiled as C, the
 * symbol stays unmangled for dlsym. */
void webview_native_lib_init(NativeLib *lib)
{
    webview_reg_funcs(lib);
    webview_reg_window(lib);
}

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
 * exposed yet: it needs a native->Koala callback with GC rooting. The window,
 * HTML, navigation and `eval` surface below is enough to drive a dashboard
 * served by koala/http and updated over koala/sse.
 */

#include "webview_shim.h"

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
    webview_set_size(wo->w, width, height, hints);
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

// run()
static TValue _window_run(TValue *self, TValue *args, int nargs)
{
    WindowObject *wo = WIN(self);

    ASSERT(nargs == 0);
    if (!_win_ready(wo)) return error_value;
    webview_run(wo->w);
    return no_value;
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
    { "__init__", _window_init },
    { "set_title", _window_set_title },
    { "set_size", _window_set_size },
    { "navigate", _window_navigate },
    { "set_html", _window_set_html },
    { "init_js", _window_init_js },
    { "eval", _window_eval },
    { "run", _window_run },
    { "terminate", _window_terminate },
    { "destroy", _window_destroy },
    { NULL, NULL },
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

void webview_reg_funcs(NativeLib *lib)
{
    kl_reg_func(lib, "_version", _version);
}

/* Entry point called by the runtime after dlopen("libkoala_webview.so").
 * The name must be `<last segment of the package path>_native_lib_init`, so
 * `koala/webview` becomes `webview_native_lib_init`. extern "C" keeps the
 * symbol unmangled for dlsym. */
extern "C" void webview_native_lib_init(NativeLib *lib)
{
    webview_reg_funcs(lib);
    webview_reg_window(lib);
}
/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * koala/webview native engine: implementation.
 *
 * Pure C on top of the system backend's C API. Linux uses GTK 3 +
 * WebKitGTK (webkit2gtk-4.1/4.0); the engine only owns the window shell —
 * HTML/CSS/JS rendering is WebKit's job. Windows (WebView2) and macOS
 * (WKWebView) backends are not implemented yet: creation fails there.
 */

#include "webview.h"

#if defined(WEBVIEW_GTK)

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <webkit2/webkit2.h>

#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif

typedef struct _WvInstance {
    /* NULL once the window has been destroyed (WM close or destroy()). */
    GtkWidget *window;
    GtkWidget *webview;
    int stop_run_loop;
} WvInstance;

/* Work around WebKit bug 261874 (DMA-BUF renderer crash on NVIDIA + X11,
 * WebKit >= 2.42): disable the DMA-BUF renderer when affected. */
static void apply_dmabuf_workaround(void)
{
    struct stat st;
    const char *env;

    if (webkit_get_major_version() != 2 || webkit_get_minor_version() < 42) return;
    env = getenv("WEBKIT_DISABLE_DMABUF_RENDERER");
    if (env && env[0]) return;
    env = getenv("WAYLAND_DISPLAY");
    if (env && env[0]) return;
    env = getenv("XDG_SESSION_TYPE");
    if (env && strcmp(env, "wayland") == 0) return;
    env = getenv("DESKTOP_SESSION");
    if (env && strstr(env, "wayland")) return;
#ifdef GDK_WINDOWING_X11
    if (!GDK_IS_X11_DISPLAY(gdk_display_get_default())) return;
#else
    return;
#endif
    if (stat("/sys/module/nvidia", &st) != 0 || !S_ISDIR(st.st_mode)) return;
    setenv("WEBKIT_DISABLE_DMABUF_RENDERER", "1", 1);
}

static void on_window_destroy(GtkWidget *widget, gpointer arg)
{
    WvInstance *inst = (WvInstance *)arg;
    (void)widget;
    inst->window = NULL;
}

/* We hold a reference on the webview, so GTK must not dispose it as a child
 * of the dying toplevel (that would dispose it twice). Unparent it first. */
static void unparent_webview(WvInstance *inst)
{
    GtkWidget *parent;

    if (!inst->webview) return;
    parent = gtk_widget_get_parent(inst->webview);
    if (parent) {
        gtk_container_remove(GTK_CONTAINER(parent), inst->webview);
    }
}

static gboolean on_window_delete(GtkWidget *widget, GdkEvent *event, gpointer arg)
{
    WvInstance *inst = (WvInstance *)arg;
    (void)widget;
    (void)event;
    unparent_webview(inst);
    return FALSE; /* proceed with the window destruction */
}

webview_t webview_create(int debug, void *window)
{
    WvInstance *inst;
    WebKitSettings *settings;

    /* Foreign-window embedding is not supported by this engine. */
    if (window) return NULL;
    if (!gtk_init_check(NULL, NULL)) return NULL;

    inst = (WvInstance *)calloc(1, sizeof(WvInstance));
    if (!inst) return NULL;

    apply_dmabuf_workaround();

    inst->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect(G_OBJECT(inst->window), "destroy", G_CALLBACK(on_window_destroy), inst);
    g_signal_connect(G_OBJECT(inst->window), "delete-event", G_CALLBACK(on_window_delete), inst);

    inst->webview = webkit_web_view_new();
    g_object_ref_sink(inst->webview);

    settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(inst->webview));
    webkit_settings_set_javascript_can_access_clipboard(settings, TRUE);
    if (debug) {
        webkit_settings_set_enable_write_console_messages_to_stdout(settings, TRUE);
        webkit_settings_set_enable_developer_extras(settings, TRUE);
    }

    gtk_container_add(GTK_CONTAINER(inst->window), inst->webview);
    gtk_widget_show(inst->webview);
    gtk_widget_grab_focus(inst->webview);
    gtk_widget_show(inst->window);
    return inst;
}

static gboolean set_done_flag(gpointer arg)
{
    *(int *)arg = 1;
    return G_SOURCE_REMOVE;
}

webview_error_t webview_destroy(webview_t w)
{
    WvInstance *inst = (WvInstance *)w;
    int done = 0;

    if (!inst) return WEBVIEW_ERROR_INVALID_ARGUMENT;
    if (inst->window) {
        /* Disconnect first so no callback fires during teardown. */
        g_signal_handlers_disconnect_by_data(G_OBJECT(inst->window), inst);
        unparent_webview(inst);
        gtk_window_close(GTK_WINDOW(inst->window));
        /* Drain the event queue so the window closes immediately. */
        g_idle_add_full(G_PRIORITY_HIGH_IDLE, set_done_flag, &done, NULL);
        while (!done) {
            g_main_context_iteration(NULL, TRUE);
        }
        inst->window = NULL;
    }
    if (inst->webview) {
        g_object_unref(inst->webview);
        inst->webview = NULL;
    }
    free(inst);
    return WEBVIEW_ERROR_OK;
}

static gboolean set_stop_flag(gpointer arg)
{
    WvInstance *inst = (WvInstance *)arg;
    inst->stop_run_loop = 1;
    return G_SOURCE_REMOVE;
}

webview_error_t webview_run(webview_t w)
{
    WvInstance *inst = (WvInstance *)w;

    if (!inst) return WEBVIEW_ERROR_INVALID_ARGUMENT;
    inst->stop_run_loop = 0;
    while (!inst->stop_run_loop) {
        g_main_context_iteration(NULL, TRUE);
    }
    return WEBVIEW_ERROR_OK;
}

webview_error_t webview_terminate(webview_t w)
{
    WvInstance *inst = (WvInstance *)w;

    if (!inst) return WEBVIEW_ERROR_INVALID_ARGUMENT;
    g_idle_add_full(G_PRIORITY_HIGH_IDLE, set_stop_flag, inst, NULL);
    return WEBVIEW_ERROR_OK;
}

typedef struct _DispatchCall {
    webview_t w;
    void (*fn)(webview_t, void *);
    void *arg;
} DispatchCall;

static gboolean dispatch_trampoline(gpointer arg)
{
    DispatchCall *call = (DispatchCall *)arg;
    call->fn(call->w, call->arg);
    free(call);
    return G_SOURCE_REMOVE;
}

webview_error_t webview_dispatch(webview_t w, void (*fn)(webview_t, void *), void *arg)
{
    DispatchCall *call;

    if (!w || !fn) return WEBVIEW_ERROR_INVALID_ARGUMENT;
    call = (DispatchCall *)calloc(1, sizeof(DispatchCall));
    if (!call) return WEBVIEW_ERROR_UNSPECIFIED;
    call->w = w;
    call->fn = fn;
    call->arg = arg;
    g_idle_add_full(G_PRIORITY_HIGH_IDLE, dispatch_trampoline, call, NULL);
    return WEBVIEW_ERROR_OK;
}

void *webview_get_window(webview_t w)
{
    WvInstance *inst = (WvInstance *)w;
    return inst ? inst->window : NULL;
}

void *webview_get_native_handle(webview_t w, webview_native_handle_kind_t kind)
{
    WvInstance *inst = (WvInstance *)w;

    if (!inst) return NULL;
    switch (kind) {
        case WEBVIEW_NATIVE_HANDLE_KIND_UI_WINDOW:
            return inst->window;
        case WEBVIEW_NATIVE_HANDLE_KIND_UI_WIDGET:
            return inst->webview;
        case WEBVIEW_NATIVE_HANDLE_KIND_BROWSER_CONTROLLER:
            return inst->webview;
        default:
            return NULL;
    }
}

webview_error_t webview_set_title(webview_t w, const char *title)
{
    WvInstance *inst = (WvInstance *)w;

    if (!inst || !title) return WEBVIEW_ERROR_INVALID_ARGUMENT;
    gtk_window_set_title(GTK_WINDOW(inst->window), title);
    return WEBVIEW_ERROR_OK;
}

webview_error_t webview_set_size(webview_t w, int width, int height, webview_hint_t hints)
{
    WvInstance *inst = (WvInstance *)w;
    GdkGeometry geo;

    if (!inst) return WEBVIEW_ERROR_INVALID_ARGUMENT;
    gtk_window_set_resizable(GTK_WINDOW(inst->window), hints != WEBVIEW_HINT_FIXED);
    switch (hints) {
        case WEBVIEW_HINT_NONE:
            gtk_window_resize(GTK_WINDOW(inst->window), width, height);
            break;
        case WEBVIEW_HINT_FIXED:
        case WEBVIEW_HINT_MIN:
            gtk_widget_set_size_request(inst->window, width, height);
            break;
        case WEBVIEW_HINT_MAX:
            geo.max_width = width;
            geo.max_height = height;
            gtk_window_set_geometry_hints(GTK_WINDOW(inst->window), NULL, &geo, GDK_HINT_MAX_SIZE);
            break;
        default:
            return WEBVIEW_ERROR_INVALID_ARGUMENT;
    }
    return WEBVIEW_ERROR_OK;
}

webview_error_t webview_navigate(webview_t w, const char *url)
{
    WvInstance *inst = (WvInstance *)w;

    if (!inst || !url) return WEBVIEW_ERROR_INVALID_ARGUMENT;
    webkit_web_view_load_uri(WEBKIT_WEB_VIEW(inst->webview), url);
    return WEBVIEW_ERROR_OK;
}

webview_error_t webview_set_html(webview_t w, const char *html)
{
    WvInstance *inst = (WvInstance *)w;

    if (!inst || !html) return WEBVIEW_ERROR_INVALID_ARGUMENT;
    webkit_web_view_load_html(WEBKIT_WEB_VIEW(inst->webview), html, NULL);
    return WEBVIEW_ERROR_OK;
}

webview_error_t webview_init(webview_t w, const char *js)
{
    WvInstance *inst = (WvInstance *)w;
    WebKitUserContentManager *manager;
    WebKitUserScript *script;

    if (!inst || !js) return WEBVIEW_ERROR_INVALID_ARGUMENT;
    manager = webkit_web_view_get_user_content_manager(WEBKIT_WEB_VIEW(inst->webview));
    script = webkit_user_script_new(js, WEBKIT_USER_CONTENT_INJECT_TOP_FRAME,
                                    WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START, NULL, NULL);
    webkit_user_content_manager_add_script(manager, script);
    webkit_user_script_unref(script);
    return WEBVIEW_ERROR_OK;
}

webview_error_t webview_eval(webview_t w, const char *js)
{
    WvInstance *inst = (WvInstance *)w;
    WebKitWebView *wv;

    if (!inst || !js) return WEBVIEW_ERROR_INVALID_ARGUMENT;
    wv = WEBKIT_WEB_VIEW(inst->webview);
    /* URI is NULL before content has begun loading. */
    if (!webkit_web_view_get_uri(wv)) return WEBVIEW_ERROR_OK;
#if (WEBKIT_MAJOR_VERSION == 2 && WEBKIT_MINOR_VERSION >= 40) || WEBKIT_MAJOR_VERSION > 2
    webkit_web_view_evaluate_javascript(wv, js, (gssize)strlen(js), NULL, NULL, NULL, NULL, NULL);
#else
    webkit_web_view_run_javascript(wv, js, NULL, NULL, NULL);
#endif
    return WEBVIEW_ERROR_OK;
}

#else /* !WEBVIEW_GTK */

/* Windows (WebView2) and macOS (WKWebView) backends are not implemented yet.
 * TODO: WebView2 via ICoreWebView2 (C ABI from WebView2Loader) / WKWebView via
 * the Objective-C C bridge; keep this file pure C. */

webview_t webview_create(int debug, void *window)
{
    (void)debug;
    (void)window;
    return NULL;
}

webview_error_t webview_destroy(webview_t w)
{
    (void)w;
    return WEBVIEW_ERROR_MISSING_DEPENDENCY;
}
webview_error_t webview_run(webview_t w)
{
    (void)w;
    return WEBVIEW_ERROR_MISSING_DEPENDENCY;
}
webview_error_t webview_terminate(webview_t w)
{
    (void)w;
    return WEBVIEW_ERROR_MISSING_DEPENDENCY;
}
webview_error_t webview_dispatch(webview_t w, void (*fn)(webview_t, void *), void *arg)
{
    (void)w;
    (void)fn;
    (void)arg;
    return WEBVIEW_ERROR_MISSING_DEPENDENCY;
}
void *webview_get_window(webview_t w)
{
    (void)w;
    return NULL;
}
void *webview_get_native_handle(webview_t w, webview_native_handle_kind_t kind)
{
    (void)w;
    (void)kind;
    return NULL;
}
webview_error_t webview_set_title(webview_t w, const char *title)
{
    (void)w;
    (void)title;
    return WEBVIEW_ERROR_MISSING_DEPENDENCY;
}
webview_error_t webview_set_size(webview_t w, int width, int height, webview_hint_t hints)
{
    (void)w;
    (void)width;
    (void)height;
    (void)hints;
    return WEBVIEW_ERROR_MISSING_DEPENDENCY;
}
webview_error_t webview_navigate(webview_t w, const char *url)
{
    (void)w;
    (void)url;
    return WEBVIEW_ERROR_MISSING_DEPENDENCY;
}
webview_error_t webview_set_html(webview_t w, const char *html)
{
    (void)w;
    (void)html;
    return WEBVIEW_ERROR_MISSING_DEPENDENCY;
}
webview_error_t webview_init(webview_t w, const char *js)
{
    (void)w;
    (void)js;
    return WEBVIEW_ERROR_MISSING_DEPENDENCY;
}
webview_error_t webview_eval(webview_t w, const char *js)
{
    (void)w;
    (void)js;
    return WEBVIEW_ERROR_MISSING_DEPENDENCY;
}

#endif /* WEBVIEW_GTK */

static const webview_version_info_t engine_version = {
    { 0, 1, 0 },
    "0.1.0",
    "",
    "",
};

const webview_version_info_t *webview_version(void) { return &engine_version; }

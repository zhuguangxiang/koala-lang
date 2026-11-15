/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * The koala `std/gc` module: python-like gc interface over runtime/gc.c.
 *
 * Debug output is the module's own, independent of the koala log level and
 * built into every version. It is printed to stderr through GCHooks.
 */

#include "gcmodule.h"
#include <strings.h>
#include "dictobj.h"
#include "gc.h"
#include "tupleobj.h"

#ifdef __cplusplus
extern "C" {
#endif

/* GC_DEBUG_XXX */
static int gc_debug_flags;

/* e.g. 512B, 1.5KB, 2MB, 1GB */
static void gcmod_format_size(char *buf, size_t len, size_t size)
{
    static const char *units[] = { "B", "KB", "MB", "GB", "TB" };
    int i = 0;
    size_t div = 1;
    while (i < 4 && size >= div * 1024) {
        div *= 1024;
        i++;
    }
    if (size % div == 0) {
        snprintf(buf, len, "%zu%s", size / div, units[i]);
    } else {
        snprintf(buf, len, "%.1f%s", (double)size / div, units[i]);
    }
}

static void gcmod_on_free(void *ptr, int kind, size_t size)
{
    char buf[32];
    gcmod_format_size(buf, sizeof(buf), size);
    switch (kind) {
        case GC_KIND_OBJECT: {
            TypeObject *tp = OB_TYPE((Object *)ptr);
            fprintf(stderr, "gc: free <%s %p> size %s\n", tp ? tp->name : "?", ptr, buf);
            break;
        }
        case GC_KIND_VALUES:
            fprintf(stderr, "gc: free <values %p> len %zu\n", ptr, size / sizeof(TValue));
            break;
        default:
            fprintf(stderr, "gc: free <bytes %p> size %s\n", ptr, buf);
            break;
    }
}

static void gcmod_on_collect(const GCStats *stats)
{
    const GCCycleStats *last = &stats->last;
    char freed[32], alive[32], used[32];
    gcmod_format_size(freed, sizeof(freed), last->freed_size);
    gcmod_format_size(alive, sizeof(alive), last->alive_size);
    gcmod_format_size(used, sizeof(used), gc_used());
    fprintf(stderr,
            "gc: collection #%zu: scanned %zu, freed %zu (%s), alive %zu (%s), "
            "old %zu, perm %zu, used %s, %.3f ms\n",
            stats->collections, last->scanned, last->freed, freed, last->alive, alive, last->old,
            gc_perm_count(), used, last->elapsed_ms);
}

void gc_set_debug(int flags)
{
    gc_debug_flags = flags;
    GCHooks hooks = {
        .on_free = (flags & GC_DEBUG_FREE) ? gcmod_on_free : NULL,
        .on_collect = (flags & GC_DEBUG_STATS) ? gcmod_on_collect : NULL,
    };
    gc_set_hooks(&hooks);
}

int gc_get_debug(void) { return gc_debug_flags; }

/* comma separated: stats, free */
static int gcmod_parse_debug(const char *s)
{
    int flags = 0;
    const char *p = s;
    while (*p) {
        size_t n = strcspn(p, ",");
        if (n == 5 && !strncasecmp(p, "stats", n)) {
            flags |= GC_DEBUG_STATS;
        } else if (n == 4 && !strncasecmp(p, "free", n)) {
            flags |= GC_DEBUG_FREE;
        } else if (n != 0) {
            panic("gc: invalid KOALA_GC_DEBUG '%s'", s);
        }
        p += n;
        if (*p == ',') p++;
    }
    return flags;
}

void gc_debug_init(void)
{
    const char *s = getenv("KOALA_GC_DEBUG");
    gc_set_debug(s ? gcmod_parse_debug(s) : 0);
}

/*
pub func collect() int
*/
static TValue gcmod_collect(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    return int64_value(gc_collect(__ks()));
}

/*
pub func get_count() (int, int, int)
*/
static TValue gcmod_get_count(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    TValue items[3] = {
        int64_value(gc_get_count()),
        int64_value(gc_normal_count()),
        int64_value(gc_perm_count()),
    };
    return obj_value(kl_new_tuple(items, 3));
}

static TValue gcmod_dict_set(TValue *dict, char *key, char *val)
{
    TValue k = kl_val_str(key);
    TValue v = kl_val_str(val);
    return kl_call_set_item(dict, &k, &v);
}

/*
pub func get_stats() dict[str, str]
*/
static TValue gcmod_get_stats(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    GCStats stats;
    gc_get_stats(&stats);

    char buf[32];
    TValue dict = obj_value(kl_new_dict());
    snprintf(buf, sizeof(buf), "%zu", stats.collections);
    TValue ret = gcmod_dict_set(&dict, "collections", buf);
    if (is_error(&ret)) return ret;
    snprintf(buf, sizeof(buf), "%zu", stats.collected);
    ret = gcmod_dict_set(&dict, "collected", buf);
    if (is_error(&ret)) return ret;
    gcmod_format_size(buf, sizeof(buf), stats.collected_size);
    ret = gcmod_dict_set(&dict, "collected_size", buf);
    if (is_error(&ret)) return ret;
    return dict;
}

/*
pub func set_debug(flags int)
*/
static TValue gcmod_set_debug(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    gc_set_debug((int)kl_arg_int(0));
    return no_value;
}

/*
pub func get_debug() int
*/
static TValue gcmod_get_debug(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    return int64_value(gc_get_debug());
}

/*
pub func used() str
*/
static TValue gcmod_used(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    char buf[32];
    gcmod_format_size(buf, sizeof(buf), gc_used());
    return kl_val_str(buf);
}

/*
pub func max() str
*/
static TValue gcmod_max(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    char buf[32];
    gcmod_format_size(buf, sizeof(buf), gc_max());
    return kl_val_str(buf);
}

void gc_native_lib_init(NativeLib *lib)
{
    kl_reg_func(lib, "collect", gcmod_collect);
    kl_reg_func(lib, "get_count", gcmod_get_count);
    kl_reg_func(lib, "get_stats", gcmod_get_stats);
    kl_reg_func(lib, "set_debug", gcmod_set_debug);
    kl_reg_func(lib, "get_debug", gcmod_get_debug);
    kl_reg_func(lib, "used", gcmod_used);
    kl_reg_func(lib, "max", gcmod_max);
}

#ifdef __cplusplus
}
#endif

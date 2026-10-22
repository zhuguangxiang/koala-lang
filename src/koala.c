/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "koala.h"
#include <limits.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "args.h"
#include "buffer.h"
#include "mm.h"

// Append each non-empty entry of `list` (a ':'-separated search path) to `out`.
// Empty entries — produced by leading, trailing, or duplicate ':' — are dropped.
// Every emitted entry is guaranteed to end with '/'. Entries are separated by
// a single ':' in the output; if `out` is already non-empty, a leading ':'
// separator is inserted before the first emitted entry.
static void append_normalized_path_list(Buffer *out, const char *list)
{
    if (!list) return;

    size_t ulen = strlen(list);
    size_t i = 0;
    while (i <= ulen) {
        size_t j = i;
        while (j < ulen && list[j] != ':') j++;
        if (j > i) {
            if (out->len > 0) buf_write_char(out, ':');
            buf_write_nstr(out, list + i, (int)(j - i));
            if (list[j - 1] != '/') buf_write_char(out, '/');
        }
        if (j >= ulen) break;
        i = j + 1;
    }
}

static void setup_default_koala_path(void)
{
    // Resolve executable directory: /proc/self/exe -> <prefix>/bin
    BUF(exe);
    buf_reserve(&exe, PATH_MAX);
    ssize_t n = readlink("/proc/self/exe", BUF_STR(exe), PATH_MAX - 1);
    if (n <= 0) {
        FINI_BUF(exe);
        return;
    }
    BUF_STR(exe)[n] = '\0';
    exe.len = (int)n;

    char *slash = strrchr(BUF_STR(exe), '/');
    if (!slash) {
        FINI_BUF(exe);
        return;
    }
    *slash = '\0';
    exe.len = (int)(slash - BUF_STR(exe));

    // Order: user KOALA_PATH > current dir > derived default lib.
    BUF(path);
    append_normalized_path_list(&path, getenv("KOALA_PATH"));

    if (BUF_LEN(path) > 0) buf_write_char(&path, ':');
    buf_write_str(&path, "./");

    buf_write_char(&path, ':');
    buf_write_str(&path, BUF_STR(exe));
    buf_write_str(&path, "/../lib/koala-lib/");

    FINI_BUF(exe);

    setenv("KOALA_PATH", BUF_STR(path), 1);
    FINI_BUF(path);
}

static int run_cmd(char *argv[])
{
    extern char **environ;

    pid_t pid;
    int rc = posix_spawnp(&pid, "koalac", NULL, NULL, argv, environ);
    if (rc != 0) {
        fprintf(stderr, "posix_spawn failed: %d\n", rc);
        return -1;
    }

    // wait sub-process
    int status;
    waitpid(pid, &status, 0);

    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

static char *make_temp_klc(void)
{
    const char *dir = "/tmp/koala";
    mkdir(dir, 0700);

    char tmpl[256];
    snprintf(tmpl, sizeof(tmpl), "/tmp/koala/tmp-XXXXXX");

    int fd = mkstemp(tmpl);
    if (fd == -1) return NULL;
    close(fd);

    size_t len = strlen(tmpl) + 5;
    char *path = malloc(len);
    snprintf(path, len, "%s.klc", tmpl);
    return path;
}

static int compile(const char *input, const char *output, KoalaOptions *opt)
{
    char buf_2[32];
    char *argv[20];
    int n = 0;

    argv[n++] = "koalac";
    argv[n++] = "--cgen";
    argv[n++] = "--fusion";
    argv[n++] = "--tail-call";
    argv[n++] = "--write-klc";

    if (opt->enable_ssa) {
        argv[n++] = "--ssa";
    }

    if (opt->enable_int_trap) {
        argv[n++] = "--int-trap";
    }

    if (opt->enable_float_trap) {
        argv[n++] = "--float-trap";
    }

    if (opt->strip_lineinfo) {
        argv[n++] = "--strip-lineinfo";
    }

    if (opt->dump) {
        snprintf(buf_2, sizeof(buf_2), "--dump=%s", opt->dump);
        argv[n++] = buf_2;
    }

    if (opt->pkg_name[0]) {
        char pkg_arg[1024 + 16];
        snprintf(pkg_arg, sizeof(pkg_arg), "--package-name=%s", opt->pkg_name);
        argv[n++] = pkg_arg;
    }

    argv[n++] = (char *)input;
    argv[n++] = "-o";
    argv[n++] = (char *)output;
    argv[n] = NULL;

    int rc = run_cmd(argv);
    return rc;
}

static int is_directory(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return S_ISDIR(st.st_mode);
}

static char *default_output_path(const char *input)
{
    size_t len = strlen(input);

    if (is_directory(input)) {
        char *out = malloc(len + 5); // ".klc"
        if (!out) return NULL;
        memcpy(out, input, len);
        strcpy(out + len, ".klc");
        return out;
    }

    if (len < 3 || strcmp(input + len - 3, ".kl") != 0) {
        return NULL;
    }

    char *out = malloc(len + 2); // ".kl" → ".klc"
    if (!out) return NULL;

    memcpy(out, input, len - 3);
    strcpy(out + (len - 3), ".klc");
    return out;
}

static char *normalize_input(const char *input)
{
    size_t len = strlen(input);
    if (len > 4 && !strcmp(input + len - 4, ".klc")) {
        return strdup(input);
    }

    char *out = malloc(len + 5);
    if (!out) return NULL;
    memcpy(out, input, len);
    strcpy(out + len, ".klc");
    return out;
}

static int run_klc(const char *input)
{
    koala_initialize();

    int failure;

    KoalaOptions *opt = kl_get_cmd_opt();
    if (opt->test_mode) {
        failure = koala_test_file((char *)input);
    } else {
        failure = koala_run_file((char *)input);
    }

    koala_finalize();

    return failure;
}

static int file_exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

int main(int argc, char *argv[])
{
    // double t0 = now_ms();

    setup_default_koala_path();

    if (kl_parse_args(argc, argv)) return -1;

    KoalaOptions *opt = kl_get_cmd_opt();

    const char *input = opt->input;

    // cache hit: if this is a .klc file, run it directly.
    if (match_suffix(input, ".klc")) {
        if (opt->compile_only) {
            fprintf(stderr, "koala: -c cannot be used with .klc\n");
            return -1;
        }

        // double t1 = now_ms();
        // fprintf(stderr, "[args] %.3f ms\n", t1 - t0);

        return run_klc(input);
    }

    // Cache hit: for a .kl input (or a package directory) with no explicit -c
    // if the sibling .klc already exists, run it directly.
    // No freshness check on purpose: input may be a directory, whose mtime
    // does not reflect source content changes.
    if (!opt->compile_only) {
        char *klc_file = normalize_input(input);
        if (klc_file) {
            if (file_exists(klc_file)) {
                int rc = run_klc(klc_file);
                free(klc_file);
                return rc;
            }
            free(klc_file);
        }
    }

    char *temp = NULL;
    const char *out = opt->output;

    if (!out) {
        if (opt->compile_only) {
            out = default_output_path(input);
            if (!out) {
                fprintf(stderr, "koala: cannot create output path\n");
                return -1;
            }
        } else {
            temp = make_temp_klc();
            if (!temp) {
                fprintf(stderr, "koala: cannot create temp file\n");
                return -1;
            }
            out = temp;
        }
    }

    if (compile(input, out, opt)) {
        if (temp) {
            remove(temp);
            free(temp);
        }
        fprintf(stderr, "koala: compilation failed\n");
        return -1;
    }

    int ret = 0;

    if (!opt->compile_only) {
        ret = run_klc(out);
    }

    if (temp) {
        remove(temp);
        free(temp);
    }

    return ret;
}

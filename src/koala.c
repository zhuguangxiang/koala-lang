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
#include "mm.h"

// Derive the default library search path from the executable location,
// e.g. <prefix>/bin/koala -> <prefix>/lib/koala-lib, and merge it into
// KOALA_PATH. User entries come first so they override/extend defaults.
// The merged value is inherited by the spawned `koalac` via environ.
static void setup_default_koala_path(void)
{
    char exe[PATH_MAX];

    ssize_t n = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
    if (n <= 0) return;
    exe[n] = '\0';

    char *slash = strrchr(exe, '/');
    if (!slash) return;
    *slash = '\0'; // executable directory, e.g. <prefix>/bin

    static const char suffix[] = "/../lib/koala-lib/";
    const char *user = getenv("KOALA_PATH");

    // strip trailing colons so merging never creates an empty entry;
    // the path search loop (str_sep) stops at empty entries.
    size_t ulen = user ? strlen(user) : 0;
    while (ulen > 0 && user[ulen - 1] == ':') ulen--;

    size_t len = strlen(exe) + sizeof(suffix);
    if (ulen) len += ulen + 1;

    char *path = malloc(len);
    if (!path) return;

    if (ulen)
        snprintf(path, len, "%.*s:%s%s", (int)ulen, user, exe, suffix);
    else
        snprintf(path, len, "%s%s", exe, suffix);

    setenv("KOALA_PATH", path, 1);
    free(path);
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

int main(int argc, char *argv[])
{
    // double t0 = now_ms();

    setup_default_koala_path();

    if (kl_parse_args(argc, argv)) return -1;

    KoalaOptions *opt = kl_get_cmd_opt();

    const char *input = opt->input;

    if (match_suffix(input, ".klc")) {
        if (opt->compile_only) {
            fprintf(stderr, "koala: -c cannot be used with .klc\n");
            return -1;
        }

        // double t1 = now_ms();
        // fprintf(stderr, "[args] %.3f ms\n", t1 - t0);

        return run_klc(input);
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

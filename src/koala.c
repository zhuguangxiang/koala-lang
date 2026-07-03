/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "koala.h"
#include <spawn.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include "args.h"

static double now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
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
    char *argv[16];
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

static int has_suffix(const char *s, const char *suffix)
{
    size_t sl = strlen(s);
    size_t su = strlen(suffix);
    if (sl < su) return 0;
    return strcmp(s + sl - su, suffix) == 0;
}

static void run_klc(const char *input)
{
    koala_initialize();
    koala_run_file((char *)input);
    koala_finalize();
}

int main(int argc, char *argv[])
{
    // double t0 = now_ms();

    KoalaOptions opt = { 0 };
    if (kl_parse_args(argc, argv, &opt)) return -1;

    const char *input = opt.input;

    if (has_suffix(input, ".klc")) {
        if (opt.compile_only) {
            fprintf(stderr, "koala: -c cannot be used with .klc\n");
            return -1;
        }

        // double t1 = now_ms();
        // fprintf(stderr, "[args] %.3f ms\n", t1 - t0);

        run_klc(input);
        return 0;
    }

    char *temp = NULL;
    const char *out = opt.output;

    if (!out) {
        if (opt.compile_only) {
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

    if (compile(input, out, &opt)) {
        if (temp) {
            remove(temp);
            free(temp);
        }
        fprintf(stderr, "koala: compilation failed\n");
        return -1;
    }

    if (!opt.compile_only) {
        run_klc(out);
    }

    if (temp) {
        remove(temp);
        free(temp);
    }

    return 0;
}

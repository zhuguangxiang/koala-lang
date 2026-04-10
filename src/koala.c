/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "koala.h"
#include <sys/stat.h>
#include "args.h"

static int run_cmd(const char *cmd)
{
    int rc = system(cmd);
    if (rc == -1) return -1;
    if (WIFEXITED(rc)) return WEXITSTATUS(rc);
    return -1;
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
    char cmd[512] = "koalac";

    // 自动开启所有优化
    strcat(cmd, " --cgen --fusion --tail-call --write-klc --dump=");

    // dump flags
    if (opt->dump_no_opt_ir) strcat(cmd, "ir,");
    if (opt->dump_ir) strcat(cmd, "opt-ir,");
    if (opt->dump_lir) strcat(cmd, "lir,");
    if (opt->dump_vreg) strcat(cmd, "vreg,");
    if (opt->dump_code) strcat(cmd, "cgen,");

    // input + output
    strcat(cmd, " ");
    strcat(cmd, input);
    strcat(cmd, " -o ");
    strcat(cmd, output);

    int rc = run_cmd(cmd);
    if (rc != 0) {
        fprintf(stderr, "koala: koalac failed (%d)\n", rc);
        return rc;
    }
    return 0;
}

static char *default_output_path(const char *input)
{
    size_t len = strlen(input);
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
    size_t ls = strlen(s);
    size_t lf = strlen(suffix);
    return lf <= ls && strcmp(s + ls - lf, suffix) == 0;
}

static void run_klc(const char *input)
{
    koala_initialize();
    koala_run_file((char *)input);
    koala_finalize();
}

int main(int argc, char *argv[])
{
    KoalaOptions opt = { 0 };
    if (kl_parse_args(argc, argv, &opt)) return -1;

    const char *input = opt.input;

    if (has_suffix(input, ".klc")) {
        if (opt.compile_only) {
            fprintf(stderr, "koala: -c cannot be used with .klc\n");
            return -1;
        }
        run_klc(input);
        return 0;
    }

    if (!has_suffix(input, ".kl")) {
        fprintf(stderr, "koala: unknown input type: %s\n", input);
        return -1;
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

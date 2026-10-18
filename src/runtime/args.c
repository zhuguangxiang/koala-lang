/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "args.h"
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <sys/utsname.h>
#include "mm.h"
#include "version.h"

#ifdef __cplusplus
extern "C" {
#endif

static KoalaOptions kl_cmd_opt;

KoalaOptions *kl_get_cmd_opt(void) { return &kl_cmd_opt; }

static void print_usage(const char *prog)
{
    printf(
        "\nUsage: %s [<options>] <package>|<file.kl>|<file.klc>\n"
        "\n"
        "Options:\n"
        "  -c                   Compile only (supports .kl file or directory as module)\n"
        "  -o <file>            Output .klc file\n"
        "  --ssa                Enable SSA construction stage.\n"
        "  --int-trap           Enable trap checking for integer cast.\n"
        "  --float-trap         Enable trap checking for float cast.\n"
        "  --dump=<list>        Dump internal information.\n"
        "                       <list> is a comma-separated list of:\n"
        "                           no-opt-ir - dump no-opt IR\n"
        "                           ssa       - dump SSA IR\n"
        "                           ir        - optimized IR (after opt passes)\n"
        "                           lir       - LIR (after isel/regalloc)\n"
        "                           vreg      - dump virtual register info\n"
        "                           code      - codegen output\n"
        "                           itable    - dump interface table\n"
        "                           all       - dump all stages\n"
        "  --package-name       Specify the package name.\n"
        "  --strip-lineinfo     Strip line information from the output.\n"
        "  --test               Enable test mode.\n"
        "  -v, --version        Show version information\n"
        "  -h, --help           Show this help message\n",
        prog);

    printf(
        "\nEnvironment variables:\n"
        "  KOALA_HOME  Koala installation directory.\n"
        "              Used as the default package search path.\n"
        "  KOALA_PATH  ':' separated list of directories for searching packages.\n"
        "\n");
}

static void version(void)
{
    printf("koala %s (build: %d) (%s, %s)\n", KOALA_VERSION_STRING, KOALA_BUILD_NUMBER, __DATE__,
           __TIME__);

    struct utsname sysinfo;
    if (!uname(&sysinfo)) {
#if defined(__clang__)
        printf("[clang %d.%d.%d on %s/%s]\r\n", __clang_major__, __clang_minor__,
               __clang_patchlevel__, sysinfo.sysname, sysinfo.machine);
#elif defined(__GNUC__)
        printf("[gcc %d.%d.%d on %s/%s]\r\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__,
               sysinfo.sysname, sysinfo.machine);
#elif defined(_MSC_VER)
        printf("[msvc %d on %s/%s]\r\n", _MSC_VER, sysinfo.sysname, sysinfo.machine);
#else
        printf("[unknown compiler on %s/%s]\r\n", sysinfo.sysname, sysinfo.machine);
#endif
    }
}

/**
 * Helper function to run a single getopt_long loop and update KoalaOptions.
 *
 * Returns 0 on success, -1 on error (usage printed, version printed,
 * unknown option, or missing option argument).
 */
static int _parse_args(int argc, char *argv[], const char *optstring,
                       const struct option *long_opts, KoalaOptions *opt)
{
    int c;
    while ((c = getopt_long(argc, argv, optstring, long_opts, NULL)) != -1) {
        switch (c) {
            case 'c':
                opt->compile_only = 1;
                break;
            case 'o':
                opt->output = optarg;
                break;
            case 'h':
                print_usage(argv[0]);
                return -1;
            case 'v':
                version();
                return -1;
            case 1:
                opt->dump = optarg;
                break;
            case 2:
                opt->enable_int_trap = 1;
                break;
            case 3:
                opt->enable_float_trap = 1;
                break;
            case 4:
                strncpy(opt->pkg_name, optarg, MAX_PATH_LEN);
                opt->pkg_name[MAX_PATH_LEN - 1] = 0;
                break;
            case 5:
                opt->enable_ssa = 1;
                break;
            case 6:
                opt->test_mode = 1;
                break;
            case 7:
                opt->strip_lineinfo = 1;
                break;
            default:
                return -1;
        }
    }
    return 0;
}

/**
 * Parse the koala command line.
 *
 * Layout:
 *
 *   koala [VM options] <script.kl> [script args] [++ VM options]
 *
 * - Stage 1 parses VM options that appear before the script path.
 *   The "+" prefix stops getopt_long at the first non-option, so the
 *   script path is preserved as argv[optind].
 *
 * - Stage 2 scans for the "++" delimiter after the script path. The
 *   delimiter separates script arguments from VM options, analogous to
 *   the "test.kl -- --test-threads 3" convention in cargo / go.
 *
 * - Stage 3 copies the script's arguments into opt->argv BEFORE stage 4
 *   runs. Stage 4 uses plain "co:hv" (no "+") which lets GNU getopt
 *   permute argv in place; copying first keeps the script's argv stable.
 *
 * - Stage 4 parses VM options after "++". Unknown options are an error
 *   here; they never fall through to the script.
 *
 * Examples:
 *
 *   koala --dump=code script.kl --lib
 *       dump=code, script args = ["--lib"]
 *
 *   koala script.kl --lib ++ --dump=code
 *       dump=code, script args = ["--lib"]
 *
 *   koala script.kl --dump=code
 *       dump not set, script args = ["--dump=code"]
 *
 *   koala script.kl ++ --bogus
 *       error, process exits
 */
int kl_parse_args(int argc, char *argv[])
{
    KoalaOptions *opt = &kl_cmd_opt;

    static struct option long_opts[] = {
        { "dump", required_argument, 0, 1 },
        { "int-trap", no_argument, 0, 2 },
        { "float-trap", no_argument, 0, 3 },
        { "help", no_argument, 0, 'h' },
        { "version", no_argument, 0, 'v' },
        { "package-name", required_argument, 0, 4 },
        { "ssa", no_argument, 0, 5 },
        { "test", no_argument, 0, 6 },
        { "strip-lineinfo", no_argument, 0, 7 },
        { 0, 0, 0, 0 },
    };

    /* Reset getopt internal state */
    optind = 1;

    /*
     * STAGE 1: Parse VM options before the script path.
     * The "+" prefix enforces POSIX mode, stopping right at the script path.
     */
    int res = _parse_args(argc, argv, "+co:hv", long_opts, opt);
    if (res != 0) return res;

    if (optind >= argc) {
        print_usage(argv[0]);
        return -1;
    }

    /* The first non-option is the script path. */
    int script_idx = optind;
    opt->input = argv[script_idx];

    /*
     * STAGE 2: Locate the "++" delimiter after the script path.
     * Anything before it belongs to the script.
     */
    int delimiter_idx = -1;
    for (int i = script_idx + 1; i < argc; i++) {
        if (strcmp(argv[i], "++") == 0) {
            delimiter_idx = i;
            break;
        }
    }

    /*
     * STAGE 3: Copy the script's arguments.
     *
     * Do this before STAGE 4, because STAGE 4 uses plain "co:hv" which
     * permutes argv in place; reading argv after STAGE 4 would see a
     * reordered array. Copying first keeps the script's argv stable.
     */
    int script_end = (delimiter_idx != -1) ? delimiter_idx : argc;
    int script_argc = script_end - script_idx;
    char **script_argv = mm_alloc(script_argc * sizeof(char *));
    ASSERT(script_argv);

    for (int i = 0; i < script_argc; i++) {
        script_argv[i] = argv[script_idx + i];
    }

    /*
     * STAGE 4: Parse VM options after the "++" delimiter.
     * Unknown options are an error here; they do not reach the script.
     */
    if (delimiter_idx != -1) {
        optind = delimiter_idx + 1;
        res = _parse_args(argc, argv, "co:hv", long_opts, opt);
        if (res != 0) {
            mm_free(script_argv);
            return res;
        }

        /* Reject leftover positional arguments after "++". */
        if (optind < argc) {
            fprintf(stderr, "%s: unexpected argument '%s' after '++'\n", argv[0], argv[optind]);
            mm_free(script_argv);
            return -1;
        }
    }

    /*
     * Hand the copied script args to the runtime.
     *
     * script_argv[0] is the script path itself; sys.args() in the
     * runtime is expected to strip it, matching the previous behavior
     * where opt->argv pointed directly at &argv[script_idx].
     */
    opt->argc = script_argc;
    opt->argv = script_argv;

    return 0;
}

/**
 * Free the memory allocated for the script arguments in the global
 * KoalaOptions structure.
 */
void kl_free_args(void)
{
    KoalaOptions *opt = &kl_cmd_opt;

    if (opt->argv) {
        mm_free(opt->argv);
        opt->argv = NULL;
        opt->argc = 0;
    }
}

#ifdef __cplusplus
}
#endif

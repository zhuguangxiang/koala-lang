/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "args.h"
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <sys/utsname.h>
#include "version.h"

#ifdef __cplusplus
extern "C" {
#endif

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

int kl_parse_args(int argc, char *argv[], KoalaOptions *opt)
{
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

    optind = 1;

    int c;
    while ((c = getopt_long(argc, argv, "co:hv", long_opts, NULL)) != -1) {
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

    if (optind < argc) {
        opt->argc = argc - optind;
        opt->argv = &argv[optind];
    } else {
        opt->argc = 0;
        opt->argv = NULL;
    }

    if (opt->argc > 0) {
        opt->input = opt->argv[0];
    }

    if (!opt->input) {
        print_usage(argv[0]);
        return -1;
    }

    return 0;
}

#ifdef __cplusplus
}
#endif

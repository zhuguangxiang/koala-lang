/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "args.h"
#include <getopt.h>
#include <sys/utsname.h>
#include "version.h"

#ifdef __cplusplus
extern "C" {
#endif

static void print_usage(const char *prog)
{
    printf(
        "Usage: %s [options] <file.kl|file.klc>\n"
        "\n"
        "Options:\n"
        "  -c                   Compile only (do not run)\n"
        "  -o <file>            Output .klc file\n"
        "  --dump-no-opt-ir     Dump IR before optimization\n"
        "  --dump-ir            Dump optimized IR\n"
        "  --dump-lir           Dump LIR\n"
        "  --dump-vreg          Dump virtual register allocation\n"
        "  --dump-code          Dump bytecode\n"
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
    printf("koalac %s (%s, %s)\n", KOALA_VERSION_STRING, __DATE__, __TIME__);

    struct utsname sysinfo;
    if (!uname(&sysinfo)) {
#if defined(__clang__)
        printf("[clang %d.%d.%d on %s/%s]\r\n", __clang_major__, __clang_minor__,
               __clang_patchlevel__, sysinfo.sysname, sysinfo.machine);
#elif defined(__GNUC__)
        printf("[gcc %d.%d.%d on %s/%s]\r\n", __GNUC__, __GNUC_MINOR__,
               __GNUC_PATCHLEVEL__, sysinfo.sysname, sysinfo.machine);
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
        { "dump-no-opt-ir", no_argument, 0, 1000 }, { "dump-ir", no_argument, 0, 1001 },
        { "dump-lir", no_argument, 0, 1002 },       { "dump-vreg", no_argument, 0, 1003 },
        { "dump-code", no_argument, 0, 1004 },      { "help", no_argument, 0, 'h' },
        { "version", no_argument, 0, 'v' },         { 0, 0, 0, 0 },
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
            case 1000:
                opt->dump_no_opt_ir = 1;
                break;
            case 1001:
                opt->dump_ir = 1;
                break;
            case 1002:
                opt->dump_lir = 1;
                break;
            case 1003:
                opt->dump_vreg = 1;
                break;
            case 1004:
                opt->dump_code = 1;
                break;
            default:
                return -1;
        }
    }

    if (optind < argc) opt->input = argv[optind];

    if (!opt->input) {
        fprintf(stderr, "%s: no input file\n", argv[0]);
        print_usage(argv[0]);
        return -1;
    }

    return 0;
}

#ifdef __cplusplus
}
#endif

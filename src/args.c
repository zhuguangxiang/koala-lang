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
        "\nUsage: %s [<options>] <package>|<file.kl>|<file.klc>\n"
        "\n"
        "Options:\n"
        "  -c               Compile only (supports .kl file or directory as module)\n"
        "  -o <file>        Output .klc file\n"
        "  --dump=<list>    Dump internal information.\n"
        "                   <list> is a comma-separated list of:\n"
        "                       no-opt-ir - dump no-opt IR\n"
        "                       ir        - optimized IR (after opt passes)\n"
        "                       lir       - LIR (after isel/regalloc)\n"
        "                       vreg      - dump virtual register info\n"
        "                       code      - codegen output\n"
        "                       all       - dump all stages\n"
        "  -v, --version    Show version information\n"
        "  -h, --help       Show this help message\n",
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
    printf("koala %s (%s, %s)\n", KOALA_VERSION_STRING, __DATE__, __TIME__);

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
        { "dump", required_argument, 0, 1000 },
        { "help", no_argument, 0, 'h' },
        { "version", no_argument, 0, 'v' },
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
            case 1000:
                opt->dump = optarg;
                break;
            default:
                return -1;
        }
    }

    if (optind < argc) opt->input = argv[optind];

    if (!opt->input) {
        print_usage(argv[0]);
        return -1;
    }

    return 0;
}

#ifdef __cplusplus
}
#endif

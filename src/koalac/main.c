/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2018-2022 James <zhuguangxiang@gmail.com>
 */

#include <getopt.h>
#include <sys/utsname.h>
/* clang-format off */
#include "parser.h"
#include "koala_yacc.h"
#include "koala_lex.h"
/* clang-format on */
#include "version.h"

static void version(void)
{
    printf("Koala(🐻) %s (%s, %s)\n", KOALA_VERSION, __DATE__, __TIME__);

    struct utsname sysinfo;
    if (!uname(&sysinfo)) {
#if defined(__clang__)
        printf("[clang %d.%d.%d on %s/%s]\r\n", __clang_major__,
               __clang_minor__, __clang_patchlevel__, sysinfo.sysname,
               sysinfo.machine);
#elif defined(__GNUC__)
        printf("[gcc %d.%d.%d on %s/%s]\r\n", __GNUC__, __GNUC_MINOR__,
               __GNUC_PATCHLEVEL__, sysinfo.sysname, sysinfo.machine);
#elif defined(_MSC_VER)
#else
#endif
    }
}

static void usage(void)
{
    printf(
        "Usage: koalac [options] <packages>|<files>...\n\n"
        "Options:\n"
        "  -l <library>     Search the library named <library> when linking.\n"
        "  -L <path>        Add a directory to package search path.\n"
        "  -o <file>        Place the output into <file>.\n"
        "  -g, --debug      Generate debug information(DWARF-2).\n"
        "  --pic            Generate position independent code.\n"
        "  --filetype=<llvm|asm|obj|shared|exe>\n"
        "                   Specify output file type.\n"
        "  --verion         Print Koala version.\n"
        "  --help           Print this message.\n"
        "\n");
}

#define MAX_INPUT_NUM 8

typedef struct _ParserOption {
    int debug;
    int pic;
    char *output;
    char *input[MAX_INPUT_NUM];
} ParserOption;

static void parse_command(int argc, char *argv[], ParserOption *parse_opt)
{
    struct option options[] = { { "pic", no_argument, NULL, 0 },
                                { "debug", no_argument, NULL, 'g' },
                                { "filetype", required_argument, NULL, 0 },
                                { "version", no_argument, NULL, 0 },
                                { "help", no_argument, NULL, 0 },
                                { NULL, 0, NULL, 0 } };
    int input = 0;
    int index;
    int opt;

    while ((opt = getopt_long(argc, argv, "-:l:L:o:g", options, &index)) !=
           -1) {
        switch (opt) {
            case 0: {
                /* long-option argument */
                const char *name = options[index].name;
                if (!strcmp(name, "filetype")) {
                    if (optarg) {
                        printf("filetype: %s\n", optarg);
                    }
                } else if (!strcmp(name, "pic")) {
                    parse_opt->pic = 1;
                    printf("pic: enabled\n");
                } else if (!strcmp(name, "version")) {
                    version();
                    exit(0);
                } else {
                    usage();
                    exit(0);
                }
                break;
            }
            case 1:
                /* regular argument */
                parse_opt->input[input++] = optarg;
                printf("input: %s\n", optarg);
                break;
            case '?':
                usage();
                exit(0);
                break;
            case ':':
                if (optopt == 'l') {
                    printf("missing library name\n");
                } else if (optopt == 'L') {
                    printf("missing library path\n");
                } else if (optopt == 'o') {
                    printf("missing output file name\n");
                } else {
                    /* empty */
                }
                usage();
                exit(0);
                break;
            case 'l':
                printf("-l %s\n", optarg);
                break;
            case 'L':
                printf("-L %s\n", optarg);
                break;
            case 'o':
                parse_opt->output = optarg;
                printf("output: %s\n", optarg);
                break;
            case 'g':
                parse_opt->debug = 1;
                printf("debug: enabled\n");
                break;
            default:
                break;
        }
    }
}

static void build_ast(char *path)
{
    FILE *in = fopen(path, "r");
    if (in == NULL) {
        printf("%s: No such file or directory.\n", path);
        return;
    }

    ParserState ps = { 0 };
    ps.row = 1;
    ps.col = 1;

    yyscan_t scanner;
    yylex_init_extra(&ps, &scanner);
    yyset_in(in, scanner);
    yyparse(&ps, scanner);
    yylex_destroy(scanner);

    fclose(in);
}

int main(int argc, char *argv[])
{
    ParserOption opt = { 0 };
    parse_command(argc, argv, &opt);
    if (opt.input[0]) {
        init_atom();
        build_ast(opt.input[0]);
    } else {
        printf("require packages or files\n");
    }
    return 0;
}

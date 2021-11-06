/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include <getopt.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser/parser.h"
#include "util/color.h"
#include "version.h"

#ifdef __cplusplus
extern "C" {
#endif

static void version(void)
{
    printf("Koala %s\n", KOALA_VERSION);
}

static void usage(void)
{
    printf(
        "Usage: koala [<options>] [<package>|<file>]\n"
        "Options:\n"
        "  -c            Compile the package or file.\n"
        "  -v, --verion  Print Koala version.\n"
        "  -h, --help    Print this message.\n"
        "\n");
    printf(
        "Environment variables:\n"
        "KOALA_HOME - koala installed directory.\n"
        "             The default package's search path.\n"
        "KOALA_PATH - ':' seperated list of directories.\n"
        "             The (third-part) package's search path (sys.path).\n"
        "\n");
}

#define MAX_PATH_LEN 1024

static int flag;
static char module[MAX_PATH_LEN];

static void save_path(char *arg)
{
    char *path = arg;
    char *slash = path + strlen(path);
    /* remove trailing slashes */
    while (*--slash == '/')
        ;
    int len = slash - path + 1;

    if (len >= MAX_PATH_LEN) {
        printf(ERR_COLOR "Too long module's path.");
        usage();
        exit(0);
    }

    memcpy(module, path, len);
}

static void parse_command(int argc, char *argv[])
{
    extern char *optarg;
    extern int optind;
    struct option options[] = { { "version", no_argument, NULL, 'v' },
                                { "help", no_argument, NULL, 'h' },
                                { NULL, 0, NULL, 0 } };
    int opt;

    while ((opt = getopt_long(argc, argv, "c:vh?", options, NULL)) != -1) {
        switch (opt) {
            case 'c': {
                flag = 1;
                save_path(optarg);
                break;
            }
            case 'v':
                version();
                exit(0);
                break;
            case 'h':
                /* fall-through */
            case '?':
                usage();
                exit(0);
                break;
            default:
                printf(ERR_COLOR "Invalid option '%c'.", opt);
                usage();
                exit(0);
                break;
        }
    }

    if (optind < argc) {
        if (!strcmp(argv[optind], "?")) {
            usage();
            exit(0);
        }
        if (flag) {
            printf(ERR_COLOR "Only one path is allowed.");
            usage();
            exit(0);
        }
        flag = 2;
        save_path(argv[optind++]);
    }

    if (optind < argc) {
        printf(ERR_COLOR "Only one path is allowed.");
        usage();
        exit(0);
    }
}

void koala_compile(char *path);
void koala_run(char *path);
void koala_cmdline(void);

int main(int argc, char *argv[])
{
    parse_command(argc, argv);

    init_atom();
    init_desc();
    init_parser();

    if (flag == 1) {
        koala_compile(module);
    } else if (flag == 2) {
        koala_run(module);
    } else {
        koala_cmdline();
    }

    fini_parser();
    fini_desc();
    fini_atom();

    return 0;
}

#ifdef __cplusplus
}
#endif

/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <dirent.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include "atom.h"
#include "log.h"
#include "parser.h"
#include "version.h"

#define MAX_PATH_LEN 1024

static char output[MAX_PATH_LEN];
static char input[MAX_PATH_LEN];

static void usage(void)
{
    printf(
        "\nusage: koalac [<options>] <package>|<file>...\n\n"
        "options:\n"
        "  -o <file>      Place the output into <file>.\n"
        "  -v, --version  Print koalac version.\n"
        "  -h, --help     Print this message.\n"
        "\n");
    printf(
        "Environment variables:\n"
        "KOALA_HOME - koala installed directory.\n"
        "             The default packages search path.\n"
        "KOALA_PATH - ':' separated list of directories for searching packages.\n"
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
#else
#endif
    }
}

static char *save_path(const char *path, char *dst)
{
    const char *slash = path + strlen(path);
    /* remove trailing slashes */
    while (*--slash == '/');
    int len = slash - path + 1;

    if (len >= MAX_PATH_LEN) {
        printf("too long path.");
        usage();
        exit(0);
    }

    memcpy(dst, path, len);
}

static void parse_command(int argc, char *argv[])
{
    extern char *optarg;
    extern int optind;
    struct option options[] = { { "version", no_argument, NULL, 'v' },
                                { "help", no_argument, NULL, 'h' },
                                { NULL, 0, NULL, 0 } };
    int opt;

    while ((opt = getopt_long(argc, argv, "o:vh?", options, NULL)) != -1) {
        switch (opt) {
            case 'o': {
                save_path(optarg, output);
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
                printf("invalid option '%c'.\n", opt);
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
        save_path(argv[optind++], input);
    }

    if (argc <= 1) {
        usage();
        exit(0);
    }

    if (optind < argc) {
        printf("too many <package>|<file>.\n");
        usage();
        exit(0);
    }
}

int isdotkl(char *filename)
{
    char *dot = strrchr(filename, '.');
    if (dot == NULL || strlen(dot) != 3) return 0;
    if (dot[1] == 'k' && dot[2] == 'l') return 1;
    return 0;
}

int isdotklc(char *filename)
{
    char *dot = strrchr(filename, '.');
    if (dot == NULL || strlen(dot) != 4) return 0;
    if (dot[1] == 'k' && dot[2] == 'l' && dot[3] == 'c') return 1;
    return 0;
}

int check_dotkl(char *path)
{
    struct stat sb;

    if (lstat(path, &sb)) {
        printf("%s: %s", path, strerror(errno));
        return -1;
    }

    if (!S_ISREG(sb.st_mode)) {
        printf("%s: Not a regular file", path);
        return -1;
    }

    char *dir = str_ndup(path, strlen(path) - 3);
    if (!lstat(dir, &sb)) {
        printf("%s: The same name file or directory exist", dir);
        mm_free(dir);
        return -1;
    }

    mm_free(dir);
    return 0;
}

int check_dir(char *path)
{
    struct stat sb;

    if (lstat(path, &sb)) {
        printf("%s: %s", path, strerror(errno));
        return -1;
    }

    if (!S_ISDIR(sb.st_mode)) {
        printf("%s: Not a package directory", path);
        return -1;
    }

    char *end = path + strlen(path) - 1;
    while (*end == '/') --end;
    char *dotkl = str_ndup_ex(path, end - path + 1, ".kl");
    if (!lstat(dotkl, &sb)) {
        printf("%s: The same name file or directory exist", dotkl);
        mm_free(dotkl);
        return -1;
    }

    mm_free(dotkl);
    return 0;
}

static int _path_cmp(const void *a, const void *b)
{
    char *pa = *(char **)a;
    char *pb = *(char **)b;
    return strcmp(pa, pb);
}

static void build_dir(char *path, Vector *pss)
{
    DIR *dir = opendir(path);
    if (dir == NULL) {
        fprintf(stderr, "%s: No such file or directory\n", path);
        return;
    }

    char *end = path + strlen(path) - 1;
    while (*end == '/') --end;
    char *prefix = str_ndup(path, end - path + 1);
    int prefixlen = strlen(prefix);
    char fullpath[MAX_PATH_LEN] = { 0 };
    char *filename;

    if (prefixlen >= sizeof(fullpath)) {
        fprintf(stderr, "path '%.64s' is too long\n", path);
        return;
    }

    Vector filenames = VECTOR_INIT_PTR;
    struct stat sb;
    struct dirent *dent;
    while ((dent = readdir(dir))) {
        if (!isdotkl(dent->d_name)) continue;

        filename = dent->d_name;
        if (prefixlen + strlen(filename) >= sizeof(fullpath)) {
            fprintf(stderr, "path '%.64s' is too long\n", path);
            continue;
        }

        filename = str_dup(filename);
        vector_push_back(&filenames, &filename);
    }
    closedir(dir);

    vector_sort(&filenames, _path_cmp);

    vector_foreach(filename, &filenames) {
        snprintf(fullpath, sizeof(fullpath) - 1, "%s/%s", prefix, filename);
        if (lstat(fullpath, &sb) || !S_ISREG(sb.st_mode)) continue;
        printf("compiling %s\n", fullpath);
        ParserState *ps = new_parser_state(fullpath);
        vector_push_back(pss, &ps);
    }
}

static void compile(char *src, char *dst)
{
    Vector pss = VECTOR_INIT_PTR;

    if (isdotkl(src)) {
        // single source file
        if (check_dotkl(src)) return;
        if (strlen(dst) == 0) {
            char *dot = strrchr(src, '.');
            if (dot) {
                snprintf(dst, MAX_PATH_LEN - 1, "%.*s.klc", (int)(dot - src), src);
            } else {
                snprintf(dst, MAX_PATH_LEN - 1, "%s.klc", src);
            }
        }
        ParserState *ps = new_parser_state(src);
        vector_push_back(&pss, &ps);
    } else {
        // package directory
        if (check_dir(src)) return;
        if (strlen(dst) == 0) {
            snprintf(dst, MAX_PATH_LEN - 1, "%s.klc", src);
        }
        build_dir(src, &pss);
    }

    do_compile(&pss, dst);

    ParserState *ps;
    vector_foreach(ps, &pss) {
        if (!ps) continue;
        free_parser_state(ps);
    }
    vector_fini(&pss);
}

int main(int argc, char *argv[])
{
    parse_command(argc, argv);
    init_atom();
    init_log(LOG_TRACE, NULL, 0);
    typespec_init();
    init_parser();
    compile(input, output);
    fini_parser();
    typespec_fini();
    fini_log();
    fini_atom();
    // mm_stat();
    return 0;
}

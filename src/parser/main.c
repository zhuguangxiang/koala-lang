/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <dirent.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include "atom.h"
#include "cmd.h"
#include "log.h"
#include "parser.h"
#include "version.h"

#define MAX_PATH_LEN 1024

static char output[MAX_PATH_LEN];
static char input[MAX_PATH_LEN];

CompileOptions opt;

static void usage(void)
{
    printf(
        "\nusage: koalac [<options>] <package>|<file>...\n\n"
        "options:\n"
        "  -o <file>          Place the output into <file>.\n"
        "  --opt              Enable optimization passes (default).\n"
        "  --isel             Enable instruction selection stage.\n"
        "  --cgen             Enable code generation stage.\n"
        "  --regalloc=<kind>  Select register allocator: simple | lsra.\n"
        "  --dump=<list>      Dump internal information.\n"
        "                     <list> is a comma-separated list of:\n"
        "                         ir       - dump no-opt IR\n"
        "                         opt-ir   - optimized IR (after opt passes)\n"
        "                         lir      - LIR (after isel/regalloc)\n"
        "                         vreg     - dump virtual register info\n"
        "                         cgen     - codegen output\n"
        "                         all      - dump all stages\n"
        "  -v, --version      Print koalac version.\n"
        "  -h, --help         Print this message.\n"
        "\n");

    printf(
        "Environment variables:\n"
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
#else
#endif
    }
}

static void save_path(const char *path, char *dst)
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

static DumpFlags parse_dump_flags(const char *s)
{
    DumpFlags flags = DUMP_NONE;

    char buf[128];
    strncpy(buf, s, sizeof(buf));
    buf[sizeof(buf) - 1] = 0;

    char *tok = strtok(buf, ",");
    while (tok) {
        if (strcmp(tok, "ir") == 0)
            flags |= DUMP_IR;
        else if (strcmp(tok, "opt-ir") == 0)
            flags |= DUMP_OPT_IR;
        else if (strcmp(tok, "lir") == 0)
            flags |= DUMP_LIR;
        else if (strcmp(tok, "vreg") == 0)
            flags |= DUMP_VREG;
        else if (strcmp(tok, "cgen") == 0)
            flags |= DUMP_CGEN;
        else if (strcmp(tok, "all") == 0)
            flags |= DUMP_ALL;

        tok = strtok(NULL, ",");
    }

    return flags;
}

static void parse_command(int argc, char *argv[])
{
    extern char *optarg;
    extern int optind;
    struct option options[] = {
        { "version", no_argument, NULL, 'v' }, { "help", no_argument, NULL, 'h' },
        { "opt", no_argument, 0, 1 },          { "isel", no_argument, 0, 2 },
        { "cgen", no_argument, 0, 3 },         { "regalloc", required_argument, 0, 4 },
        { "dump", required_argument, 0, 5 },   { NULL, 0, NULL, 0 },
    };

    int opt_id;
    int long_index;

    while ((opt_id = getopt_long(argc, argv, "o:vh?", options, &long_index)) != -1) {
        switch (opt_id) {
            case 1:
                opt.enable_opt = 1;
                break;

            case 2:
                opt.enable_isel = 1;
                break;

            case 3:
                opt.enable_cgen = 1;
                break;

            case 4:
                if (strcmp(optarg, "none") == 0)
                    opt.regalloc = 0;
                else if (strcmp(optarg, "simple") == 0)
                    opt.regalloc = 1;
                else if (strcmp(optarg, "lsra") == 0)
                    opt.regalloc = 2;
                else {
                    fprintf(stderr, "Unknown regalloc mode: %s\n", optarg);
                    exit(1);
                }
                break;

            case 5:
                opt.dump = parse_dump_flags(optarg);
                break;

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
                printf("Unknown option\n");
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
    size_t prefixlen = strlen(prefix);
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
        ParserState *ps = new_parser_state(fullpath);
        vector_push_back(pss, &ps);
    }

    mm_free(prefix);

    vector_foreach(filename, &filenames) {
        if (!filename) continue;
        mm_free(filename);
    }
    vector_fini(&filenames);
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

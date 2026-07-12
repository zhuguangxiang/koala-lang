/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <dirent.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include "atom.h"
#include "cgen.h"
#include "cmd.h"
#include "isel.h"
#include "klc.h"
#include "log.h"
#include "lsra.h"
#include "opt.h"
#include "parser.h"
#include "version.h"

static char output[MAX_PATH_LEN + 8];
static char input[MAX_PATH_LEN];

CompileOptions cmd_opt;

static void usage(void)
{
    printf(
        "\nUsage: koalac [<options>] <package>|<file.kl>\n\n"
        "options:\n"
        "  -o <file>        Place the output into <file>.\n"
        "  --irgen          Enable IR generation stage.\n"
        "  --ssa            Enable SSA construction stage.\n"
        "  --opt            Enable optimization passes (default).\n"
        "  --isel           Enable instruction selection stage.\n"
        "  --lsra           Enable linear scan register allocator.\n"
        "  --cgen           Enable code generation stage.\n"
        "  --fusion         Enable fusion optimization passes.\n"
        "  --tail-call      Enable tail call optimization.\n"
        "  --build-stdlib   Build the Koala standard library.\n"
        "  --write-klc      Write the compiled output to a .klc file.\n"
        "  --int-trap       Enable trap checking for integer cast.\n"
        "  --float-trap     Enable trap checking for float cast.\n"
        "  --dump=<list>    Dump internal information.\n"
        "                   <list> is a comma-separated list of:\n"
        "                       no-opt-ir - dump no-opt IR\n"
        "                       ssa       - dump SSA IR\n"
        "                       ir        - optimized IR (after opt passes)\n"
        "                       lir       - LIR (after isel/regalloc)\n"
        "                       vreg      - dump virtual register info\n"
        "                       code      - codegen output\n"
        "                       itable    - dump interface table\n"
        "                       all       - dump all stages\n"
        "  --package-name   Specify the package name.\n"
        "  -v, --version    Print koalac version.\n"
        "  -h, --help       Print this message.\n"
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
    printf("koalac %s (build: %d) (%s, %s)\n", KOALA_VERSION_STRING, KOALA_BUILD_NUMBER, __DATE__,
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
        if (str_equal(tok, "no-opt-ir"))
            flags |= DUMP_NO_OPT_IR;
        else if (str_equal(tok, "ssa"))
            flags |= DUMP_SSA_IR;
        else if (str_equal(tok, "ir"))
            flags |= DUMP_IR;
        else if (str_equal(tok, "lir"))
            flags |= DUMP_LIR;
        else if (str_equal(tok, "vreg"))
            flags |= DUMP_VREG;
        else if (str_equal(tok, "code"))
            flags |= DUMP_CODE;
        else if (str_equal(tok, "itable"))
            flags |= DUMP_ITABLE;
        else if (str_equal(tok, "all"))
            flags |= DUMP_ALL;

        tok = strtok(NULL, ",");
    }

    return flags;
}

static void save_pkg_path(CompileOptions *opt, const char *path)
{
    if (strlen(path) >= MAX_PATH_LEN) {
        printf("too long path.");
        usage();
        exit(0);
    }
    strncpy(opt->pkg_path, path, MAX_PATH_LEN);
    opt->pkg_path[MAX_PATH_LEN - 1] = 0;
}

static void parse_command(int argc, char *argv[])
{
    extern char *optarg;
    extern int optind;
    struct option options[] = {
        { "version", no_argument, NULL, 'v' },
        { "help", no_argument, NULL, 'h' },
        { "irgen", no_argument, 0, 1 },
        { "opt", no_argument, 0, 2 },
        { "isel", no_argument, 0, 3 },
        { "lsra", no_argument, 0, 4 },
        { "cgen", no_argument, 0, 5 },
        { "fusion", no_argument, 0, 6 },
        { "tail-call", no_argument, 0, 7 },
        { "build-stdlib", no_argument, 0, 8 },
        { "write-klc", no_argument, 0, 9 },
        { "dump", required_argument, 0, 10 },
        { "int-trap", no_argument, 0, 11 },
        { "float-trap", no_argument, 0, 12 },
        { "package-name", required_argument, 0, 13 },
        { "ssa", no_argument, 0, 14 },
        { NULL, 0, NULL, 0 },
    };

    optind = 1;

    int opt_id;
    int long_index;

    while ((opt_id = getopt_long(argc, argv, "o:vh", options, &long_index)) != -1) {
        switch (opt_id) {
            case 1:
                cmd_opt.enable_genir = 1;
                break;

            case 2:
                cmd_opt.enable_genir = 1;
                cmd_opt.enable_opt = 1;
                break;

            case 3:
                cmd_opt.enable_genir = 1;
                if (cmd_opt.enable_ssa == 0) {
                    cmd_opt.enable_opt = 1;
                }
                cmd_opt.enable_isel = 1;
                break;

            case 4:
                cmd_opt.enable_genir = 1;
                if (cmd_opt.enable_ssa == 0) {
                    cmd_opt.enable_opt = 1;
                }
                cmd_opt.enable_isel = 1;
                cmd_opt.enable_lsra = 1;
                break;

            case 5:
                cmd_opt.enable_genir = 1;
                if (cmd_opt.enable_ssa == 0) {
                    cmd_opt.enable_opt = 1;
                }
                cmd_opt.enable_isel = 1;
                cmd_opt.enable_lsra = 1;
                cmd_opt.enable_cgen = 1;
                break;

            case 6:
                cmd_opt.enable_fusion = 1;
                break;

            case 7:
                cmd_opt.enable_tail_call = 1;
                break;

            case 8:
                cmd_opt.build_stdlib = 1;
                break;

            case 9:
                cmd_opt.enable_write_klc = 1;
                break;

            case 10:
                cmd_opt.dump = parse_dump_flags(optarg);
                break;

            case 11:
                cmd_opt.enable_int_trap = 1;
                break;

            case 12:
                cmd_opt.enable_float_trap = 1;
                break;

            case 13:
                save_pkg_path(&cmd_opt, optarg);
                break;

            case 14:
                cmd_opt.enable_genir = 1;
                cmd_opt.enable_ssa = 1;
                cmd_opt.enable_opt = 0;
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

static void build_dir(char *path, ParserModule *pm)
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
        new_parser_state(pm, fullpath);
    }

    mm_free(prefix);

    vector_foreach(filename, &filenames) {
        if (!filename) continue;
        mm_free(filename);
    }
    vector_fini(&filenames);
}

static void compile(ParserModule *pm)
{
    if (isdotkl(input)) {
        // single source file
        if (check_dotkl(input)) return;

        char *dot = strrchr(input, '.');

        if (strlen(output) == 0) {
            if (dot) {
                snprintf(output, MAX_PATH_LEN + 7, "%.*s.klc", (int)(dot - input), input);
            } else {
                snprintf(output, MAX_PATH_LEN + 7, "%s.klc", input);
            }
        }

        if (pm->pkg_path == NULL) {
            char *slash = strrchr(input, '/');
            if (slash) {
                pm->pkg_path = str_ndup(slash + 1, dot - slash - 1);
            } else {
                pm->pkg_path = str_ndup(input, dot - input);
            }
        }
        new_parser_state(pm, input);
    } else {
        // package directory
        if (check_dir(input)) return;

        if (strlen(output) == 0) {
            snprintf(output, MAX_PATH_LEN + 7, "%s.klc", input);
        }

        if (pm->pkg_path == NULL) {
            char *slash = strrchr(input, '/');
            if (slash) {
                pm->pkg_path = str_dup(slash + 1);
            } else {
                pm->pkg_path = str_dup(input);
            }
        }
        build_dir(input, pm);
    }

    int errors = 0;

    ParserState *ps;
    vector_foreach(ps, &pm->pss) {
        if (!ps) continue;
        kl_parse_ast(ps);
        errors += ps->errors;
    }

    if (errors > 0) {
        errno = -1;
        return;
    }

    if (!is_build_stdlib()) {
        build_intf_table(pm->stbl);
    }

    if (genir_enabled()) {
        kl_gen_ir(pm);
    }

    KlrModule *m = pm->m;

    if (opt_enabled()) {
        kl_optimize(m);
    }

    if (ssa_enabled()) {
        kl_ssa_opt(m);
    }

    if (isel_enabled()) {
        kl_do_isel(m);
    }

    if (lsra_enabled()) {
        kl_do_lsra(m);
    }

    if (cgen_enabled()) {
        kl_do_codegen(m);
    }

    if (write_klc_enabled()) {
        write_to_klc(pm);
    }
}

int main(int argc, char *argv[])
{
    parse_command(argc, argv);
    init_atom();
    init_log(LOG_TRACE, NULL, 0);
    typespec_init();

    ParserModule module = { 0 };
    module.path = output;
    if (cmd_opt.pkg_path[0]) {
        module.pkg_path = cmd_opt.pkg_path;
    }
    vector_init_ptr(&module.links);

    init_parser(&module);
    compile(&module);
    fini_parser(&module);

    typespec_fini();
    fini_log();
    fini_atom();
    return 0;
}

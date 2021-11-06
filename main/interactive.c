/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include <sys/utsname.h>
#include "parser/parser.h"
#include "util/readline.h"
#include "version.h"

/* clang-format off */
#include "parser/koala_yacc.h"
#include "parser/koala_lex.h"
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

static inline void show_banner(void)
{
    printf("Koala %s (%s, %s)\n", KOALA_VERSION, __DATE__, __TIME__);

    struct utsname sysinfo;
    if (!uname(&sysinfo)) {
#if defined(__clang__)
        printf("[clang %d.%d.%d] on %s/%s\n", __clang_major__, __clang_minor__,
               __clang_patchlevel__, sysinfo.sysname, sysinfo.machine);
#elif defined(__GNUC__)
        printf("[gcc %d.%d.%d] on %s/%s\n", __GNUC__, __GNUC_MINOR__,
               __GNUC_PATCHLEVEL__, sysinfo.sysname, sysinfo.machine);
#else
#error "unknown supported plateform"
#endif
    }

    printf("Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>\n");
}

#define PROMPT      ">>> "
#define MORE_PROMPT "... "

static ParserState ps;
static ParserGroup grp;

static void cmd_handle_stmt(ParserState *ps, Stmt *stmt)
{
    if (!stmt) return;
    parser_enter_scope(ps, SCOPE_TOP, 0);
    ps->scope->stbl = grp.stbl;
    ps->scope->sym = grp.sym;
    parse_stmt(ps, stmt);
    parser_exit_scope(ps);
    free_stmt(stmt);
}

static int empty(char *buf, int len)
{
    char ch;
    for (int i = 0; i < len; i++) {
        ch = buf[i];
        if (ch != '\r' && ch != '\n' && ch != ' ') return 0;
    }
    return 1;
}

static int stdin_input(ParserState *ps, char *buf, int size, FILE *in)
{
    int len;
    if (ps->more) {
        len = readline(MORE_PROMPT, buf, size);
    } else {
        ps->errors = 0;
        len = readline(PROMPT, buf, size);
    }

    if (len == 0) {
        if (ferror(stdin)) clearerr(stdin);
        ps->quit = 1;
        return 0;
    }

    if (!empty(buf, len)) {
        // for multi-line statement, if it's completed, yacc will clear it.
        ps->more = 1;
    }

    /* save to line buffer */
    RESET_BUF(ps->linebuf);
    buf_write_nstr(&ps->linebuf, buf, len - 2);

    return len;
}

static void init_cmdline(void)
{
    grp.pkgs = stbl_new();
    grp.sym = stbl_add_package(grp.pkgs, null, "stdin");
    grp.stbl = ((PackSymbol *)grp.sym)->stbl;
    vector_init_ptr(&grp.files);

    ps.cmdline = 1;
    ps.row = 1;
    ps.col = 1;
    ps.filename = "stdin";
    ps.grp = &grp;
    vector_init_ptr(&ps.stmts);
    vector_init_ptr(&ps.scope_stack);

    INIT_BUF(ps.linebuf);

    INIT_BUF(ps.buf);
    vector_init(&ps.svec, sizeof(Buffer));

    ps.lexer_input = stdin_input;
    ps.handle_stmt = cmd_handle_stmt;

    init_readline();
    show_banner();
}

static void fini_cmdline(void)
{
    fini_readline();
    // FINI_BUF(cmdps.linebuf);
    FINI_BUF(ps.buf);
}

void koala_cmdline(void)
{
    init_cmdline();

    yyscan_t scanner;
    yylex_init_extra(&ps, &scanner);
    yyset_in(stdin, scanner);
    yyparse(&ps, scanner);
    yylex_destroy(scanner);

    fini_cmdline();
}

#ifdef __cplusplus
}
#endif

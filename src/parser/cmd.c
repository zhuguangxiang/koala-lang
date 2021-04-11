/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2020-2021 James <zhuguangxiang@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 */

// clang-format off
#include <sys/utsname.h>
#include "version.h"
#include "parser.h"
#include "koala_yacc.h"
#include "koala_lex.h"
#include "readline.h"
// clang-format on

#ifdef __cplusplus
extern "C" {
#endif

#define PROMPT      ">>> "
#define MORE_PROMPT "    "

static yyscan_t scanner;
static Module cmdmod;
static ParserState cmdps;
static SymbolRef cursym;

static void init_cmd_env(void)
{
    cmdmod.path = "__main__";
    cmdmod.name = "__main__";
    cmdmod.stbl = stbl_new();
    VectorInit(&cmdmod.pss, sizeof(void *));

    cmdps.cmd = 1;
    cmdps.line = 1;
    cmdps.col = 1;
    cmdps.filename = "stdin";
    cmdps.in = stdin;
    cmdps.mod = &cmdmod;

    // RESET_BUF(cmdps.linebuf);
    RESET_BUF(cmdps.buf);
    VectorInit(&cmdps.svec, sizeof(Buffer));
}

static void fini_cmd_env(void)
{
    // FINI_BUF(cmdps.linebuf);
    FINI_BUF(cmdps.buf);
}

int cmd_add_var(ParserStateRef ps, Ident id)
{
    cursym = stbl_add_var(cmdmod.stbl, id.name, NULL);
    if (cursym != NULL) {
        return 0;
    } else {
        printf("'%s' is redeclared", id.name);
        return -1;
    }
}

void cmd_eval_stmt(ParserStateRef ps, StmtRef stmt)
{
    parse_stmt(ps, stmt);
}

static void show_banner(void)
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

void kl_cmdline(void)
{
    InitAtom();
    init_cmd_env();
    InitReadLine();

    show_banner();
    yylex_init_extra(&cmdps, &scanner);
    yyset_in(stdin, scanner);
    cmdps.lexer = scanner;
    yyparse(&cmdps, scanner);
    yylex_destroy(scanner);

    FiniReadLine();
    fini_cmd_env();
    FiniAtom();
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

int stdin_input(ParserStateRef ps, char *buf, int size)
{
    int len;
    if (ps->more) {
        len = ReadLine(MORE_PROMPT, buf, size);
    } else {
        len = ReadLine(PROMPT, buf, size);
    }

    if (!empty(buf, len)) {
        // BufWriteNStr(&ps->linebuf, buf, len - 2);
        ps->more = 1;
    }

    return len;
}

#ifdef __cplusplus
}
#endif

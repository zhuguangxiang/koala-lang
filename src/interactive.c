/*
MIT License

Copyright (c) 2018 James, https://github.com/zhuguangxiang

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/utsname.h>
#include "version.h"
#include "parser.h"
#include "koala_yacc.h"
#include "koala_lex.h"
#include <readline/readline.h>
#include <readline/history.h>

#define PROMPT      "> "
#define MORE_PROMPT ". "

static void show_banner(void)
{
  printf("koala %s (%s, %s)\n", KOALA_VERSION, __DATE__, __TIME__);

  struct utsname sysinfo;
  if (!uname(&sysinfo)) {
    printf("[GCC %d.%d.%d] on %s %s\n",
           __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__,
           sysinfo.sysname, sysinfo.release);
  }
}

static struct module mod;
static struct parserstate ps;

void koala_active(void)
{
  yyscan_t scanner;

  ps.interactive = 1;

  show_banner();
  yylex_init_extra(&ps, &scanner);
  yyset_in(stdin, scanner);
  yyparse(&ps, scanner);
  yylex_destroy(scanner);
  putchar('\n');
}

static int empty(char *buf, int size)
{
  int n;
  char ch;
  for (n = 0; n < size; ++n) {
    ch = buf[n];
    if (ch != '\n' && ch != ' ' && ch != '\t' && ch != '\r')
      return 0;
  }
  return 1;
}

int interactive(struct parserstate *ps, char *buf, int size)
{
  char *line;
  if (ps->more) {
    line = readline(MORE_PROMPT);
  } else {
    line = readline(PROMPT);
  }

  if (!line) {
    if (ferror(stdin)) clearerr(stdin);
    return 0;
  }

  strcpy(buf, line);
  int len = strlen(buf);
  assert(len < size - 2);
  buf[len++] = '\n';
  buf[len++] = ' ';

  if (empty(buf, len)) {
    ps->more = 0;
  } else {
    ps->more++;
  }

  return len;
}

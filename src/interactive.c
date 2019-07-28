/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
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
static struct parser_state ps;

void Koala_Active(void)
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

int interactive(struct parser_state *ps, char *buf, int size)
{
  char *line;
  /* TAB as insert, not completion */
  rl_bind_key('\t', rl_insert);
  /* set TAB width as 2 spaces */
  rl_generic_bind(ISMACR, "\t", "  ", rl_get_keymap());

  if (ps->more) {
    line = readline(MORE_PROMPT);
  } else {
    line = readline(PROMPT);
  }

  if (!line) {
    if (ferror(stdin)) clearerr(stdin);
    return 0;
  }

  /* add history of readline */
  add_history(line);

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

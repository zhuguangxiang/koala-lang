/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "parser.h"
#include "koala_yacc.h"
#include "koala_lex.h"
#include "vector.h"
#include "log.h"

int file_input(parserstate *ps, char *buf, int size, FILE *in)
{
  errno = 0;
  int result = 0;
  while ((result = (int)fread(buf, 1, (size_t)size, in)) == 0 && ferror(in)) {
    if (errno != EINTR) {
      error("Input in scanner failed.");
      break;
    }
    errno = 0;
    clearerr(in);
  }
  return result;
}

int exist(char *path)
{
  struct stat sb;
  if (stat(path, &sb) < 0)
    return 0;
  return 1;
}

int isdotkl(char *filename)
{
  char *dot = strrchr(filename, '.');
  if (!dot || strlen(dot) != 3)
    return 0;
  if (dot[1] == 'k' && dot[2] == 'l')
    return 1;
  return 0;
}

int isdotklc(char *filename)
{
  char *dot = strrchr(filename, '.');
  if (!dot || strlen(dot) != 4)
    return 0;
  if (dot[1] == 'k' && dot[2] == 'l' && dot[3] == 'c')
    return 1;
  return 0;
}

int validate_srcfile(char *path)
{
  char *filename = strrchr(path, '/');
  if (!filename)
    filename = path;

  if (!isdotkl(filename)) {
    error("%s: Not a valid koala source file.", path);
    return 0;
  }

  struct stat sb;

  char *dir = string_ndup(path, strlen(path) - 3);
  if (!stat(dir, &sb)) {
    kfree(dir);
    error("%s: The same name file or directory exist.", path);
    return 0;
  } else {
    kfree(dir);
  }

  dir = strrchr(path, '/');
  if (!dir) {
    if (!stat("./__init__.kl", &sb)) {
      error("Not allowed '__init__.kl' exist, "
             "when single source file '%s' is compiled.", path);
      return 0;
    }
  } else {
    int extra = strlen("./__init__.kl");
    dir = string_ndup_extra(path, dir - path + 1, extra);
    strcat(dir, "./__init__.kl");
    if (!stat(dir, &sb)) {
      kfree(dir);
      error("Not allowed '__init__.kl' exist, "
             "when single source file '%s' is compiled.", path);
      return 0;
    }
    kfree(dir);
  }

  return 1;
}

/* koala -c a/b/foo.kl [a/b/foo] */
void Koala_Compile(char *path)
{
  if (isdotkl(path)) {
    /* single source file */

  } else {
    /* module directory */
  }
}

void build_ast(module *mod, char *path)
{
  /*
  FILE *in = fopen(path, "r");
  if (in == NULL) {
    error("%s: No such file or directory.", path);
    return;
  }
  parserstate *ps = new_parser(mod, path);
  yyscan_t scanner;
  yylex_init_extra(ps, &scanner);
  yyset_in(in, scanner);
  yyparse(ps, scanner);
  yylex_destroy(scanner);
  fclose(in);
  */
}
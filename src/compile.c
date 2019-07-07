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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "compile.h"
#include "koala_lex.h"

int file_input(struct parserstate *ps, char *buf, int size, FILE *in)
{
  errno = 0;
  int result = 0;
  while ((result = (int)fread(buf, 1, (size_t)size, in)) == 0 && ferror(in)) {
    if (errno != EINTR) {
      fprintf(stdout, "input in scanner failed");
      break;
    }
    errno = 0;
    clearerr(in);
  }
  return result;
}

static inline int isdotkl(char *filename)
{
  char *dot = strrchr(filename, '.');
  if (!dot || strlen(dot) != 3)
    return 0;
  if (dot[1] == 'k' && dot[2] == 'l' && dot[3] == '\0')
    return 1;
  return 0;
}

static int valid_source(char *path)
{
  char *filename = strrchr(path, '/');
  if (!filename)
    filename = path;

  if (!isdotkl(filename)) {
    fprintf(stdout, "%s: Not a valid koala source file.\n", path);
    return 0;
  }

  char *dir = strndup(path, strlen(path) - 3);
  struct stat sb;
  if (!stat(dir, &sb)) {
    free(dir);
    fprintf(stdout, "%s: The same name directory is exist.\n", path);
    return 0;
  }

  free(dir);
  return 1;
}

struct parserstate ps_test;

void koala_compile(char *path)
{
  struct stat sb;
  if (stat(path, &sb) < 0) {
    fprintf(stdout, "%s: No such file or directory.\n", path);
    return;
  }

  if (S_ISREG(sb.st_mode)) {
    /* source file */
    if (!valid_source(path))
      return;

    FILE *in = fopen(path, "r");
    assert(in);
    yyscan_t scanner;
    yylex_init_extra(&ps_test, &scanner);
    yyset_in(in, scanner);
    yyparse(&ps_test, scanner);
    yylex_destroy(scanner);
    fclose(in);
    return;
  }

  if (S_ISDIR(sb.st_mode)) {
    /* module directory */
  }
}

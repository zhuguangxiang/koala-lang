/*
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <sys/utsname.h>
#include "env.h"
#include "parser.h"
#include "koala_yacc.h"
#include "koala_lex.h"
#include "options.h"
#include "mem.h"
#include "log.h"

static int isdotkl(char *filename)
{
  char *dot = strrchr(filename, '.');
  if (dot == NULL)
    return 0;
  if (dot[1] == 'k' && dot[2] == 'l' && dot[3] == '\0')
    return 1;
  return 0;
}

/*
 * compile one package
 * compile all source files in the package and output into 'pkg-name'.klc
 * input: the package's dir
 */
static int compile(char *input, char *output, struct options *options)
{
  struct stat sb;
  if (lstat(input, &sb) == - 1) {
    fprintf(stderr, "%s: invalid filename\n", input);
    return -1;
  }

  if (!S_ISDIR(sb.st_mode)) {
    fprintf(stderr, "%s: is not a directory\n", input);
    return -1;
  }

  DIR *dir = opendir(input);
  if (!dir) {
    fprintf(stderr, "%s: no such file or directory\n", input);
    return -1;
  }

  Vector vec;
  Vector_Init(&vec);

  PkgInfo pkg;
  Init_PkgInfo(&pkg, output, options);
  ParserState *ps;
  int errnum = 0;

  struct dirent *dent;
  char name[512];
  FILE *in;
  while ((dent = readdir(dir))) {
    if (!isdotkl(dent->d_name))
      continue;
    sprintf(name, "%s/%s", input, dent->d_name);
    if (lstat(name, &sb) != 0) continue;
    if (S_ISDIR(sb.st_mode)) continue;
    printf("compile %s\n", name);
    in = fopen(name, "r");
    if (!in) {
      printf("%s: no such file or directory\n", name);
      continue;
    }
    ps = New_Parser(&pkg, dent->d_name);
    yyset_in(in, ps->scanner);
    yyparse(ps, ps->scanner);
    fclose(in);
    Vector_Append(&vec, ps);
    errnum += ps->errnum;
  }
  closedir(dir);

  Vector_ForEach(ps, &vec) {
    //if (ps->errnum <= 0)
    //  Parse(ps);
    Destroy_Parser(ps);
  }

  if (errnum <= 0) {
    //parse_module_scope(pkg);
    //Generate_KImage(pkg->sym->ptr, pkg->pkgname, pkg->pkgfile);
  } else {
    fprintf(stderr, "There are %d errors.\n", errnum);
  }

  Show_PkgInfo_Symbols(&pkg);

  Vector_Fini(&vec, NULL, NULL);
  Fini_PkgInfo(&pkg);
  return 0;
}

#define KOALAC_VERSION "0.8.5"

static void show_version(void)
{
  struct utsname sysinfo;
  if (!uname(&sysinfo)) {
    fprintf(stderr, "Koalac version %s, %s/%s\n",
            KOALAC_VERSION, sysinfo.sysname, sysinfo.machine);
  }
}

static void show_usage(char *prog)
{
  fprintf(stderr,
    "Usage: %s [<options>] <source packages>\n"
    "Options:\n"
    "\t-p <path>         Specify where to find external packages.\n"
    "\t-o <directory>    Specify where to place generated packages.\n"
    "\t-s <directory>    Specify where to find source packages.\n"
    "\t-v                Print compiler version.\n"
    "\t-h                Print this message.\n",
    prog);
}

static void init_compiler(void)
{
  AtomString_Init();
  Init_TypeDesc();
}

static void fini_compiler(void)
{
  Fini_TypeDesc();
  AtomString_Fini();
}

int main(int argc, char *argv[])
{
  struct options opts;
  init_options(&opts);
  set_options_usage(&opts, show_usage);
  set_options_version(&opts, show_version);
  parse_options(argc, argv, &opts);
  show_options(&opts);

  //Koala_Initialize();
  init_compiler();

  /*
   * search klc pathes:
   * 1. current(output) directory
   * 2. -p directory
   */
  // if (opts.outpath == NULL)
  //   Koala_Add_Property(KOALA_PATH, ".");
  // else
  //   Koala_Add_Property(KOALA_PATH, opts.outpath);
  // char *path;
  // Vector_ForEach(path, &opts.pathes) {
  //   Koala_Add_Property(KOALA_PATH, path);
  // }

  char *input;
  char *output;
  int srclen = 0;
  int outlen = 0;
  if (opts.srcpath != NULL)
    srclen = strlen(opts.srcpath);
  if (opts.outpath != NULL)
    outlen = strlen(opts.outpath);

  char *s;
  Vector_ForEach(s, &opts.names) {
    /* if not specify srcpath, use ./ as default srcpath */
    input = mm_alloc(srclen + strlen(s) + 4);
    if (opts.srcpath != NULL)
      sprintf(input, "%s/%s", opts.srcpath, s);
    else
      strcpy(input, s);

    output = mm_alloc(outlen + strlen(s) + 8);
    if (opts.outpath != NULL)
      sprintf(output, "%s/%s.klc", opts.outpath, s);
    else
      sprintf(output, "%s.klc", s);

    Log_Debug("input:%s", input);
    Log_Debug("outout:%s", output);
    compile(input, output, &opts);
    mm_free(input);
    mm_free(output);
  }

  //Koala_Finalize();
  fini_compiler();

  fini_options(&opts);

  show_memstat();
  return 0;
}

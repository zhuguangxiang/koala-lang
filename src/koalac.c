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
#include "stringbuf.h"
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
static void compile(char *pkgdir, PkgInfo *pkg)
{
  struct stat sb;
  if (lstat(pkgdir, &sb) == - 1) {
    fprintf(stderr, "%s: invalid filename\n", pkgdir);
    return;
  }

  if (!S_ISDIR(sb.st_mode)) {
    fprintf(stderr, "%s: is not a directory\n", pkgdir);
    return;
  }

  DIR *dir = opendir(pkgdir);
  if (!dir) {
    fprintf(stderr, "%s: no such file or directory\n", pkgdir);
    return;
  }

  VECTOR(psvec);

  ParserState *ps;
  int errnum = 0;

  struct dirent *dent;
  char name[512];
  FILE *in;
  while ((dent = readdir(dir))) {
    if (!isdotkl(dent->d_name))
      continue;

    sprintf(name, "%s/%s", pkgdir, dent->d_name);
    if (lstat(name, &sb) != 0 || S_ISDIR(sb.st_mode))
      continue;

    Log_Debug("compile %s", name);

    in = fopen(name, "r");
    if (!in) {
      printf("%s: no such file or directory\n", name);
      continue;
    }

    ps = New_Parser(pkg, dent->d_name);
    Vector_Append(&psvec, ps);

    /* build abstact syntax tree */
    errnum += Build_AST(ps, in);

    fclose(in);
  }

  closedir(dir);

  /* semantic analysis and code generation */
  Vector_ForEach(ps, &psvec) {
    if (ps->errnum <= 0)
      Parse(ps);
    Destroy_Parser(ps);
  }

  Vector_Fini_Self(&psvec);

  if (errnum <= 0) {
    KImage *image = Gen_KImage(pkg->stbl);
    assert(image != NULL);
    KImage_Write_File(image, pkg->pkgfile.str);
    KImage_Free(image);
  } else {
    fprintf(stderr, "There are %d errors.\n", errnum);
  }
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
    "\t-Dname=value      Set a property.\n"
    "\t-v                Print compiler version.\n"
    "\t-h                Print this message.\n",
    prog);
}

int main(int argc, char *argv[])
{
  AtomString_Init();
  Init_TypeDesc();

  Options opts;
  init_options(&opts, show_usage, show_version);
  parse_options(argc, argv, &opts);
  show_options(&opts);

  char *pkgname;
  char *s;
  Vector_ForEach(s, &opts.names) {
    DeclareStringBuf(input);
    DeclareStringBuf(output);

    /* if not specify srcpath, use ./ as default srcpath */
    if (opts.srcpath != NULL)
      StringBuf_Format_CStr(input, "#/#", opts.srcpath, s);
    else
      StringBuf_Append_CStr(input, s);

    /* if not specify outpath, use ./ as default outpath */
    if (opts.outpath != NULL)
      StringBuf_Format_CStr(output, "#/#.klc", opts.outpath, s);
    else
      StringBuf_Format_CStr(output, "#.klc", s);

    /* if package dir has slash, get the last dir name as package name */
    pkgname = strrchr(s, '/');
    if (pkgname == NULL)
      pkgname = s;

    Log_Debug("input:%s", input.data);
    Log_Debug("outout:%s", output.data);
    Log_Debug("package:%s", pkgname);

    PkgInfo pkg;
    Init_PkgInfo(&pkg, pkgname, output.data, &opts);
    compile(input.data, &pkg);
    Show_PkgInfo(&pkg);
    Fini_PkgInfo(&pkg);

    FiniStringBuf(input);
    FiniStringBuf(output);
  }

  fini_options(&opts);

  Fini_TypeDesc();
  AtomString_Fini();

  show_memstat();
  return 0;
}

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

LOGGER(0)

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
    "\t-p <path>          Specify where to find external packages.\n"
    "\t-o <directory>     Specify where to place generated packages.\n"
    "\t-s <directory>     Specify where to find source packages.\n"
    "\t-v                 Print compiler version.\n"
    "\t-h                 Print this message.\n",
    prog);
}

static inline int isdotkl(char *filename)
{
  char *dot = strrchr(filename, '.');
  if (dot == NULL)
    return 0;
  if (dot[1] == 'k' && dot[2] == 'l' && dot[3] == '\0')
    return 1;
  return 0;
}

/* trim tail slashes, NOTES: string needs writable */
static inline void trim_tail_slashes(char *s)
{
  char *slash = s + strlen(s);
  while (*--slash == '/');
  *(slash + 1) = '\0';
}

/* packages, path as key */
static HashTable packages;
/* ParserGroup for compile, MAY add new one at compiling-time */
static Vector groups;

Package *New_Package(char *path)
{
  Package *pkg = Malloc(sizeof(Package));
  Init_HashNode(&pkg->hnode, pkg);
  pkg->path = AtomString_New(path).str;
  int res = HashTable_Insert(&packages, &pkg->hnode);
  assert(!res);
  return pkg;
}

void Free_Package(Package *pkg)
{
  HashTable_Remove(&packages, &pkg->hnode);
  STable_Free(pkg->stbl);
  Mfree(pkg);
}

Package *Find_Package(char *path)
{
  Package key = {.path = path};
  HashNode *hnode = HashTable_Find(&packages, &key);
  if (hnode == NULL)
    return NULL;
  return container_of(hnode, Package, hnode);
}

const char *scope_strings[] = {
  "PACKAGE", "MODULE", "CLASS", "FUNCTION", "BLOCK", "CLOSURE"
};

static ParserGroup *new_group(char *path)
{
  ParserGroup *grp = Malloc(sizeof(ParserGroup));
  memset(grp, 0, sizeof(ParserGroup));
  grp->pkg = New_Package(path);
  grp->pkg->stbl = STable_New();
  Vector_Init(&grp->modules);
  return grp;
}

static void free_group(ParserGroup *grp)
{
  Vector_Fini_Self(&grp->modules);
  Mfree(grp);
}

static void show_group(ParserGroup *grp)
{
  Log_Puts("\n----------------------------------------");
  Log_Printf("scope-0(%s, %s) symbols:\n",
             scope_strings[0], grp->pkg->pkgname);
  STable_Show(grp->pkg->stbl);
  Log_Puts("----------------------------------------\n");
}

/* add new ParserGroup to be compiled */
void Add_ParserGroup(char *pkgpath)
{
  Options *opts = &options;

  ParserGroup *grp;
  Vector_ForEach(grp, &groups) {
    if (!strcmp(grp->pkg->path, pkgpath)) {
      Log_Debug("package '%s' is exist.", pkgpath);
      return;
    }
  }

  Vector_Append(&groups, new_group(pkgpath));
}

static void build_symbols(ParserGroup *grp)
{
  char *path = grp->pkg->path;

  /* if specify srcpath, use absolute path */
  if (options.srcpath != NULL)
    path = AtomString_Format("#/#", options.srcpath, path);

  struct stat sb;
  if (stat(path, &sb) == - 1) {
    fprintf(stderr, "%s: No such file or directory\n", path);
    return;
  }

  if (!S_ISDIR(sb.st_mode)) {
    fprintf(stderr, "%s: Not a directory\n", path);
    return;
  }

  DIR *dir = opendir(path);
  if (!dir) {
    fprintf(stderr, "%s: No such file or directory\n", path);
    return;
  }

  ParserState *ps;
  int errors = 0;

  struct dirent *dent;
  /* FIXME */
  char name[512];
  FILE *in;
  while ((dent = readdir(dir))) {
    if (!isdotkl(dent->d_name))
      continue;

    snprintf(name, 511, "%s/%s", path, dent->d_name);
    if (lstat(name, &sb) != 0 || S_ISDIR(sb.st_mode))
      continue;

    Log_Debug("---------------------------------------------");
    Log_Debug("build AST: %s", name);

    in = fopen(name, "r");
    if (!in) {
      Log_Printf("%s: no such file or directory\n", name);
      continue;
    }

    ps = New_Parser(grp, dent->d_name);

    /* build abstact syntax tree */
    Build_AST(ps, in);
    Vector_Append(&grp->modules, ps);

    fclose(in);
  }

  closedir(dir);
}

static int parse_package(ParserGroup *grp)
{
  int errors = 0;
  ParserState *ps;

  Vector_ForEach(ps, &grp->modules) {
    Parse_Imports(ps);
    CheckConflictWithExternal(ps);
    errors += ps->errnum;
  }

  if (errors > 0)
    return errors;

  Vector_ForEach(ps, &grp->modules) {
    Parse_VarStmts(ps);
    errors += ps->errnum;
  }

  if (errors > 0) {
    // parse variable declarations firstly has errors
    // parse them twice
    errors = 0;
    Vector_ForEach(ps, &grp->modules) {
      Parse_VarStmts(ps);
      errors += ps->errnum;
    }
  }

  Vector_ForEach(ps, &grp->modules) {
    Parse_AST(ps);
    errors += ps->errnum;
    Destroy_Parser(ps);
  }
  return errors;
}

static void write_image(Package *pkg)
{
  char *path = pkg->path;
  KImage *image = Gen_Image(pkg->stbl, pkg->pkgname);
  assert(image != NULL);
  if (options.outpath != NULL)
    path = AtomString_Format("#/#.klc", options.outpath, path);
  else
    path = AtomString_Format("#.klc", path);
  Log_Debug("write image to file '%s'", path);
  KImage_Show(image);
  KImage_Write_File(image, path);
  KImage_Free(image);
}

static uint32 pkg_hash_func(void *k)
{
  Package *pkg = k;
  return hash_string(pkg->path);
}

static int pkg_equal_func(void *k1, void *k2)
{
  Package *pkg1 = k1;
  Package *pkg2 = k2;
  return !strcmp(pkg1->path, pkg2->path);
}

static void pkg_free_func(HashNode *hnode, void *arg)
{
  UNUSED_PARAMETER(arg);
  Package *pkg = container_of(hnode, Package, hnode);
  STable_Free(pkg->stbl);
  Mfree(pkg);
}

static inline void init_koalac(void)
{
  /* initialize command options */
  init_options(&options, show_usage, show_version);
  /* initialize package hashtable */
  HashTable_Init(&packages, pkg_hash_func, pkg_equal_func);
  /* intialize groups */
  Vector_Init(&groups);
}

static inline void fini_koalac(void)
{
  HashTable_Fini(&packages, pkg_free_func, NULL);
  fini_options(&options);
  Vector_Fini_Self(&groups);
}

/* compiling options */
Options options;

int main(int argc, char *argv[])
{
  AtomString_Init();
  Init_TypeDesc();
  init_koalac();

  /* parse command's parameters */
  parse_options(argc, argv, &options);
  show_options(&options);

  /* add packages to be compiled */
  char *s;
  Vector_ForEach(s, &options.args) {
    trim_tail_slashes(s);
    Log_Debug("compile package '%s'", s);
    Add_ParserGroup(s);
  }

  /* build ASTs */
  ParserGroup *grp;
  Vector_ForEach(grp, &groups)
    build_symbols(grp);

  /* parse ASTs */
  Vector_ForEach(grp, &groups) {
    if (parse_package(grp) <= 0) {
      show_group(grp);
      write_image(grp->pkg);
    }
    free_group(grp);
  }

  fini_koalac();
  Fini_TypeDesc();
  AtomString_Fini();

  Show_MemStat();
  return 0;
}

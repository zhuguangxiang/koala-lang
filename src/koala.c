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

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/utsname.h>
#include "packageobject.h"
#include "tupleobject.h"
#include "stringobject.h"
#include "intobject.h"
#include "koala.h"
#include "options.h"
#include "pkgnode.h"
#include "env.h"
#include "properties.h"
#include "eval.h"
#include "task.h"
#include "log.h"

LOGGER(0)

#define KOALA_VERSION "0.8.5"

static void show_version(void)
{
  struct utsname sysinfo;
  if (!uname(&sysinfo)) {
    fprintf(stderr, "Koala version %s, %s/%s\n",
            KOALA_VERSION, sysinfo.sysname, sysinfo.machine);
  }
}

static void show_usage(char *prog)
{
  fprintf(stderr,
    "Usage: %s [<options>] <main package>\n"
    "Options:\n"
    "\t-p <path>          Specify where to find external packages.\n"
    "\t-D <name>=<value>  Set a property.\n"
    "\t-v                 Print virtual machine version.\n"
    "\t-h                 Print this message.\n",
    prog);
}

/* environment values */
static Properties properties;
/* package tree */
static PkgState pkgstate;
/*
 * global variables' pool(tuple's vector)
 * access by index stored in PackageObject
 */
static VECTOR(variables);
/* list of KoalaState */
static int ksnum;
static LIST_HEAD(kslist);

static inline void init_packages(void)
{
  Init_String_Klass();
  Init_Integer_Klass();
  Init_Tuple_Klass();
  Init_Package_Klass();
  Object *pkg = Package_New("lang");
  Package_Add_Class(pkg, &String_Klass);
  Package_Add_Class(pkg, &Int_Klass);
  Package_Add_Class(pkg, &Tuple_Klass);
  Package_Add_Class(pkg, &Package_Klass);
  Koala_Add_Package("lang", pkg);
}

static void init_koala(void)
{
  AtomString_Init();
  Init_TypeDesc();
  Properties_Init(&properties);
  Init_PkgState(&pkgstate);
  init_packages();
  Show_PkgState(&pkgstate);
}

static inline void fini_packages(void)
{
  Fini_PkgState(&pkgstate, NULL, NULL);
  Fini_Package_Klass();
  Fini_Tuple_Klass();
  Fini_Integer_Klass();
  Fini_String_Klass();
}

static inline void fini_koala(void)
{
  fini_packages();
  Fini_TypeDesc();
  AtomString_Fini();
}

int Koala_Add_Package(char *path, Object *ob)
{
  PackageObject *pkg = (PackageObject *)ob;
  PkgNode *node = Add_PkgNode(&pkgstate, path, pkg);
  if (node == NULL) {
    Log_Debug("add package '%s' failed", pkg->name);
    return -1;
  }
  Log_Debug("new package '%s'", pkg->name);
  pkg->index = Vector_Size(&variables);
  Vector_Append(&variables, Tuple_New(pkg->varcnt));
  return 0;
}

int Koala_Set_Value(Object *ob, char *name, Object *value)
{
  PackageObject *pkg = (PackageObject *)ob;
  int index = pkg->index;
  MemberDef *m = Package_Find_MemberDef(ob, name);
  assert(m && m->kind == MEMBER_VAR);
  Log_Debug("set var '%s'(%d) in '%s'(%d)",
            name, m->offset, pkg->name, index);
  Object *tuple = Vector_Get(&variables, index);
  OB_INCREF(value);
  return Tuple_Set(tuple, m->offset, value);
}

Object *Koala_Get_Value(Object *ob, char *name)
{
  PackageObject *pkg = (PackageObject *)ob;
  int index = pkg->index;
  MemberDef *m = Package_Find_MemberDef(ob, name);
  assert(m && m->kind == MEMBER_VAR);
  Log_Debug("get var '%s'(%d) in '%s'(%d)",
            name, m->offset, pkg->name, index);
  Object *tuple = Vector_Get(&variables, index);
  return Tuple_Get(tuple, m->offset);
}

static int koala_run_file(char *path, Vector *args)
{
  KoalaState *ks = KoalaState_New();
  task_set_object(ks);
  return 0;
}

static void *task_entry_func(void *arg)
{
  koala_run_file(arg, NULL);
  Log_Printf("task-%lu is finished.\n", current_task()->id);
  return NULL;
}

int main(int argc, char *argv[])
{
  /* parse arguments */
  Options options;
  init_options(&options, show_usage, show_version);
  parse_options(argc, argv, &options);
  show_options(&options);
  /*
   * set path for searching packages
   * 1. current source code directory
   * 2. -p (search path) directory
   */
  Properties_Put(&properties, KOALA_PATH, ".");
  char *path;
  Vector_ForEach(path, &options.pathes)
    Properties_Put(&properties, KOALA_PATH, path);
  /* get execute file */
  char *file = Vector_Get(&options.names, 0);
  /* finialize options */
  fini_options(&options);

  /* initial koala machine */
  init_koala();

  /* initial task's processors */
  task_init_procs(1);

  /* new a task and wait for it's finished */
  task_attr_t attr = {.stacksize = 1 * 512 * 1024};
  task_t *task = task_create(&attr, task_entry_func, file);
  task_join(task, NULL);

  /* finialize koala machine */
  fini_koala();

  return 0;
}

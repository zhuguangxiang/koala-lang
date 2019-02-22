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

#include "koala.h"
#include "pkgnode.h"
#include "options.h"
#include "lang.h"
#include "io.h"

LOGGER(0)

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

int Koala_Add_Package(char *path, Object *ob)
{
  PackageObject *pkg = (PackageObject *)ob;
  PkgNode *node = Add_PkgNode(&pkgstate, path, pkg);
  if (node == NULL) {
    Log_Debug("add package '%s' failed", pkg->name);
    return -1;
  }

  if (pkg->varcnt == 0) {
    pkg->index = -1;
  } else {
    pkg->index = Vector_Size(&variables);
    Vector_Append(&variables, Tuple_New(pkg->varcnt));
  }
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

static inline void koala_set_pathes(Vector *pathes)
{
  Properties_Put(&properties, KOALA_PATH, ".");
  char *path;
  Vector_ForEach(path, pathes) {
    Properties_Put(&properties, KOALA_PATH, path);
  }
}

KoalaState *New_koalaState(void)
{
  KoalaState *ks = Malloc(sizeof(KoalaState));
  init_list_head(&ks->ksnode);
  Init_Stack(&ks->stack, nr_elts(ks->objs), ks->objs);
  list_add_tail(&ks->ksnode, &kslist);
  ksnum++;
  return ks;
}

void Free_KoalaState(KoalaState *ks)
{
  list_del(&ks->ksnode);
  ksnum--;
  Mfree(ks);
}

void Koala_Main(Options *opts)
{
  /* set pathes for searching packages */
  koala_set_pathes(&opts->pathes);

  /* initialize processors */
  task_init_procs(1);

  /* set task's private object */
  task_set_private(New_koalaState());

  char *path = Vector_Get(&opts->args, 0);

  /* find or load package */
  Object *pkg = NULL; //Koala_Get_Package(path);
  if (pkg == NULL) {
    fprintf(stderr, "error: cannot open '%s.klc'\n", path);
    goto EXIT;
  }

  /* initial package's variables */
  MemberDef *m = Package_Find_MemberDef(pkg, KOALA_INIT);
  if (m == NULL || m->kind != MEMBER_CODE) {
    fprintf(stderr, "error: no '%s' function in '%s'", KOALA_INIT, path);
    goto EXIT;
  }
  Koala_RunCode(m->code, pkg, NULL);

  /* run 'main' function */
  m = Package_Find_MemberDef(pkg, KOALA_MAIN);
  if (m == NULL || m->kind != MEMBER_CODE) {
    fprintf(stderr, "error: no '%s' function in '%s'", KOALA_INIT, path);
    goto EXIT;
  }

  int size = Vector_Size(&opts->args);
  Object *args = NULL;
  if (size > 1) {
    int i = 1;
    char *str;
    args = Tuple_New(size - 1);
    while (i < size) {
      str = Vector_Get(&opts->args, i);
      Tuple_Set(args, i - 1, String_New(str));
    }
  }
  Koala_RunCode(m->code, pkg, args);

EXIT:
  Free_KoalaState(task_get_private());
  /* finalize processors */
  task_fini_procs();
}

typedef struct taskparam {
  Object *code;
  Object *ob;
  Object *args;
} TaskParam;

static void *koala_task_entry(void *arg)
{
  TaskParam *param = arg;
  task_set_private(New_koalaState());
  Koala_RunCode(param->code, param->ob, param->args);
  Mfree(param);
  return NULL;
}

void Koala_RunTask(Object *code, Object *ob, Object *args)
{
  /* new a task and run it concurrently */
  task_attr_t attr = {.stacksize = KOALA_TASK_STACKSIZE};
  TaskParam *param = Malloc(sizeof(TaskParam));
  param->code = code;
  param->ob = ob;
  param->args = args;
  task_create(&attr, koala_task_entry, param);
}

void Koala_Initialize(void)
{
  AtomString_Init();
  Init_TypeDesc();
  Properties_Init(&properties);
  Init_PkgState(&pkgstate);
  Init_Lang_Package();
  Init_IO_Package();
  Show_PkgState(&pkgstate);
}

void Koala_Finalize(void)
{
  Fini_IO_Package();
  Fini_Lang_Package();
  Fini_PkgState(&pkgstate, Package_Free_Func, NULL);
  Properties_Fini(&properties);
  Fini_TypeDesc();
  AtomString_Fini();
}

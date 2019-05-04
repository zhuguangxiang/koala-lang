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
#include "codeobject.h"
#include "io.h"

/* list of KoalaState */
static int ksnum;
static LIST_HEAD(kslist);

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
  Env_Set(KOALA_PATH, ".");
  Env_Set_Vec(KOALA_PATH, &opts->pathes);

  /* initialize processors */
  task_init_procs(1);

  /* set task's private object */
  task_set_private(New_koalaState());

  char *path = Vector_Get(&opts->args, 0);

  /* find or load package */
  Object *pkg = Find_Package(path);
  if (pkg == NULL) {
    fprintf(stderr, "\x1b[31merror\x1b[0m: cannot open '%s' package\n", path);
    goto EXIT;
  }

  /* initial package's variables */
  Object *code = Pkg_Get_Func(pkg, KOALA_INIT);
  if (code != NULL)
    Koala_RunCode(code, pkg, NULL);

  /* run 'main' function */
  code = Pkg_Get_Func(pkg, KOALA_MAIN);
  if (code == NULL) {
    fprintf(stderr, "\x1b[31merror\x1b[0m: '%s' not found in '%s'\n",
            KOALA_MAIN, path);
    goto EXIT;
  }
  CodeObject *co = (CodeObject *)code;
  ProtoDesc *proto = (ProtoDesc *)co->proto;
  assert(proto->arg == NULL && proto->ret == NULL);
  Koala_RunCode(code, pkg, NULL);

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

static void Init_Lang_Package(void)
{
  Init_Package_Klass();

  Init_Code_Klass();
  Init_Class_Klass();

  Init_String_Klass();
  Init_Integer_Klass();
  Init_Tuple_Klass();

  Object *pkg = New_Package(KOALA_PKG_LANG);
  Pkg_Add_Class(pkg, &Pkg_Klass);
  Pkg_Add_Class(pkg, &Class_Klass);
  Pkg_Add_Class(pkg, &Any_Klass);
  Pkg_Add_Class(pkg, &String_Klass);
  Pkg_Add_Class(pkg, &Int_Klass);
  Pkg_Add_Class(pkg, &Bool_Klass);
  Pkg_Add_Class(pkg, &Tuple_Klass);
  Pkg_Add_Class(pkg, &Code_Klass);
  Install_Package(KOALA_PKG_LANG, pkg);
}

static void Fini_Lang_Package(void)
{
  Fini_Package_Klass();

  Fini_Tuple_Klass();
  Fini_Integer_Klass();
  Fini_String_Klass();

  Fini_Class_Klass();
  Fini_Code_Klass();
}

void Koala_Initialize(void)
{
  AtomString_Init();
  Init_TypeDesc();
  Init_Env();
  Init_Lang_Package();
  //Init_Format_Package();
  Init_IO_Package();
}

void Koala_Finalize(void)
{
  Fini_IO_Package();
  Fini_Lang_Package();
  //Fini_Format_Package();
  Fini_Env();
  Fini_TypeDesc();
  AtomString_Fini();
}

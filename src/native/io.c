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

#include "globalstate.h"
#include "io.h"
#include "tupleobject.h"
#include "log.h"
#include "mem.h"

static Logger logger;

void fd_free(Object *ob)
{
  OB_ASSERT_KLASS(ob, Fd_Klass);
  mm_free(ob);
}

Klass Fd_Klass = {
  OBJECT_HEAD_INIT(&Klass_Klass)
  .name = "Fd",
  .basesize = sizeof(FdObject),
  .ob_free = fd_free,
};

FdObject *Stdout;
FdObject *Stdin;

FdObject *Fd_New(int fd)
{
  FdObject *ob = mm_alloc(sizeof(FdObject));
  Init_Object_Head(ob, &Fd_Klass);
  ob->fd = fd;
  return ob;
}

void Fd_Free(FdObject *fd)
{
  OB_DECREF(fd);
}

FdObject *Fd_Open(char *path, int flag)
{

}

int Fd_Write(FdObject *fd, char *data, int size)
{
  return write(fd->fd, data, size);
}

int Fd_Read(FdObject *fd, char *buf, int size)
{
  return read(fd->fd, buf, size);
}

int Fd_Close(FdObject *fd)
{
  if (fd->fd <= 0) {
    Log_Warn("Invalid fd.");
    return -1;
  }

  if (fd->fd == STDOUT_FILENO) {
    Log_Warn("Stdout cannot be closed.");
    return -1;
  }

  if (fd->fd == STDIN_FILENO) {
    Log_Warn("Stdin cannot be closed.");
    return -1;
  }

  return close(fd->fd);
}

#define BUF_SIZE 1024

static Object *__print(Object *ob, Object *args)
{
  int size = Tuple_Size(args);
  char *buf = mm_alloc(BUF_SIZE);
  Object *val;
  int n;
  for (int i = 0; i < size; i++) {
    val = Tuple_Get(args, i);
    n = Object_Print(buf, BUF_SIZE - 1, val);
    Fd_Write(Stdout, buf, n);
    if (i + 1 < size) {
      Fd_Write(Stdout, " ", 1); /* space */
    }
  }
  mm_free(buf);
  return NULL;
}

static Object *__println(Object *ob, Object *args)
{
  __print(ob, args);
  Fd_Write(Stdout, "\n", 1);
  return NULL;
}

static Object *__open(Object *ob, Object *args)
{
#if 0
  Object *s = Tuple_Get(args, 0);
  Object *i = Tuple_Get(args, 1);
  char *path = String_RawString(s);
  int flag = (int)Integer_ToCInt(i);
  int fd = open(path, flag);
  Object *ret = Tuple_New(2);
  Tuple_Set(ret, 0, Integer_New(fd));
  if (fd < 0) {
    //Tuple_Set(ret, 1, Error_New(errno, strerror(errno)));
  }
  return ret;
#endif
  return NULL;
}

static FuncDef io_funcs[] = {
  {"Println", NULL, "...", __println},
  {"Open", "Lnative/io.Fd;e", "si", __open},
  {NULL}
};

void Init_Nio_Package(void)
{
  Stdout = Fd_New(STDOUT_FILENO);
  Stdin = Fd_New(STDIN_FILENO);

  PackageObject *pkg = Package_New("io");
  Package_Add_CFunctions(pkg, io_funcs);
  Package_Add_Var(pkg, "Stdout", TypeDesc_Get_Klass("native/io", "Fd"), 1);
  Package_Add_Var(pkg, "Stdin", TypeDesc_Get_Klass("native/io", "Fd"), 1);
  Package_Add_Class(pkg, &Fd_Klass);

  Koala_Add_Package("native", pkg);
  Koala_Set_Value(pkg, "Stdout", (Object *)Stdout);
  Koala_Set_Value(pkg, "Stdin", (Object *)Stdin);
}

void Fini_Nio_Package(void)
{
  Fd_Free(Stdout);
  Fd_Free(Stdin);
}

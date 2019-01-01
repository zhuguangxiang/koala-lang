
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "stringobject.h"
#include "state.h"
#include "intobject.h"
#include "io.h"

void test_native_io(void)
{
  PackageObject *pkg = (PackageObject *)Koala_Get_Package("native/io");
  assert(pkg);
  FdObject *fd = (FdObject *)Koala_Get_Value(pkg, "Stdout");
  assert(fd);
  int n = Fd_Write(fd, "hello,world\n", 12);
  assert(n == 12);
}

int main(int argc, char *argv[])
{
  Koala_Initialize();
  test_native_io();
  Koala_Finalize();
  return 0;
}

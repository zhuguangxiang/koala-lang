/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include <inttypes.h>
#include "log.h"

int main(int argc, char *argv[])
{
  debug("hello, %s:%d", "world", 100);
  warn("this is a warnning: %s,%d", "not a dog", 100);
  error("this is an error: %s,%d", "not a cat", 100);
  debug("hello, koala");
  warn("hello, koala");
  error("hello, koala");
  printf("sizeof(int):%ld\n", sizeof(int));
  printf("sizeof(long):%ld\n", sizeof(long));
  printf("%lx\n", (uintptr_t)&argv);
  //panic(0, "hello, %s", "world");
  return 0;
}

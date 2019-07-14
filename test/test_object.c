/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "debug.h"

int main(int argc, char *argv[])
{
  debug("hello, %s:%d", "world", 100);
  warn("this is a warnning: %s,%d", "not a dog", 100);
  error("this is an error: %s,%d", "not a cat", 100);
  debug("hello, koala");
  warn("hello, koala");
  error("hello, koala");
  errmsg("hello, koala");
  return 0;
}

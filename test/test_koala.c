/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include <koala/koala.h>

int main(int argc, char *argv[])
{
  init_modules();
  koala_active();
  fini_modules();
  return 0;
}

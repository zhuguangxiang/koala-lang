/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include <assert.h>
#include <string.h>
#include "atom.h"

int main(int argc, char *argv[])
{
  atom_initialize();

  char *s = atom("Hello,");
  assert(!strcmp(s, "Hello,"));
  char *s2 = atom_string(" Koala", 6);
  assert(!strcmp(s2, " Koala"));
  char *s3 = atom("Hello,");
  assert(s == s3);
  char *s4 = atom_string(" Koala", 6);
  assert(s2 == s4);
  char *s5 = atom_nstring(2, s, s2);
  assert(!strcmp(s5, "Hello, Koala"));

  atom_destroy();
  return 0;
}

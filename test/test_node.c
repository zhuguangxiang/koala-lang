/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include <stdio.h>
#include "node.h"
#include "memory.h"
#include "atom.h"
#include "path.h"

int main(int argc, char *argv[])
{
  atom_initialize();
  node_initialize();
  char *s = "github.com";
  int len = strlen(s);
  char **arr = path_toarr(s, len);
  assert(!strcmp(arr[0], "github.com"));
  assert(!arr[1]);

  s = "github.com/koala/json";
  len = strlen(s);
  arr = path_toarr(s, len);
  assert(!strcmp(arr[0], "github.com"));
  assert(!strcmp(arr[1], "koala"));
  assert(!strcmp(arr[2], "json"));
  assert(!arr[3]);

  s = "github.com/koala/io";
  len = strlen(s);
  char **pathes1 = path_toarr(s, len);
  add_leaf(pathes1, "foo");
  s = "github.com/koala/lang";
  len = strlen(s);
  char **pathes2 = path_toarr(s, len);
  add_leaf(pathes2, "bar");
  void *data = get_leaf(pathes1);
  assert(!strcmp(data, "foo"));
  data = get_leaf(pathes2);
  assert(!strcmp(data, "bar"));
  kfree(pathes2);
  kfree(pathes1);
  kfree(arr);
  node_destroy();
  atom_destroy();
  return 0;
}

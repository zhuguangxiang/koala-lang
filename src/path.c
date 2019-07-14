/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "vector.h"
#include "atom.h"

char **path_toarr(char *path, int size)
{
  VECTOR_PTR(strs);
  char *end = path + size;
  char *p;
  char *s;
  int len;
  char **arr;

  while (*path && path < end) {
    p = path;
    while (*path && *path != '/' && path < end)
      path++;

    len = path - p;
    if (len > 0) {
      s = atom_string(p, len);
      vector_push_back(&strs, &s);
    }

    /* remove trailing slashes */
    while (*path && *path == '/' && path < end)
      path++;
  }

  arr = vector_toarr(&strs);
  vector_free(&strs, NULL, NULL);
  return arr;
}

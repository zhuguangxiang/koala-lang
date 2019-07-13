/*
MIT License

Copyright (c) 2018 James, https://github.com/zhuguangxiang

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdio.h>
#include "node.h"
#include "memory.h"
#include "atom.h"

int main(int argc, char *argv[])
{
  atom_initialize();
  node_initialize();
  char **arr = path_toarr("github.com/koala/json");
  assert(!strcmp(arr[0], "github.com"));
  assert(!strcmp(arr[1], "koala"));
  assert(!strcmp(arr[2], "json"));
  assert(!arr[3]);

  char **pathes1 = path_toarr("github.com/koala/io");
  add_leaf(pathes1, "foo");
  char **pathes2 = path_toarr("github.com/koala/lang");
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

/*
 MIT License

 Copyright (c) 2018 James, https://github.com/zhuguangxiang

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
*/

#include "log.h"
#include <stdio.h>
#include "node.h"
#include "memory.h"
#include "atom.h"

int main(int argc, char *argv[])
{
  atom_init();
  node_init();

  char *s = "github.com";
  int len = strlen(s);
  char **arr = path_toarr(s, len);
  expect(!strcmp(arr[0], "github.com"));
  expect(!arr[1]);

  s = "github.com/koala/json";
  len = strlen(s);
  arr = path_toarr(s, len);
  expect(!strcmp(arr[0], "github.com"));
  expect(!strcmp(arr[1], "koala"));
  expect(!strcmp(arr[2], "json"));
  expect(!arr[3]);

  s = "github.com/koala/io";
  len = strlen(s);
  char **pathes1 = path_toarr(s, len);
  add_leaf(pathes1, "foo");
  s = "github.com/koala/lang";
  len = strlen(s);
  char **pathes2 = path_toarr(s, len);
  add_leaf(pathes2, "bar");
  void *data = get_leaf(pathes1);
  expect(!strcmp(data, "foo"));
  data = get_leaf(pathes2);
  expect(!strcmp(data, "bar"));
  kfree(pathes2);
  kfree(pathes1);
  kfree(arr);

  node_fini();
  atom_fini();
  return 0;
}

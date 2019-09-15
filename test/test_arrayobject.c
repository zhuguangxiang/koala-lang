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

#include "koala.h"
#include "log.h"

/*
  arr := []
  arr.append("hello")
  arr.append("world")
  print(arr)
  arr.pop()
  arr.pop()
 */
int main(int argc, char *argv[])
{
  koala_initialize();
  TypeDesc *desc = desc_from_str;
  Object *arr = array_new(desc);
  TYPE_DECREF(desc);
  Object *v = string_new("hello");
  Object_Call(arr, "append", v);
  OB_DECREF(v);

  v = string_new("world");
  Object_Call(arr, "append", v);
  OB_DECREF(v);

  Array_Print(arr);

  Object *i = Integer_New(1);

  v = Object_Call(arr, "__getitem__", i);
  expect(!strcmp("world", String_AsStr(v)));
  OB_DECREF(i);
  OB_DECREF(v);

  v = Object_Call(arr, "pop", NULL);
  expect(!strcmp("world", String_AsStr(v)));
  OB_DECREF(v);

  v = Object_Call(arr, "pop", NULL);
  expect(!strcmp("hello", String_AsStr(v)));
  OB_DECREF(v);

  OB_DECREF(arr);
  koala_finalize();
  return 0;
}

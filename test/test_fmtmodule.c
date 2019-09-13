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

int main(int argc, char *argv[])
{
  koala_initialize();

  Object *m = Module_Load("fmt");
  Object *fmtstr = String_New("Hello, {}. I was born at {}.");
  Object *name = String_New("Koala");
  Object *year = Integer_New(2018);
  Object *args = Tuple_Pack(3, fmtstr, name, year);
  Object_Call(m, "println", args);
  OB_DECREF(args);
  OB_DECREF(fmtstr);

  fmtstr = String_New("{}");
  Object *tuple = Tuple_Pack(4, name, year, name, year);
  args = Tuple_Pack(2, fmtstr, tuple);
  Object_Call(m, "println", args);
  OB_DECREF(args);
  OB_DECREF(tuple);
  OB_DECREF(fmtstr);

  OB_DECREF(year);
  OB_DECREF(name);

  OB_DECREF(m);

  koala_finalize();
  return 0;
}

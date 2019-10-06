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

#include "opcode.h"
#include "intobject.h"
#include "floatobject.h"
#include "stringobject.h"

Object *num_add(Object *x, Object *y)
{
  Object *z;
  if (integer_check(x) && integer_check(y)) {
    int64_t a, r;
    a = integer_asint(x);
    if (integer_check(y)) {
      int64_t b = integer_asint(y);
      r = (int64_t)((uint64_t)a + b);
    } else if (byte_check(y)) {
      int b = byte_asint(y);
      r = (int64_t)((uint64_t)a + b);
    } else if (Float_Check(y)) {
      double b = Float_AsFlt(y);
      r = (int64_t)((double)a + b);
    } else {
      panic("Not implemented");
    }
    z = integer_new(r);
  } else if (Float_Check(x) && Float_Check(y)) {
    double a, r;
    a = Float_AsFlt(x);
    if (Float_AsFlt(y)) {

    } else if (integer_check(y)) {

    } else if (byte_check(y)) {

    } else {
      panic("Not implemented");
    }
  } else if (string_check(x) && string_check(y)) {
    STRBUF(sbuf);
    strbuf_append(&sbuf, string_asstr(x));
    strbuf_append(&sbuf, string_asstr(y));
    string_set(x, strbuf_tostr(&sbuf));
    strbuf_fini(&sbuf);
    z = OB_INCREF(x);
  } else {
    z = object_call(x, opcode_map(OP_ADD), y);
  }
}

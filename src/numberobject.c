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

#include "object.h"
#include "intobject.h"
#include "floatobject.h"

static MethodDef number_methods[]= {
  {"bytevalue",   NULL, "b", NULL, 1},
  {"intvalue",    NULL, "i", NULL, 1},
  {"floatvalue",  NULL, "f", NULL, 1},
  {NULL}
};

TypeObject number_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name    = "Number",
  .flags   = TPFLAGS_TRAIT,
  .methods = number_methods,
};

void init_number_type(void)
{
  number_type.desc = desc_from_klass("lang", "Number");
  if (type_ready(&number_type) < 0)
    panic("Cannot initalize 'Number' type.");
}

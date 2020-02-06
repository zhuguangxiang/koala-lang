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

#include "methodobject.h"

TypeObject error_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name  = "Error",
  .flags = TPFLAGS_ENUM,
};

void init_error_type(void)
{
  error_type.desc = desc_from_klass("lang", "Error");
  TypeDesc *ret = desc_from_str;
  TypeDesc *desc = desc_from_proto(NULL, ret);
  Object *proto = proto_new("error", desc);
  type_add_ifunc(&error_type, proto);
  OB_DECREF(proto);
  TYPE_DECREF(desc);
  TYPE_DECREF(ret);

  if (type_ready(&error_type) < 0)
    panic("Cannot initalize 'Error' type.");
}

void fini_error_type(void)
{
  type_fini(&error_type);
}

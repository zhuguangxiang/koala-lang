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

static Object *disassemble(Object *self, Object *args)
{

}

static MethodDef lang_methods[] = {
  {"disassemble", "A", "s", disassemble},
  {NULL}
};

void init_lang_module(void)
{
  Object *m = Module_New("lang");
  Module_Add_Type(m, &any_type);
  Module_Add_Type(m, &byte_type);
  Module_Add_Type(m, &integer_type);
  Module_Add_Type(m, &bool_type);
  Module_Add_Type(m, &string_type);
  Module_Add_Type(m, &char_type);
  Module_Add_Type(m, &float_type);
  Module_Add_Type(m, &array_type);
  Module_Add_Type(m, &tuple_type);
  Module_Add_Type(m, &map_type);
  Module_Add_Type(m, &field_type);
  Module_Add_Type(m, &method_type);
  Module_Add_Type(m, &proto_type);
  Module_Add_Type(m, &class_type);
  Module_Add_Type(m, &module_type);
  Module_Add_Type(m, &code_type);
  Module_Add_FuncDefs(m, lang_methods);
  Module_Install("lang", m);
  OB_DECREF(m);
}

void fini_lang_module(void)
{
  Module_Uninstall("lang");
}

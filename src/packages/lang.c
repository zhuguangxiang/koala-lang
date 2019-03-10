/*
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "lang.h"

void Init_Lang_Package(Package *pkg)
{
  Init_Klass_Klass();
  Init_String_Klass();
  Init_Integer_Klass();
  Init_Tuple_Klass();
  Pkg_Add_Class(pkg, &Klass_Klass);
  Pkg_Add_Class(pkg, &Any_Klass);
  Pkg_Add_Class(pkg, &String_Klass);
  Pkg_Add_Class(pkg, &Int_Klass);
  Pkg_Add_Class(pkg, &Tuple_Klass);
}

void Fini_Lang_Package(void)
{
  Fini_Tuple_Klass();
  Fini_Integer_Klass();
  Fini_String_Klass();
  Fini_Klass_Klass();
}

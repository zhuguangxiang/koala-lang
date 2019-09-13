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
#include <string.h>
#include "atom.h"

int main(int argc, char *argv[])
{
  init_atom();

  char *s = atom("Hello,");
  expect(!strcmp(s, "Hello,"));
  char *s2 = atom_nstring(" Koala", 6);
  expect(!strcmp(s2, " Koala"));
  char *s3 = atom("Hello,");
  expect(s == s3);
  char *s4 = atom_nstring(" Koala", 6);
  expect(s2 == s4);
  char *s5 = atom_vstring(2, s, s2);
  expect(!strcmp(s5, "Hello, Koala"));

  fini_atom();
  return 0;
}

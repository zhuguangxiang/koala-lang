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
#include <stdio.h>
#include "utf8.h"

void test_utf16(void)
{
  int res;
  wchar buf[4];
  int unicode = 0x10437;
  res = encode_one_utf16_char(unicode, buf);
  expect(res == 2);
  expect(buf[0].val == 0xd801);
  expect(buf[1].val == 0xdc37);
  wchar *p = buf;
  int ch = decode_one_utf16_char(&p);
  expect(ch == unicode);
  expect(p == &buf[2]);
}

void test_utf8(void)
{
  char han[] = "汉";
  char *p = han;
  int unicode = decode_one_utf8_char(&p);
  expect(unicode == 0x6c49);
  char buf[4];
  int res = encode_one_utf8_char(unicode, buf);
  expect(res == 3);
  expect(buf[0] == han[0]);
  expect(buf[1] == han[1]);
  expect(buf[2] == han[2]);
  expect(buf[0] == (char)0xe6);
  expect(buf[1] == (char)0xb1);
  expect(buf[2] == (char)0x89);
}

void test_u8to16(void)
{
  char han[] = "汉";
  char *p = han;
  int ch = decode_one_utf8_char(&p);
  wchar buf[4];
  int res = encode_one_utf16_char(ch, buf);
  expect(res == 1);
  expect(buf[0].val == 0x6c49);
}

void test_u16to8(void)
{
  char han[] = "汉";
  char *p = han;
  int ch = decode_one_utf8_char(&p);
  wchar buf[4];
  int res = encode_one_utf16_char(ch, buf);
  expect(res == 1);
  expect(buf[0].val == 0x6c49);
  wchar *wp = buf;
  ch = decode_one_utf16_char(&wp);
  expect(ch == 0x6c49);
  char buf2[4];
  res = encode_one_utf8_char(ch, buf2);
  expect(res == 3);
  expect(buf2[0] == (char)0xe6);
  expect(buf2[1] == (char)0xb1);
  expect(buf2[2] == (char)0x89);
}

int main(int argc, char *argv[])
{
  test_utf16();
  test_utf8();
  test_u8to16();
  test_u16to8();
  return 0;
}

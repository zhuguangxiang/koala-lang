/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "utf8.h"

void test_utf16(void)
{
  int res;
  wchar buf[4];
  int unicode = 0x10437;
  res = encode_one_utf16_char(unicode, buf);
  assert(res == 2);
  assert(buf[0].val == 0xd801);
  assert(buf[1].val == 0xdc37);
  wchar *p = buf;
  int ch = decode_one_utf16_char(&p);
  assert(ch == unicode);
  assert(p == &buf[2]);
}

void test_utf8(void)
{
  char han[] = "汉";
  char *p = han;
  int unicode = decode_one_utf8_char(&p);
  assert(unicode == 0x6c49);
  char buf[4];
  int res = encode_one_utf8_char(unicode, buf);
  assert(res == 3);
  assert(buf[0] == han[0]);
  assert(buf[1] == han[1]);
  assert(buf[2] == han[2]);
  assert(buf[0] == (char)0xe6);
  assert(buf[1] == (char)0xb1);
  assert(buf[2] == (char)0x89);
}

void test_u8to16(void)
{
  char han[] = "汉";
  char *p = han;
  int ch = decode_one_utf8_char(&p);
  wchar buf[4];
  int res = encode_one_utf16_char(ch, buf);
  assert(res == 1);
  assert(buf[0].val == 0x6c49);
}

void test_u16to8(void)
{
  char han[] = "汉";
  char *p = han;
  int ch = decode_one_utf8_char(&p);
  wchar buf[4];
  int res = encode_one_utf16_char(ch, buf);
  assert(res == 1);
  assert(buf[0].val == 0x6c49);
  wchar *wp = buf;
  ch = decode_one_utf16_char(&wp);
  assert(ch == 0x6c49);
  char buf2[4];
  res = encode_one_utf8_char(ch, buf2);
  assert(res == 3);
  assert(buf2[0] == (char)0xe6);
  assert(buf2[1] == (char)0xb1);
  assert(buf2[2] == (char)0x89);
}

int main(int argc, char *argv[])
{
  test_utf16();
  test_utf8();
  test_u8to16();
  test_u16to8();
  return 0;
}

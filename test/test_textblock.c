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

#include "textblock.h"
#include <string.h>
#include "log.h"

static void test_1(void)
{
  reset_textblock();
  char buf[16] = {0};
  textblock_input("\"\"\"\n", buf);
  expect(!strcmp(buf, "\"\"\""));
  int status = textblock_state();
  expect(status == TB_CONT);
  char *data = textblock_data();
  expect(!strcmp(data, "\n"));
}

static void test_2(void)
{
  reset_textblock();
  char buf[16] = {0};
  textblock_input("\"\"\"foo\n", buf);
  expect(!strcmp(buf, "\"\"\""));
  int status = textblock_state();
  expect(status == TB_CONT);
  char *data = textblock_data();
  expect(!strcmp(data, "foo\n"));
}

static void test_3(void)
{
  reset_textblock();
  char buf[16] = {0};
  textblock_input("\"\"\"foo\"\"\"\n", buf);
  expect(!strcmp(buf, "\"\"\"\"\"\"\n"));
  int status = textblock_state();
  expect(status == TB_END);
  char *data = textblock_data();
  expect(!strcmp(data, "foo"));
}

static void test_4(void)
{
  reset_textblock();
  char buf[16] = {0};
  textblock_input("\"\"\"foo\"\"\"1+2\n", buf);
  expect(!strcmp(buf, "\"\"\"\"\"\"1+2\n"));
  int status = textblock_state();
  expect(status == TB_END);
  char *data = textblock_data();
  expect(!strcmp(data, "foo"));
}

static void test_5(void)
{
  reset_textblock();
  char buf[16] = {0};
  textblock_input("\"\"\"foo\nbar\n\"\"\"", buf);
  expect(!strcmp(buf, "\"\"\"\"\"\""));
  int status = textblock_state();
  expect(status == TB_END);
  char *data = textblock_data();
  expect(!memcmp(data, "foo\nbar\n", 8));
}

static void test_6(void)
{
  reset_textblock();
  char buf[64] = {0};
  textblock_input("io.println(\"\"\"Option.Some(1,\"foo\")\"\"\")", buf);
  expect(!strcmp(buf, "io.println(\"\"\"\"\"\")"));
  int status = textblock_state();
  expect(status == TB_END);
  char *data = textblock_data();
  expect(!strcmp(data, "Option.Some(1,\"foo\")"));
}

static void test_7(void)
{
  char *textblock = "\"\"\"\
{\
\"name\": \"jacky\",\
\"age\": 123\
}\
\"\"\"";

  reset_textblock();
  char buf[1024] = {0};
  textblock_input(textblock, buf);
  expect(!strcmp(buf, "\"\"\"\"\"\""));
  int status = textblock_state();
  expect(status == TB_END);
  char *data = textblock_data();
  printf("%s\n", data);
}

static void test_8(void)
{
  char *textblock = "\"\"\"\n";
  reset_textblock();
  char buf[1024] = {0};
  textblock_input(textblock, buf);
  textblock = "abc\n";
  textblock_input(textblock, buf);
  textblock = "\"hello\"\n";
  textblock_input(textblock, buf);
  textblock = "\"\"\"\n";
  textblock_input(textblock, buf);
}

int main(int argc, char *argv[])
{
  test_1();
  test_2();
  test_3();
  test_4();
  test_5();
  test_6();
  test_7();
  test_8();
  return 0;
}

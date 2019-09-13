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
#include <time.h>
#include "bytebuffer.h"

static void random_string(char *data, int len)
{
  static const char char_set[] = "0123456789abcdefghijklmnopqrstuvwxyz" \
    "ABCDEFGHIJKLMNOPQRSTUWXYZ";
  int i;
  int idx;

  for (i = 0; i < len; ++i) {
    idx = rand() % (sizeof(char_set)/sizeof(char_set[0]) - 1);
    data[i] = char_set[idx];
  }
}

static char saved[128];

int main(int argc, char *argv[])
{
  ByteBuffer buffer;
  char data[10];

  bytebuffer_init(&buffer, 16);
  srand(time(NULL));

  for (int i = 0; i < 10; ++i) {
    random_string(data, 10);
    memcpy(saved + i * 10, data, 10);
    bytebuffer_write(&buffer, data, 10);
  }
  expect(vector_size(&buffer.vec) == 7);
  expect(buffer.total == 100);

  char a = 'A';
  bytebuffer_write_byte(&buffer, a);
  saved[100] = a;
  short b = 0xbeaf;
  bytebuffer_write_2bytes(&buffer, b);
  *(short *)(saved + 101) = b;
  int c = 0xdeadbeaf;
  bytebuffer_write_4bytes(&buffer, c);
  *(int *)(saved + 103) = c;
  expect(vector_size(&buffer.vec) == 7);
  expect(buffer.total == 107);

  char *buf = NULL;
  int size = bytebuffer_toarr(&buffer, &buf);
  expect(buf && size == 107);
  expect(!memcmp(saved, buf, 107));
  kfree(buf);

  bytebuffer_fini(&buffer);
  return 0;
}

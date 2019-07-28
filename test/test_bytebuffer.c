/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

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
  struct bytebuffer buffer;
  char data[10];

  bytebuffer_init(&buffer, 16);
  srand(time(NULL));

  for (int i = 0; i < 10; ++i) {
    random_string(data, 10);
    memcpy(saved + i * 10, data, 10);
    bytebuffer_write(&buffer, data, 10);
  }
  assert(vector_size(&buffer.vec) == 7);
  assert(buffer.total == 100);

  char a = 'A';
  bytebuffer_write_byte(&buffer, a);
  saved[100] = a;
  short b = 0xbeaf;
  bytebuffer_write_2bytes(&buffer, b);
  *(short *)(saved + 101) = b;
  int c = 0xdeadbeaf;
  bytebuffer_write_4bytes(&buffer, c);
  *(int *)(saved + 103) = c;
  assert(vector_size(&buffer.vec) == 7);
  assert(buffer.total == 107);

  char *buf = NULL;
  int size = bytebuffer_toarr(&buffer, &buf);
  assert(buf && size == 107);
  assert(!memcmp(saved, buf, 107));
  kfree(buf);

  bytebuffer_free(&buffer);
  return 0;
}

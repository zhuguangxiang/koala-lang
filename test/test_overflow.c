/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include <stdio.h>

int main(int argc, char *argv[])
{
  short a, b;
  short r;

  a = 32767;
  b = 10;
  r = a + b;
  if (r < 0)
    printf("%d + %d = %d overflow\n", a, b, r);

  a = -32768;
  b = 10;
  r = a - b;
  if (r > 0)
    printf("%d - %d = %d overflow\n", a, b, r);
  r = b - a;
  if (r < 0)
    printf("%d - %d = %d overflow\n", b, a, r);


  a = 32767;
  b = 32766;
  r = (unsigned short)a + b;
  if (r < 0)
    printf("%d + %d = %d overflow\n", a, b, r);
  return 0;
}
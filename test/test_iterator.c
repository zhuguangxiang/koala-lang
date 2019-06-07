/*
MIT License

Copyright (c) 2018 James, https://github.com/zhuguangxiang

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <assert.h>
#include "iterator.h"

static int number_next(struct iterator *iter)
{
  int *ptr = iter->iterable;
  iter->current = ptr + iter->index;
  iter->index++;
  if (iter->index > 100)
    return 0;
  return 1;
}

int main(int argc, char *argv[])
{
  int numbers[100];
  for (int i = 0; i < 100; i++)
    numbers[i] = 1001 + i;

  ITERATOR(iter, numbers, number_next);
  int j = 0;
  int val;
  iter_for_each_as(iter, int, val) {
    assert(val == numbers[j]);
    ++j;
  }
  assert(j == 100);

  iter_reset(iter);
  j = 0;
  int *pval;
  iter_for_each(iter, pval) {
    assert(*pval == numbers[j]);
    ++j;
  }
  assert(j == 100);
}

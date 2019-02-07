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

#include "stringex.h"
#include "mem.h"

/*
 * Note: This function returns a pointer to a substring of the original string.
 * The original string must NOT be literal string,
 * because the original string maybe changed.
 * The return value must NOT be deallocated using free() etc.
 */
char *string_trim(char *str)
{
  char *end;

  /* trim leading space */
  while (isspace((int)*str))
    str++;

  if (*str == 0)  /* all spaces? */
    return str;

  /* trim trailing space */
  end = str + strlen(str) - 1;
  while (end > str && isspace((int)*end))
    end--;

  /* write new null terminator character */
  end[1] = '\0';

  return str;
}

int string_to_int64(char *str, int64 *rval)
{
  char ch;
  char ch1;
  int64 val;
  char *s;
  char *buf = string_dup(str);

  str = string_trim(buf);
  ch = str[0];
  ch1 = str[1];

  if (ch == '0' && (ch1 == 'x' || ch1 == 'X')) {
    s = str + 2;
    while ((ch = *s++)) {
      if (!((ch >= '0' && ch <= '9') ||
            (ch >= 'a' && ch <= 'f') ||
            (ch >= 'A' && ch < 'F'))) {
        Mfree(buf);
        return -1;
      }
    }
    val = strtoll(str, NULL, 16);
  } else if (ch == '0') {
    s = str + 1;
    while ((ch = *s++)) {
      if (!(ch >= '0' && ch <= '7')) {
        Mfree(buf);
        return -1;
      }
    }
    val = strtoll(str, NULL, 8);
  }
  else {
    s = str;
    while ((ch = *s++)) {
      if (!(ch >= '0' && ch <= '9')) {
        Mfree(buf);
        return -1;
      }
    }
    val = strtoll(str, NULL, 10);
  }

  Mfree(buf);
  *rval = val;
  return 0;
}

char *string_dup(char *str)
{
  char *s = Malloc(strlen(str) + 1);
  strcpy(s, str);
  return s;
}

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

#include <stddef.h>
#include <string.h>
#include "common.h"

/*
  https://en.wikipedia.org/wiki/UTF-16
  https://www.fileformat.info/info/unicode/utf8.htm
  https://github.com/zhuguangxiang/git/blob/master/utf8.c
  http://tidy.sourceforge.net/cgi-bin/lxr/source/src/utf8.c
 */
int decode_one_utf8_char(char **start)
{
  unsigned char *s = (unsigned char *)*start;
  int ch;
  int incr;

  if (*s < 0x80) {
    /* 0xxx_xxxx */
    ch = *s;
    incr = 1;
  } else if ((s[0] & 0xe0) == 0xc0) {
    /* 110X_XXXx 10xx_xxxx */
    if ((s[1] & 0xc0) != 0x80 ||
        (s[0] & 0xfe) == 0xc0)
      goto invalid;
    ch = ((s[0] & 0x1f) << 6) | (s[1] & 0x3f);
    incr = 2;
  } else if ((s[0] & 0xf0) == 0xe0) {
		/* 1110_XXXX 10Xx_xxxx 10xx_xxxx */
		if ((s[1] & 0xc0) != 0x80 ||
		    (s[2] & 0xc0) != 0x80 ||
		    /* overlong? */
		    (s[0] == 0xe0 && (s[1] & 0xe0) == 0x80) ||
		    /* surrogate? */
		    (s[0] == 0xed && (s[1] & 0xe0) == 0xa0) ||
		    /* U+FFFE or U+FFFF? */
		    (s[0] == 0xef && s[1] == 0xbf && (s[2] & 0xfe) == 0xbe))
			goto invalid;
		ch = ((s[0] & 0x0f) << 12) | ((s[1] & 0x3f) << 6) | (s[2] & 0x3f);
		incr = 3;
	} else if ((s[0] & 0xf8) == 0xf0) {
		/* 1111_0XXX 10XX_xxxx 10xx_xxxx 10xx_xxxx */
		if ((s[1] & 0xc0) != 0x80 ||
		    (s[2] & 0xc0) != 0x80 ||
		    (s[3] & 0xc0) != 0x80 ||
		    /* overlong? */
		    (s[0] == 0xf0 && (s[1] & 0xf0) == 0x80) ||
		    /* > U+10FFFF? */
		    (s[0] == 0xf4 && s[1] > 0x8f) ||
        s[0] > 0xf4)
			goto invalid;
		ch = ((s[0] & 0x07) << 18) | ((s[1] & 0x3f) << 12) |
			   ((s[2] & 0x3f) << 6) | (s[3] & 0x3f);
		incr = 4;
	} else {
invalid:
    *start = NULL;
    return 0;
  }

  *start += incr;
  return ch;
}

int encode_one_utf8_char(int ch, char *buf)
{
  int bytes;

  if (ch <= 0x7f) {
    /* 0xxx_xxxx */
    buf[0] = (char)ch;
    bytes = 1;
  } else if (ch <= 0x7ff) {
    /* 110x_xxxx */
    buf[0] = (char)(0xc0 | (ch >> 6));
    buf[1] = (char)(0x80 | (ch & 0x3f));
    bytes = 2;
  } else if (ch <= 0xffff) {
    /* 1110_xxxx */
    buf[0] = (char)(0xe0 | (ch >> 12));
    buf[1] = (char)(0x80 | ((ch >> 6) & 0x3f));
    buf[2] = (char)(0x80 | (ch & 0x3f));
    bytes = 3;
    if (ch == 0xfffe || ch == 0xffff) {
      /* not a valid utf8 character */
      bytes = 0;
    }
  } else if (ch <= 0x1fffff) {
    /* 1111_0xxx */
    buf[0] = (char)(0xf0 | (ch >> 18));
    buf[1] = (char)(0x80 | ((ch >> 12) & 0x3f));
    buf[2] = (char)(0x80 | ((ch >> 6) & 0x3f));
    buf[3] = (char)(0x80 | (ch & 0x3f));
    bytes = 4;
    if (ch > 0x10ffff) {
      /* beyond max UCS4 */
      bytes = 0;
    }
  } else {
    /* too big */
    bytes = 0;
  }

  return bytes;
}

int get_one_utf8_size(char *str)
{
  unsigned char *s = (unsigned char *)str;
  int count;

  if (*s < 0x80) {
    /* 0xxx_xxxx */
    s += 1;
    ++count;
  } else if ((s[0] & 0xe0) == 0xc0) {
    /* 110X_XXXx 10xx_xxxx */
    if ((s[1] & 0xc0) != 0x80 ||
        (s[0] & 0xfe) == 0xc0)
      goto invalid;
    s += 2;
    ++count;
  } else if ((s[0] & 0xf0) == 0xe0) {
    /* 1110_XXXX 10Xx_xxxx 10xx_xxxx */
    if ((s[1] & 0xc0) != 0x80 ||
        (s[2] & 0xc0) != 0x80 ||
        /* overlong? */
        (s[0] == 0xe0 && (s[1] & 0xe0) == 0x80) ||
        /* surrogate? */
        (s[0] == 0xed && (s[1] & 0xe0) == 0xa0) ||
        /* U+FFFE or U+FFFF? */
        (s[0] == 0xef && s[1] == 0xbf && (s[2] & 0xfe) == 0xbe))
      goto invalid;
    s += 3;
    ++count;
  } else if ((s[0] & 0xf8) == 0xf0) {
    /* 1111_0XXX 10XX_xxxx 10xx_xxxx 10xx_xxxx */
    if ((s[1] & 0xc0) != 0x80 ||
        (s[2] & 0xc0) != 0x80 ||
        (s[3] & 0xc0) != 0x80 ||
        /* overlong? */
        (s[0] == 0xf0 && (s[1] & 0xf0) == 0x80) ||
        /* > U+10FFFF? */
        (s[0] == 0xf4 && s[1] > 0x8f) ||
        s[0] > 0xf4)
      goto invalid;
    s += 4;
    ++count;
  } else {
invalid:
    return -1;
  }

  return count;
}

int check_utf8_valid_with_len(void *str, int len)
{
  unsigned char *s = (unsigned char *)str;
  unsigned char *end = s + len;
  int count = 0;

  while (s < end) {
    if (*s < 0x80) {
      /* 0xxx_xxxx */
      s += 1;
      ++count;
    } else if ((s[0] & 0xe0) == 0xc0) {
      /* 110X_XXXx 10xx_xxxx */
      if ((s[1] & 0xc0) != 0x80 ||
          (s[0] & 0xfe) == 0xc0)
        goto invalid;
      s += 2;
      ++count;
    } else if ((s[0] & 0xf0) == 0xe0) {
      /* 1110_XXXX 10Xx_xxxx 10xx_xxxx */
      if ((s[1] & 0xc0) != 0x80 ||
		      (s[2] & 0xc0) != 0x80 ||
		      /* overlong? */
		      (s[0] == 0xe0 && (s[1] & 0xe0) == 0x80) ||
		      /* surrogate? */
		      (s[0] == 0xed && (s[1] & 0xe0) == 0xa0) ||
		      /* U+FFFE or U+FFFF? */
		      (s[0] == 0xef && s[1] == 0xbf && (s[2] & 0xfe) == 0xbe))
			  goto invalid;
      s += 3;
      ++count;
    } else if ((s[0] & 0xf8) == 0xf0) {
		  /* 1111_0XXX 10XX_xxxx 10xx_xxxx 10xx_xxxx */
      if ((s[1] & 0xc0) != 0x80 ||
          (s[2] & 0xc0) != 0x80 ||
          (s[3] & 0xc0) != 0x80 ||
          /* overlong? */
          (s[0] == 0xf0 && (s[1] & 0xf0) == 0x80) ||
          /* > U+10FFFF? */
          (s[0] == 0xf4 && s[1] > 0x8f) ||
          s[0] > 0xf4)
        goto invalid;
     s += 4;
     ++count;
    } else {
invalid:
      return -1;
    }
  }

  return count;
}

// return count of utf8 characters, or -1 if not valid.
int check_utf8_valid(void *str)
{
  return check_utf8_valid_with_len(str, strlen(str));
}

int decode_one_utf16_char(wchar **start)
{
  wchar *wch = *start;
  int high = (int)wch->val;
  ++wch;
  if ((high & 0xd800) == 0xd800) {
    /* 4 bytes */
    int low = (int)wch->val;
    *start = ++wch;
    return ((high - 0xd800) << 10) + (low - 0xdc00) + 0x10000;
  } else {
    /* 2 bytes */
    *start = wch;
    return high;
  }
}

int encode_one_utf16_char(int ch, wchar *buf)
{
  if (ch <= 0xffff) {
    /* 2 bytes */
    buf[0].val = (unsigned short)ch;
    return 1;
  } else {
    /* 4 bytes */
    buf[0].val = ((ch - 0x10000) >> 10) + 0xd800;
    buf[1].val = ((ch - 0x10000) & 0x3ff) + 0xdc00;
    return 2;
  }
}

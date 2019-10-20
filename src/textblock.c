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

#include "strbuf.h"
#include "log.h"
#include "textblock.h"

typedef struct textblockstate {
  int status;
  StrBuf buf;
} TextBlockState;

static TextBlockState tbs;

int textblock_state(void)
{
  return tbs.status;
}

char *textblock_data(void)
{
  return strbuf_tostr(&tbs.buf);
}

void reset_textblock(void)
{
  strbuf_fini(&tbs.buf);
  memset(&tbs, 0, sizeof(TextBlockState));
}

#define TEXT_FLAG "\"\"\""

/*
  single line:
    XXX """ YYY """ (;) ZZZ
  multi line:
    XXX """ YYY
    ... ... ...
    ... ... ...
    XXX """ (;) YYY
  handle pattern:
    XXX """
 */

// return 0: not handled.
// return 1: handled.
int textblock_input(char *line, char *buf)
{
  if (line == NULL || strlen(line) == 0) {
    tbs.status = TB_END;
    return 1;
  }

  buf[0] = 0;

  char *text = strstr(line, TEXT_FLAG);
  int status = tbs.status;
  switch (status) {
  case TB_END: {
    if (text != NULL) {
      // copy left text with """
      int len = text - line + 3;
      memcpy(buf, line, len);
      tbs.status = TB_CONT;
      return textblock_input(text + 3, buf + len);
    } else {
      return 0;
    }
  }
  case TB_CONT: {
    if (text == NULL) {
      strbuf_append(&tbs.buf, line);
    } else {
      // copy left text with """
      int len = text - line;
      strbuf_nappend(&tbs.buf, line, len);
      strcpy(buf, text);
      tbs.status = TB_END;
    }
    return 1;
  }
  default:
    panic("invalid textblock status");
    break;
  }
}

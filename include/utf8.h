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

#ifndef _KOALA_UTF8_H_
#define _KOALA_UTF8_H_

#include "common.h"

#ifdef  __cplusplus
extern "C" {
#endif

int decode_one_utf8_char(char **start);
int encode_one_utf8_char(int unicode, char *buf);

int decode_one_utf16_char(wchar **start);
int encode_one_utf16_char(int unicode, wchar *buf);

// return count of utf8 characters, or -1 if not valid.
int check_utf8_valid(void *str);
int check_utf8_valid_with_len(void *str, int len);
int get_one_utf8_size(char *str);

#ifdef  __cplusplus
}
#endif

#endif /* _KOALA_UTF8_H_ */

/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
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

#ifdef  __cplusplus
}
#endif

#endif /* _KOALA_UTF8_H_ */

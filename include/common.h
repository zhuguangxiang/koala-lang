/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_COMMON_H_
#define _KOALA_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef union {
  unsigned char data[2];
  unsigned short val;
} wchar;

/* Get the min(max) one of the two numbers */
#define MIN(a, b) ((a) > (b) ? (b) : (a))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/* Count the number of elements in an array */
#define COUNT_OF(arr) ((int)(sizeof(arr) / sizeof((arr)[0])))

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_COMMON_H_ */

/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_COMMON_H_
#define _KOALA_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

/* unicode character */
typedef union wchar {
  unsigned char data[2];
  unsigned short val;
} wchar;

/* Get the min(max) one of the two numbers */
#define MIN(a, b) ((a) > (b) ? (b) : (a))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/* Count the number of elements in an array */
#define COUNT_OF(arr) ((int)(sizeof(arr) / sizeof((arr)[0])))

/*
 * Free function to free each item.
 */
typedef void (*freefunc)(void *, void *);

/*
 * Compare function to test two keys for equality.
 * Returns 0 if the two entries are equal,
 * -1 if a < b and 1 if a > b.
 */
typedef int (*cmpfunc)(void *, void *);

/*
 * Hash function to generate hash code for hash map.
 * strhash() and memhash() -> hashmap.h
 */
typedef unsigned int (*hashfunc)(void *);

/*
 * Compare function to test two keys for equality.
 * Returns 1 if the two entries are equal.
 */
typedef int (*equalfunc)(void *, void *);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_COMMON_H_ */

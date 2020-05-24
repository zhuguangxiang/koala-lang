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

/* endian check */
#define CHECK_BIG_ENDIAN ({   \
  int i = 1; !*((char *)&i);  \
})

/* Get the min(max) one of the two numbers */
#define MIN(a, b) ((a) > (b) ? (b) : (a))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/* Count the number of elements in an array */
#define COUNT_OF(arr) ((int)(sizeof(arr) / sizeof((arr)[0])))

/* pointer to integer */
#define PTR2INT(p) ((int)(intptr_t)(p))

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

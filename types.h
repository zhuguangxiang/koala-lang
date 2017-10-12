/*
 * types.h - define basic types and useful micros.
 */
#ifndef _KOALA_TYPES_H_
#define _KOALA_TYPES_H_

/* stddef.h - standard type definitions */
#include <stddef.h>

/*
 * http://www.gnu.org/software/libc/manual/html_node/Integers.html
 */
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef float   float32_t;
typedef double  float64_t;

/* Get the min(max) one of the two numbers */
#define min(a, b) ((a) > (b) ? (b) : (a))
#define max(a, b) ((a) > (b) ? (a) : (b))

/* Count the number of elements in an array. */
#define nr_elts(arr)  (sizeof(arr) / sizeof((arr)[0]))

/* Get the struct address from its member's address */
#define container_of(ptr, type, member) \
  ((type *)((char *)ptr - offsetof(type, member)))

#define TYPE_CHAR   1
#define TYPE_BYTE   2
#define TYPE_INT    3
#define TYPE_FLOAT  4
#define TYPE_BOOL   5
#define TYPE_STRING 6

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_TYPES_H_ */

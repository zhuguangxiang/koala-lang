/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_STRING_OBJECT_H_
#define _KOALA_STRING_OBJECT_H_

#include <inttypes.h>
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct stringobject {
  OBJECT_HEAD
  /* unicode length */
  int len;
  char *atom;
} StringObject;

typedef struct charobject {
  OBJECT_HEAD
  uint32_t value;
} CharObject;

extern Klass string_type;
void init_stringobject(void);
void fini_stringobject(void);
Object *new_string(char *str);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_STRING_OBJECT_H_ */

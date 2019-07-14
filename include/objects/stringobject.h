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

struct string_object {
  OBJECT_HEAD
  /* unicode length */
  int len;
  char *atom;
};

struct char_object {
  OBJECT_HEAD
  uint32_t value;
};

extern struct klass string_type;
void init_string_type(void);
void free_string_type(void);
struct object *new_string(char *str);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_STRING_OBJECT_H_ */

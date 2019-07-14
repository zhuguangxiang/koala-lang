/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_MAP_OBJECT_H_
#define _KOALA_MAP_OBJECT_H_

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

extern struct klass string_type;
void init_string_type(void);
void free_string_type(void);
struct object *new_string(char *str);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_MAP_OBJECT_H_ */

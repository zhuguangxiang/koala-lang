/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_CODE_OBJECT_H_
#define _KOALA_CODE_OBJECT_H_

#include <inttypes.h>
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

struct codeinfo {
  struct vector locvec;
  struct object *consts;
  int size;
  uint8_t codes[0];
};

#define CODE_KCODE 0
#define CODE_CFUNC 1

struct code_object {
  OBJECT_HEAD
  struct typedesc *proto;
  int kind;
  union {
    cfunc_t cfunc;
    struct codeinfo *kcode;
  };
};

extern struct klass code_type;
void init_code_type(void);
void free_code_type(void);
struct object *code_from_cfunc(struct cfuncdef *f);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_CODE_OBJECT_H_ */

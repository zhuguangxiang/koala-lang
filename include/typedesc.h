/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_TYPEDESC_H_
#define _KOALA_TYPEDESC_H_

#include <assert.h>
#include "strbuf.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TYPE_BYTE  'b'
#define TYPE_CHAR  'c'
#define TYPE_INT   'i'
#define TYPE_FLOAT 'f'
#define TYPE_BOOL  'z'
#define TYPE_STR   's'
#define TYPE_ANY   'A'
#define TYPE_KLASS 128
#define TYPE_PROTO 129

#define TYPEDESC_HEAD \
  int kind; int refcnt;

struct typedesc {
  TYPEDESC_HEAD
};

#define TYPE_INCREF(_desc_) \
({                          \
  if (_desc_)               \
    ++(_desc_)->refcnt;     \
  _desc_;                   \
})

#define TYPE_DECREF(_desc_)    \
({                             \
  struct typedesc *_d_ =       \
  (struct typedesc *)(_desc_); \
  if (_d_) {                   \
    --_d_->refcnt;             \
    assert(_d_->refcnt >= 0);  \
    if (_d_->refcnt <= 0)      \
      typedesc_free(_d_);      \
  }                            \
})

void typedesc_destroy(void);
void typedesc_free(struct typedesc *desc);
void typedesc_tostr(struct typedesc *desc, struct strbuf *buf);
int typedesc_equal(struct typedesc *desc1, struct typedesc *desc2);
struct typedesc **typestr_toarr(char *s);

struct basedesc {
  TYPEDESC_HEAD
};

struct klassdesc {
  TYPEDESC_HEAD
  char **pathes;
  char *type;
};

struct protodesc {
  TYPEDESC_HEAD
  struct typedesc **args;
  struct typedesc *ret;
};

struct typedesc *get_basedesc(int kind);
struct typedesc *new_klassdesc(char **pathes, char *type);
struct typedesc *new_protodesc(struct typedesc **args, struct typedesc *ret);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_TYPEDESC_H_ */

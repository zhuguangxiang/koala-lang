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

typedef struct typedesc {
  TYPEDESC_HEAD
} TypeDesc;

#define TYPE_INCREF(_desc_) \
({                          \
  if (_desc_)               \
    ++(_desc_)->refcnt;     \
  _desc_;                   \
})

#define TYPE_DECREF(_desc_)             \
({                                      \
  TypeDesc *_d_ = (TypeDesc *)(_desc_); \
  if (_d_) {                            \
    --_d_->refcnt;                      \
    assert(_d_->refcnt >= 0);           \
    if (_d_->refcnt <= 0)               \
      typedesc_free(_d_);               \
  }                                     \
})

void init_typedesc(void);
void fini_typedesc(void);
void typedesc_free(TypeDesc *desc);
void typedesc_tostr(TypeDesc *desc, struct strbuf *buf);
int typedesc_equal(TypeDesc *desc1, TypeDesc *desc2);
TypeDesc **typestr_toarr(char *s);

typedef struct basedesc {
  TYPEDESC_HEAD
} BaseDesc;

typedef struct klassdesc {
  TYPEDESC_HEAD
  char **pathes; /* null-terminated array */
  char *type;
} KlassDesc;

typedef struct protodesc {
  TYPEDESC_HEAD
  TypeDesc **args; /* null-terminated array */
  TypeDesc *ret;
} ProtoDesc;

TypeDesc *get_basedesc(int kind);
TypeDesc *new_klassdesc(char **pathes, char *type);
TypeDesc *new_protodesc(TypeDesc **args, TypeDesc *ret);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_TYPEDESC_H_ */

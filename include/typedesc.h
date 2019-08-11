/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_TYPEDESC_H_
#define _KOALA_TYPEDESC_H_

#include <inttypes.h>
#include "common.h"
#include "vector.h"
#include "strbuf.h"
#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BASE_BYTE  'b'
#define BASE_CHAR  'c'
#define BASE_INT   'i'
#define BASE_FLOAT 'f'
#define BASE_BOOL  'z'
#define BASE_STR   's'
#define BASE_ANY   'A'

/* constant value */
typedef struct constvalue {
  /* see BASE_XXX */
  char kind;
  union {
    wchar cval;
    int64_t ival;
    double fval;
    int bval;
    char *str;
  };
} ConstValue;

void Const_Show(ConstValue *val, StrBuf *buf);

typedef enum desckind {
  TYPE_BASE = 1,
  TYPE_KLASS,
  TYPE_PROTO,
  TYPE_ARRAY,
  TYPE_MAP,
  TYPE_VARG,
  TYPE_TUPLE,
  TYPE_PARAREF,
} DescKind;

/*
 * Type descriptor
 * Klass: Lio.File;
 * Array: [s
 * Map: Mss
 * Proto: Pis:e;
 * Varg: ...s
 */
#define TYPEDESC_HEAD \
  DescKind kind; int refcnt;

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
    panic(_d_->refcnt < 0,              \
          "type of '%d' refcnt error",  \
          _d_->kind);                   \
    if (_d_->refcnt <= 0) {             \
      typedesc_free(_d_);               \
      (_desc_) = NULL;                  \
    }                                   \
  }                                     \
})

typedef struct basedesc {
  TYPEDESC_HEAD
  char type; /* one of BASE_XXX */
  char *str;
} BaseDesc;

typedef struct klassdesc {
  TYPEDESC_HEAD
  char *path;
  char *type;
} KlassDesc;

typedef struct protodesc {
  TYPEDESC_HEAD
  Vector *args;
  TypeDesc *ret;
} ProtoDesc;

void init_typedesc(void);
void fini_typedesc(void);
void typedesc_free(TypeDesc *desc);
void typedesc_tostring(TypeDesc *desc, StrBuf *buf);
int typedesc_equal(TypeDesc *desc1, TypeDesc *desc2);

TypeDesc *typedesc_getbase(int kind);
TypeDesc *typedesc_getklass(char *path, char *type);
TypeDesc *typedesc_getproto(Vector *args, TypeDesc *ret);
TypeDesc *string_totypedesc(char *s);
TypeDesc *string_toproto(char *ptype, char *rtype);
Vector *string_totypedescvec(char *s);
char *basedesc_str(int kind);
void typedesc_show(TypeDesc *desc);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_TYPEDESC_H_ */

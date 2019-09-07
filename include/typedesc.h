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
typedef struct literal {
  /* see BASE_XXX */
  char kind;
  union {
    wchar cval;
    int64_t ival;
    double fval;
    int bval;
    char *str;
  };
} Literal;

typedef struct typeparadef {
  char *name;
  Vector *types;
} TypeParaDef;

typedef enum desckind {
  TYPE_BASE = 1,
  TYPE_KLASS,
  TYPE_PROTO,
  TYPE_VARG,
  TYPE_PARAREF,
} DescKind;

/*
 * Type descriptor
 * Klass: Lio.File;
 * Proto: Pis:e
 * Varg: ...s
 * Array: [s
 * Map: Mss
 */
typedef struct typedesc {
  DescKind kind;
  int refcnt;
  Vector *typeparas;
  union {
    char base;  /* one of BASE_XXX */
    struct {
      char *path;
      char *type;
      Vector *types;
    } klass;
    struct {
      Vector *args;
      struct typedesc *ret;
    } proto;
    struct {
      struct typedesc *type;
    } varg;
    struct {
      char *name;
      int index;
    } pararef;
  };
} TypeDesc;

#define TYPE_INCREF(desc) ({ \
  if (desc)                  \
    ++(desc)->refcnt;        \
  desc;                      \
})

#define TYPE_DECREF(desc) ({             \
  if (desc) {                            \
    --(desc)->refcnt;                    \
    if ((desc)->refcnt < 0)              \
      panic("type of '%d' refcnt error", \
            (desc)->kind);               \
    if ((desc)->refcnt <= 0) {           \
      desc_free(desc);                   \
      (desc) = NULL;                     \
    }                                    \
  }                                      \
})

void init_typedesc(void);
void fini_typedesc(void);
void literal_show(Literal *val, StrBuf *sbuf);
TypeParaDef *new_typeparadef(char *name, Vector *types);
void desc_free(TypeDesc *desc);
void desc_tostr(TypeDesc *desc, StrBuf *buf);
char *base_str(int kind);
void desc_show(TypeDesc *desc);
int desc_check(TypeDesc *desc1, TypeDesc *desc2);

TypeDesc *desc_from_base(int kind);
#define desc_from_byte \
  desc_from_base(BASE_BYTE)
#define desc_from_int \
  desc_from_base(BASE_INT)
#define desc_from_float \
  desc_from_base(BASE_FLOAT)
#define desc_from_char \
  desc_from_base(BASE_CHAR)
#define desc_from_str \
  desc_from_base(BASE_STR)
#define desc_from_bool \
  desc_from_base(BASE_BOOL)
#define desc_from_any \
  desc_from_base(BASE_ANY)
TypeDesc *desc_from_klass(char *path, char *type);
TypeDesc *desc_from_proto(Vector *args, TypeDesc *ret);
TypeDesc *desc_from_array(TypeDesc *para);
TypeDesc *desc_from_varg(TypeDesc *base);
TypeDesc *desc_from_map(TypeDesc *key, TypeDesc *val);
TypeDesc *str_to_desc(char *s);
TypeDesc *str_to_proto(char *ptype, char *rtype);


#define desc_isint(desc) \
  (((desc)->kind == TYPE_BASE) && \
    ((desc)->base == BASE_INT))
#define desc_isbyte(desc) \
  (((desc)->kind == TYPE_BASE) && \
    ((desc)->base == BASE_BYTE))
#define desc_is_float(desc) \
  (((desc)->kind == TYPE_BASE) && \
    ((desc)->base == BASE_FLOAT))
#define desc_ischar(desc) \
  (((desc)->kind == TYPE_BASE) && \
    ((desc)->base == BASE_CHAR))
#define desc_isstr(desc) \
  (((desc)->kind == TYPE_BASE) && \
    ((desc)->base == BASE_STR))
#define desc_isbool(desc) \
  (((desc)->kind == TYPE_BASE) && \
    ((desc)->base == BASE_BOOL))
#define desc_isany(desc) \
  (((desc)->kind == TYPE_BASE) && \
    ((desc)->base == BASE_ANY))

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_TYPEDESC_H_ */

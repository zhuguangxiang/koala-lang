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

/* literal value */
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
} literal;

typedef enum desckind {
  TYPE_BASE = 1,
  TYPE_KLASS,
  TYPE_PROTO,
  TYPE_VARG,
  TYPE_PARAREF,
} desckind;

typedef struct typeparadef {
  char *name;
  vector *types;
} typeparadef;

/*
 * type descriptor
 * Klass: Lio.File;
 * Proto: Pis:e;
 * Varg: ...s
 * Array: [s
 * Map: Mss
 */
typedef struct typedesc {
  desckind kind;
  int refcnt;
  union {
    char base;  /* one of BASE_XXX */
    struct {
      char *path;
      char *type;
      vector *typeparas;
      vector *types;
    } klass;
    struct {
      vector *args;
      struct typedesc *ret;
      vector *typeparas;
    } proto;
    struct {
      struct typedesc *type;
    } varg;
    struct {
      char *name;
      int index;
    } pararef;
  };
} typedesc;

#define desc_incref(desc) ({ \
  if (desc)                  \
    ++(desc)->refcnt;        \
  desc;                      \
})

#define desc_decref(desc) ({             \
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
void literal_show(literal *val, StrBuf *sbuf);
void desc_free(typedesc *desc);
void desc_tostr(typedesc *desc, StrBuf *buf);
void desc_show(typedesc *desc);
int desc_check(typedesc *desc1, typedesc *desc2);
char *base_str(int kind);

typedesc *desc_from_base(int kind);
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

typedesc *desc_from_klass(char *path, char *type, vector *types);
typedesc *desc_from_proto(vector *args, typedesc *ret);
void desc_set_typeparas(typedesc *desc, vector *typeparas);
typedesc *desc_from_array(typedesc *para);
typedesc *str_to_desc(char *s);
typedesc *str_to_proto(char *ptype, char *rtype);

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

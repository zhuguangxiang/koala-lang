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

void literal_show(Literal *val, StrBuf *sbuf);

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

typedef struct typeparadef {
  char *name;
  Vector *types;
} TypeParaDef;

/*
 * Type descriptor
 * Klass: Lio.File;
 * Array: [s
 * Map: Mss
 * Proto: Pis:e;
 * Varg: ...s
 */
typedef struct typedesc {
  DescKind kind;
  int refcnt;
  union {
    struct {
      char type; /* one of BASE_XXX */
      char *str;
    } base;
    struct {
      char *path;
      char *type;
      Vector *typeparas;
      Vector *pararefs;
    } klass;
    struct {
      Vector *typeparas;
      Vector *args;
      struct typedesc *ret;
    } proto;
    struct {
      Vector *typeparas;
      struct typedesc *para;
    } array;
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
void desc_free(TypeDesc *desc);
void desc_tostr(TypeDesc *desc, StrBuf *buf);
int desc_check(TypeDesc *desc1, TypeDesc *desc2);

TypeDesc *desc_from_base(int kind);
#define desc_from_byte()    desc_from_base(BASE_BYTE)
#define desc_from_integer() desc_from_base(BASE_INT)
#define desc_from_float()   desc_from_base(BASE_FLOAT)
#define desc_from_char()    desc_from_base(BASE_CHAR)
#define desc_from_string()  desc_from_base(BASE_STR)
#define desc_from_bool()    desc_from_base(BASE_BOOL)
#define desc_from_any()     desc_from_base(BASE_ANY)
TypeDesc *desc_from_klass(char *path, char *type, Vector *paras);
TypeDesc *desc_from_proto(Vector *args, TypeDesc *ret);
TypeDesc *desc_from_array(TypeDesc *para);
TypeDesc *string_to_desc(char *s);
TypeDesc *string_to_proto(char *ptype, char *rtype);
Vector *string_to_descs(char *s);
char *desc_base_str(int kind);
void desc_show(TypeDesc *desc);
#define desc_is_int(desc) \
  (((desc)->kind == TYPE_BASE) && ((desc)->base.type == BASE_INT))
#define desc_is_bye(desc) \
  (((desc)->kind == TYPE_BASE) && ((desc)->base.type == BASE_BYTE))
#define desc_is_float(desc) \
  (((desc)->kind == TYPE_BASE) && ((desc)->base.type == BASE_FLOAT))
#define desc_is_char(desc) \
  (((desc)->kind == TYPE_BASE) && ((desc)->base.type == BASE_CHAR))
#define desc_is_str(desc) \
  (((desc)->kind == TYPE_BASE) && ((desc)->base.type == BASE_STR))
#define desc_is_bool(desc) \
  (((desc)->kind == TYPE_BASE) && ((desc)->base.type == BASE_BOOL))
#define desc_is_any(desc) \
  (((desc)->kind == TYPE_BASE) && ((desc)->base.type == BASE_ANY))
#define desc_is_array(desc) \
  ((desc)->kind == TYPE_ARRAY)

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_TYPEDESC_H_ */

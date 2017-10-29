
#ifndef _KOALA_OBJECT_H_
#define _KOALA_OBJECT_H_

#include "namei.h"
#include "list.h"
#include "hash_table.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TYPE_CHAR   1
#define TYPE_BYTE   2
#define TYPE_SHORT  3
#define TYPE_INT    4
#define TYPE_FLOAT  5
#define TYPE_BOOL   6
#define TYPE_STRING 7
#define TYPE_ANY    8
#define TYPE_DEFINE 9

typedef struct TValue (*cfunc_t)(struct TValue ob, struct TValue args);

union Value {
  struct Object *gc;
  void *ptr;
  int bval;
  uint16_t ch;
  int64_t ival;
  float64_t fval;
  cfunc_t func;
};

#define TVALUE_HEAD int type; union Value value;

struct TValue {
  TVALUE_HEAD
};

#define OBJECT_HEAD \
  struct Object *ob_next; int ob_mark; struct Klass *ob_klass;

struct Object {
  OBJECT_HEAD
};

#define OBJECT_HEAD_INIT(klass) \
  .ob_next = NULL, .ob_mark = 1, .ob_klass = klass

#define init_object_head(ob, klass)  do {  \
  struct Object *o = (struct Object *)ob;  \
  o->ob_next = NULL; o->ob_mark = 1; o->ob_klass = klass; \
} while (0)

#define OB_KLASS(ob)  (((struct Object *)(ob))->ob_klass)

typedef void (*scan_fn_t)(struct Object *ob);

typedef struct Object *(*new_fn_t)(struct Object *klass, struct Object *args);

typedef void (*free_fn_t)(struct Object *ob);

struct object_metainfo {
  int nr_fields;
  int nr_methods;
  struct hash_table *namei_table;
  struct vector *methods_vector;
};

struct Klass {
  OBJECT_HEAD
  const char *name;
  size_t bsize;
  size_t isize;

  scan_fn_t   ob_scan;
  new_fn_t    ob_new;
  free_fn_t   ob_free;

  //NumberOperations *nu_ops;

};

struct cfunc_struct {
  char *name;
  char *signature;
  int access;
  cfunc_t func;
};

struct linkage_struct {
  char *str;
  int index;
};

extern struct Klass klass_klass;
// int klass_add_method(struct object *ko, struct namei *ni, struct object *ob);
// int klass_add_field(struct object *ko, struct namei *ni);
// int klass_add_cfunctions(struct object *ko, struct cfunc_struct *funcs);
// int klass_get(struct object *ko, struct namei *namei,
//               struct object **ob, int *index);
//
// void init_klass_klass(void);
// void init_klass(char *module, char *klass, int access, struct object *ko);
// void klass_display(struct object *ob);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_OBJECT_H_ */


#ifndef _KOALA_OBJECT_H_
#define _KOALA_OBJECT_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "namei.h"
#include "list.h"
#include "hash_table.h"
#include "vector.h"

#define OBJECT_HEAD \
  struct object *ob_next; int ob_mark; \
  struct klass_object *ob_klass;

struct object {
  OBJECT_HEAD
};

#define OBJECT_HEAD_INIT(klass) \
  .ob_next = NULL, .ob_mark = 1, .ob_klass = klass

#define init_object_head(ob, klass)  do {  \
  struct object *o = (struct object *)ob; \
  o->ob_next = NULL; o->ob_mark = 1; \
  o->ob_klass = klass; \
} while (0)

#define OB_KLASS(ob)  (((struct object *)(ob))->ob_klass)

typedef void (*scan_func_t)(struct object *ob);

typedef struct object *(*new_func_t)(struct object *klass, struct object *args);

typedef void (*free_func_t)(struct object *ob);

struct object_metainfo {
  int nr_fields;
  int nr_methods;
  struct hash_table *namei_table;
  struct vector *methods_vector;
};

struct klass_object {
  OBJECT_HEAD
  const char *name;
  size_t bsize;
  size_t isize;

  scan_func_t   ob_scan;
  new_func_t    ob_new;
  free_func_t   ob_free;

  struct object_metainfo *ob_meta;
};

typedef struct object *(*cfunc_t)(struct object *ob, struct object *args);

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

extern struct klass_object klass_klass;
int klass_add_method(struct object *ko, struct namei *ni, struct object *ob);
int klass_add_field(struct object *ko, struct namei *ni);
int klass_add_cfunctions(struct object *ko, struct cfunc_struct *funcs);
int klass_get(struct object *ko, struct namei *namei,
              struct object **ob, int *index);

void init_klass_klass(void);
void init_klass(char *module, char *klass, int access, struct object *ko);
void klass_display(struct object *ob);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_OBJECT_H_ */

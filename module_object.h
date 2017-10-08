
#ifndef _KOALA_MODULE_OBJECT_H_
#define _KOALA_MODULE_OBJECT_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "object.h"
#include "namei.h"

struct module_object {
  OBJECT_HEAD
  int nr_objs;
  struct hash_table *table;
  struct vector *vec;
};

extern struct klass_object module_klass;
int module_add(struct object *mo, struct namei *ni, struct object *ob);
int module_add_cfunctions(struct object *mo, struct cfunc_struct *cfuncs);
int module_get(struct object *mo, struct namei *ni,
               struct object **ob, int *index);
struct object *new_module(char *name);
struct object *load_module(char *mod_name);
struct object *find_module(char *name);
void module_display(struct object *mo);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_MODULE_OBJECT_H_ */

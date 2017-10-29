
#ifndef _KOALA_MODULE_OBJECT_H_
#define _KOALA_MODULE_OBJECT_H_

#include "Object.h"
#include "namei.h"

#ifdef __cplusplus
extern "C" {
#endif

struct module_object {
  OBJECT_HEAD
  int nr_objs;
  struct hash_table *table;
  struct vector *vec;
};

extern struct Klass module_klass;
int module_add(struct Object *mo, struct namei *ni, struct Object *ob);
int module_add_cfunctions(struct Object *mo, struct cfunc_struct *cfuncs);
int module_get(struct Object *mo, struct namei *ni,
               struct Object **ob, int *index);
struct Object *new_module(char *name);
struct Object *load_module(char *mod_name);
struct Object *find_module(char *name);
void module_display(struct Object *mo);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_MODULE_OBJECT_H_ */

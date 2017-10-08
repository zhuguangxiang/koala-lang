
#ifndef _KOALA_GSTATE_H_
#define _KOALA_GSTATE_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "object.h"
#include "hash_table.h"
#include "vector.h"

struct global_state {
  int nr_mods;
  struct hash_table *mod_table;
  struct vector *mod_vec;
  //coroutine
  //signal
  //thread
};

int gstate_get_module(char *mod_name, struct object **ob, int *index);
int gstate_add_module(char *mod_name, struct object *ob, int *index);
void init_gstate(void);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_GSTATE_H_ */


#ifndef _KOALA_GLOBLESTATE_H_
#define _KOALA_GLOBLESTATE_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct globalstate {
  // gc objects
  Object *gcobjects;
  // modules's table
  Object *modules;
  // coroutine
  // signal
  // thread
} GlobalState;

/* Exported symbols */
void Init_GlobalState(void);
int GState_Add_Module(char *path_name, Object *mo);
Object *GState_Find_Module(char *path_name);

void Object_Add_GCList(Object *ob);
void Object_Clean(void);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_GLOBLESTATE_H_ */

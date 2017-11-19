
#ifndef _KOALA_KSTATE_H_
#define _KOALA_KSTATE_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct koalastate {
  // gc objects
  Object *gcobjects;
  // module
  // coroutine
  // signal
  // thread
} KoalaState;

void Object_Add_GCList(Object *ob);
void Object_Clean(void);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_KSTATE_H_ */


#ifndef _KOALA_GCSTATE_H_
#define _KOALA_GCSTATE_H_

#include "list.h"
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct gcstate {
  int nr_objs;
  struct list_head rt_objs;
  int gcrun;
  Object *whiteobj;
  Object *grayobj;
  Object *blackobj;
} GCState;

/* Exported APIs */


#ifdef __cplusplus
}
#endif
#endif /* _KOALA_GCSTATE_H_ */

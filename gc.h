
#ifndef _KOALA_GCSTATE_H_
#define _KOALA_GCSTATE_H_

#include "list.h"
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GC_LEVEL_0 0.6
#define GC_LEVEL_1 0.8

#define GC_WHITE0 1
#define GC_WHITE1 2
#define GC_GRAY   3
#define GC_BLACK  4

#define GC_STOP  1
#define GC_MARK  2
#define GC_SWEEP 3

typedef struct gcstate {
  int state;
  int currentwhite;
  int count;
  Object *gcobjs;
  Vector grayobjs;
  Vector blackobjs;
  int total;
  int used;
  int threshold0;
  int threshold1;
} GCState;

/* Exported APIs */
void *GC_Alloc(int size);
void GC_Free(Object *ob);
void GC_Init(void);
void GC_Run(void);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_GCSTATE_H_ */

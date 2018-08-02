
#ifndef _KOALA_GCSTATE_H_
#define _KOALA_GCSTATE_H_

#include "list.h"
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GC_WHITE 0
#define GC_GRAY  1
#define GC_BLACK 2

#define GC_STOP  1
#define GC_MARK  2
#define GC_SWEEP 3

typedef struct gcstate {
	int state;
	int count;
	Object *gcobjs;
	Vector markobjs;
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

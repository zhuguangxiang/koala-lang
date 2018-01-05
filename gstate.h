
#ifndef _KOALA_GSTATE_H_
#define _KOALA_GSTATE_H_

#include "list.h"
#include "object.h"
//#include "coroutine.h"
//#include <pthread.h>
//#include <semaphore.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct gstate GState;

typedef struct threadstate {
  //pthread_t id;
  GState *gs;
  //CoRoutine *rt;
} ThreadState;

#define NR_THREAD   2
#define NR_PRIORITY 3

struct gstate {
  //int initialized;
  // gc objects' list
  //Object *gcobjects;
  // modules's table
  int nr_mods;
  HashTable modules;
  // thread
  //ThreadState threads[NR_THREAD];
  // coroutine
  int nr_rt;
  struct list_head rt_list;
  struct list_head ready_list[NR_PRIORITY];
  //sem_t sem;
  // signal
};

/* Exported symbols */
void GState_Initialize(GState *gs);
void GState_Finalize(GState *gs);
int GState_Add_Module(GState *gs, char *path, Object *mo);
int GState_Load_Module(GState *gs, char *path);
Object *GState_Find_Module(GState *gs, char *path);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_GSTATE_H_ */

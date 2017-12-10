
#ifndef _KOALA_GLOBALSTATE_H_
#define _KOALA_GLOBALSTATE_H_

#include "list.h"
#include "object.h"
#include "coroutine.h"
#include <pthread.h>
#include <semaphore.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct globalstate GlobalState;

typedef struct threadstate {
  pthread_t id;
  GlobalState *gstate;
  CoRoutine *current;
} ThreadState;

#define NR_THREAD   2
#define NR_PRIORITY 3

struct globalstate {
  int initialized;
  // gc objects
  Object *gcobjects;
  // modules's table
  Object *modules;
  // thread
  ThreadState threads[NR_THREAD];
  // coroutine
  int nr_rt;
  struct list_head rt_list;
  struct list_head ready_list[NR_PRIORITY];
  sem_t sem;
  // signal
};

/* Exported symbols */
void GState_Add_CoRoutine(CoRoutine *rt);
int GState_IsInitialized(void);
CoRoutine *GState_Next_CoRoutine(ThreadState *thread);
void Init_GlobalState(void);
void Fini_GlobalState(void);
int GState_Add_Module(char *path_name, Object *mo);
Object *GState_Find_Module(char *path_name);

void Object_Add_GCList(Object *ob);
void Object_Clean(void);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_GLOBALSTATE_H_ */

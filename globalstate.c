
#include "globalstate.h"
#include "tableobject.h"
#include "nameobject.h"
#include "moduleobject.h"

static GlobalState gState;

void Object_Add_GCList(Object *ob)
{
  // lock
  Object *temp = gState.gcobjects;
  ob->ob_next = temp;
  gState.gcobjects = ob;
  // unlock
}

void Object_Clean(void)
{
  Object *ob = gState.gcobjects;
  Object *next;

  while (ob != NULL) {
    next = ob->ob_next;
    if (ob->ob_ref == 0) {
      OB_KLASS(ob)->ob_free(ob);
    } else {
      ob->ob_ref = 0;
    }
    ob = next;
  }
}

/*-------------------------------------------------------------------------*/

static void *pthread_start(void *arg)
{
  ThreadState *thread = arg;

  printf("Thread is Waiting\n");

  while (!GState_IsInitialized());

  printf("Thread is Running\n");

  while (1) {
    sem_wait(&thread->gstate->sem);
    CoRoutine *rt = GState_Next_CoRoutine(thread);
    if (rt != NULL) CoRoutine_Run(rt);
  }
  return NULL;
}

static int thread_init(ThreadState *thread, GlobalState *gstate)
{
  thread->gstate = gstate;
  return pthread_create(&thread->id, NULL, pthread_start, thread);
}

static void thread_wait(ThreadState *thread)
{
  pthread_join(thread->id, NULL);
}

/*-------------------------------------------------------------------------*/

void GState_Add_CoRoutine(CoRoutine *rt)
{
  ++gState.nr_rt;
  list_add(&rt->link, &gState.rt_list);
  rt->gstate = &gState;
  rt->state = STATE_READY;
  list_add_tail(&rt->ready_link, &gState.ready_list[rt->prio]);
  sem_post(&rt->gstate->sem);
}

int GState_IsInitialized(void)
{
  return gState.initialized;
}

CoRoutine *GState_Next_CoRoutine(ThreadState *thread)
{
  CoRoutine *rt;
  struct list_head *entry;
  for (int i = 0; i < nr_elts(gState.ready_list); i++) {
    entry = list_first(&gState.ready_list[i]);
    if (entry != NULL) {
      list_del(entry);
      rt = container_of(entry, CoRoutine, ready_link);
      assert(rt->state == STATE_READY);
      rt->state = STATE_RUNNING;
      thread->rt = rt;
      return rt;
    }
  }
  return NULL;
}

void Init_GlobalState(void)
{
  gState.modules = Table_New();

  init_list_head(&gState.rt_list);

  for (int i = 0; i < nr_elts(gState.ready_list); i++)
    init_list_head(&gState.ready_list[i]);

  sem_init(&gState.sem, 0, 0);

  for (int i = 0; i < nr_elts(gState.threads); i++)
    thread_init(&gState.threads[i], &gState);

  gState.initialized = 1;
}

void Fini_GlobalState(void)
{
  for (int i = 0; i < nr_elts(gState.threads); i++)
    thread_wait(&gState.threads[i]);

  gState.initialized = 0;
}

/*-------------------------------------------------------------------------*/

int GState_Add_Module(char *path_name, Object *mo)
{
  assert(OB_KLASS(mo) == &Module_Klass);
  Object *no = Name_New(path_name, NT_MODULE, 0, NULL, NULL);
  TValue key = TValue_Build('O', no);
  TValue val = TValue_Build('O', mo);
  return Table_Put(gState.modules, key, val);
}

Object *GState_Find_Module(char *path_name)
{
  Object *ret = NULL;
  Object *ob = Name_New(path_name, NT_MODULE, 0, NULL, NULL);
  TValue v;

  if (Table_Get(gState.modules, TValue_Build('O', ob), NULL, &v)) {
    fprintf(stderr, "[WARN]cannot find module:%s\n", path_name);
    return NULL;
  }
  TValue_Parse(v, 'O', &ret);
  return ret;
}

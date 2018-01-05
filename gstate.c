
#include "gstate.h"
#include "tableobject.h"
#include "moduleobject.h"

// static GState GState;

// void Object_Add_GCList(Object *ob)
// {
//   // lock
//   Object *temp = GState.gcobjects;
//   ob->ob_next = temp;
//   GState.gcobjects = ob;
//   // unlock
// }

// void Object_Clean(void)
// {
//   Object *ob = GState.gcobjects;
//   Object *next;

//   while (ob != NULL) {
//     next = ob->ob_next;
//     if (ob->ob_ref == 0) {
//       OB_KLASS(ob)->ob_free(ob);
//     } else {
//       ob->ob_ref = 0;
//     }
//     ob = next;
//   }
// }

/*-------------------------------------------------------------------------*/

// static void *pthread_start(void *arg)
// {
//   ThreadState *thread = arg;

//   printf("Thread is Waiting\n");

//   while (!GState_IsInitialized());

//   printf("Thread is Running\n");

//   while (1) {
//     sem_wait(&thread->gstate->sem);
//     CoRoutine *rt = GState_Next_CoRoutine(thread);
//     if (rt != NULL) CoRoutine_Run(rt);
//   }
//   return NULL;
// }

// static int thread_init(ThreadState *thread, GlobalState *gstate)
// {
//   thread->gstate = gstate;
//   return pthread_create(&thread->id, NULL, pthread_start, thread);
// }

// static void thread_wait(ThreadState *thread)
// {
//   pthread_join(thread->id, NULL);
// }

/*-------------------------------------------------------------------------*/

struct mod_entry {
  HashNode hnode;
  char *path;
  Object *mo;
};

static struct mod_entry *new_mod_entry(char *path, Object *mo)
{
  struct mod_entry *e = malloc(sizeof(*e));
  init_hash_node(&e->hnode, e);
  e->path = path;
  e->mo = mo;
  return e;
}

static void free_mod_entry(struct mod_entry *e)
{
  assert(hash_node_unhashed(&e->hnode));
  free(e);
}

static uint32 mod_entry_hash(void *k)
{
  struct mod_entry *e = k;
  return hash_string(e->path);
}

static int mod_entry_equal(void *k1, void *k2)
{
  struct mod_entry *e1 = k1;
  struct mod_entry *e2 = k2;
  return !strcmp(e1->path, e2->path);
}

static void mod_entry_finalize(HashNode *hnode, void *arg)
{
  struct mod_entry *e = container_of(hnode, struct mod_entry, hnode);
  free_mod_entry(e);
  GState *gs = arg;
  --gs->nr_mods;
  assert(gs->nr_mods >= 0);
}

// void GState_Add_CoRoutine(CoRoutine *rt)
// {
//   ++GState.nr_rt;
//   list_add(&rt->link, &GState.rt_list);
//   rt->gstate = &GState;
//   rt->state = STATE_READY;
//   list_add_tail(&rt->ready_link, &GState.ready_list[rt->prio]);
//   sem_post(&rt->gstate->sem);
// }

// int GState_IsInitialized(void)
// {
//   return GState.initialized;
// }

// CoRoutine *GState_Next_CoRoutine(ThreadState *thread)
// {
//   CoRoutine *rt;
//   struct list_head *entry;
//   for (int i = 0; i < nr_elts(GState.ready_list); i++) {
//     entry = list_first(&GState.ready_list[i]);
//     if (entry != NULL) {
//       list_del(entry);
//       rt = container_of(entry, CoRoutine, ready_link);
//       assert(rt->state == STATE_READY);
//       rt->state = STATE_RUNNING;
//       thread->rt = rt;
//       return rt;
//     }
//   }
//   return NULL;
// }

void GState_Initialize(GState *gs)
{
  HashTable_Initialize(&gs->modules, mod_entry_hash, mod_entry_equal);

  // init_list_head(&GState.rt_list);

  // for (int i = 0; i < nr_elts(GState.ready_list); i++)
  //   init_list_head(&GState.ready_list[i]);

  // sem_init(&GState.sem, 0, 0);

  // for (int i = 0; i < nr_elts(GState.threads); i++)
  //   thread_init(&GState.threads[i], &GState);

  // GState.initialized = 1;
}

void GState_Finalize(GState *gs)
{
  HashTable_Finalize(&gs->modules, mod_entry_finalize, gs);
  // for (int i = 0; i < nr_elts(GState.threads); i++)
  //   thread_wait(&GState.threads[i]);

  // GState.initialized = 0;
}

/*-------------------------------------------------------------------------*/

int GState_Add_Module(GState *gs, char *path, Object *mo)
{
  assert(OB_KLASS(mo) == &Module_Klass);
  struct mod_entry *e = new_mod_entry(path, mo);
  if (HashTable_Insert(&gs->modules, &e->hnode) < 0) {
    free_mod_entry(e);
    return -1;
  }

  ++gs->nr_mods;
  return 0;
}

Object *GState_Find_Module(GState *gs, char *path)
{
  struct mod_entry e = {.path = path};
  HashNode *hnode = HashTable_Find(&gs->modules, &e);
  if (hnode == NULL) return NULL;
  return (Object *)(container_of(hnode, struct mod_entry, hnode)->mo);
}

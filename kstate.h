
#ifndef _KOALA_KSTATE_H_
#define _KOALA_KSTATE_H_

#include "gcstate.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NR_PRIORITY 3

typedef struct koalastate {
  /* modules */
  HashTable modules;

  /* symbols' item table for all modules and classes */
  ItemTable *itable;

  /* garbage collection */
  GCState *gcs;

  /* routine */
  int nr_rts;
  struct list_head rt_list;

} KoalaState;

/* Exported APIs */
void KState_Init(KoalaState *ks);
void KState_Fini(KoalaState *ks);
int KState_Add_Module(KoalaState *ks, char *path, Object *mo);
int KState_Load_Module(KoalaState *ks, char *path);
Object *KState_Get_Module(KoalaState *ks, char *path);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_GSTATE_H_ */


#ifndef _KOALA_VM_H_
#define _KOALA_VM_H_

#include "object.h"
#include "properties.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct koalastate {
	HashTable modules;
	Properties config;
	struct list_head routines;
} KoalaState;

/* Exported APIs */
extern KoalaState gs;
Object *Koala_New_Module(char *name, char *path);
Object *Koala_Get_Module(char *path);
Object *Koala_Load_Module(char *path);
Klass *Koala_Get_Klass(Object *ob, char *path, char *type);
void Koala_Initialize(void);
void Koala_Finalize(void);
void Koala_Run(char *path);
void Koala_Collect_Modules(Vector *vec);
Object *Koala_Run_Code(Object *code, Object *ob, Object *args);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_VM_H_ */

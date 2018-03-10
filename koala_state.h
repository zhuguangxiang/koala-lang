
#ifndef _KOALA_STATE_H_
#define _KOALA_STATE_H_

#include "object.h"
#include "properties.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct koalastate {
	HashTable modules;
	Properties config;
} KoalaState;

/* Exported APIs */
Object *Koala_New_Module(char *name, char *path);
Object *Koala_Get_Module(char *path);
Object *Koala_Load_Module(char *path);
void Koala_Initialize(void);
void Koala_Finalize(void);
void Koala_Run(char *path);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_STATE_H_ */

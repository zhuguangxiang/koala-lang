
#ifndef _KOALA_HEADER_H_
#define _KOALA_HEADER_H_

#include "log.h"
#include "stringobject.h"
#include "tupleobject.h"
#include "tableobject.h"
#include "moduleobject.h"
#include "methodobject.h"
#include "routine.h"
#include "buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported APIs */
Object *Koala_New_Module(char *name, char *path);
Object *Koala_Get_Module(char *path);
Object *Koala_Load_Module(char *path);
void Koala_Init(void);
void Koala_Fini(void);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_HEADER_H_ */

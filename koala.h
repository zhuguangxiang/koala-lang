
#ifndef _KOALA_HEADER_H_
#define _KOALA_HEADER_H_

#include "stringobject.h"
#include "tupleobject.h"
#include "tableobject.h"
#include "moduleobject.h"
#include "methodobject.h"
#include "routine.h"
#include "kstate.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported APIs */
extern KoalaState ks;
void Koala_Init(void);
void Koala_Fini(void);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_HEADER_H_ */

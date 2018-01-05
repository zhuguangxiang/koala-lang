
#ifndef _KOALA_HEADER_H_
#define _KOALA_HEADER_H_

#include "stringobject.h"
#include "tupleobject.h"
#include "tableobject.h"
#include "moduleobject.h"
#include "methodobject.h"
#include "gstate.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported symbols */
extern GState gs;
void Koala_Initialize(void);
void Koala_Finalize(void);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_HEADER_H_ */

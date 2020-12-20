/*===-- koala.h - Koala Language C Interfaces ---------------------*- C -*-===*\
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) 2020 James, https://github.com/zhuguangxiang                 *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* This header declares koala c interfaces.                                   *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_H_
#define _KOALA_H_

#include "stringobject.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct KoalaState KoalaState;

/* state functions */
DLLEXPORT KoalaState *koala_newstate(void);

/* stack push functions */
DLLEXPORT void koala_pushnil(KoalaState *ks);
DLLEXPORT void koala_pushbyte(KoalaState *ks, int8_t bval);
DLLEXPORT void koala_pushint(KoalaState *ks, int64_t ival);
DLLEXPORT void koala_pushfloat(KoalaState *ks, double fval);
DLLEXPORT void koala_pushbool(KoalaState *ks, int8_t zval);
DLLEXPORT void koala_pushchar(KoalaState *ks, int32_t cval);
DLLEXPORT void koala_pushstr(KoalaState *ks, const char *s);

/* stack access functions */
DLLEXPORT int8_t koala_tobyte(KoalaState *ks, int idx);
DLLEXPORT int64_t koala_toint(KoalaState *ks, int idx);
DLLEXPORT double koala_tofloat(KoalaState *ks, int idx);
DLLEXPORT int8_t koala_tobool(KoalaState *ks, int idx);
DLLEXPORT int32_t koala_tochar(KoalaState *ks, int idx);
DLLEXPORT const char *koala_tostr(KoalaState *ks, int idx);
/* get stack length(stack top index) */
DLLEXPORT int koala_gettop(KoalaState *ks);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_H_ */

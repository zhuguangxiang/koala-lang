/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_API_H_
#define _KOALA_API_H_

#include "version.h"
#include "log.h"
#include "memory.h"
#include "atom.h"
#include "eval.h"
#include "fieldobject.h"
#include "methodobject.h"
#include "classobject.h"
#include "intobject.h"
#include "stringobject.h"
#include "arrayobject.h"
#include "tupleobject.h"
#include "dictobject.h"
#include "moduleobject.h"
#include "codeobject.h"
#include "fmtmodule.h"
#include "parser.h"

#ifdef __cplusplus
extern "C" {
#endif

void Koala_Initialize(void);
void Koala_Finalize(void);
void Koala_CmdLine(void);
void Koala_Compile(char *path);
void Koala_Run(char *path);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_API_H_ */

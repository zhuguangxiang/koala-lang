/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_FMT_MODULE_H_
#define _KOALA_FMT_MODULE_H_

#include "object.h"
#include "strbuf.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct formatterobject {
  OBJECT_HEAD
  struct strbuf buf;
} FormatterObject;

extern TypeObject Formatter_Type;
#define Fmtter_Check(ob) (OB_TYPE(ob) == &Formatter_Type)

void init_fmt_module(void);
Object *Fmtter_New(void);
void Fmtter_Free(Object *ob);
Object *Fmtter_WriteFormat(Object *self, Object *args);
Object *Fmtter_WriteString(Object *self, Object *args);
Object *Fmtter_WriteInteger(Object *self, Object *args);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_FMT_MODULE_H_ */

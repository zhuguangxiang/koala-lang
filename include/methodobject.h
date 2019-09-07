/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_METHOD_OBJECT_H_
#define _KOALA_METHOD_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct methodobject {
  OBJECT_HEAD
  /* method name */
  char *name;
  /* method owner */
  Object *owner;
  /* method type descriptor */
  TypeDesc *desc;
  /* cfunc or kfunc ? */
  int cfunc;
  /* cfunc or kfunc pointer */
  void *ptr;
} MethodObject;

typedef struct protoobject {
  OBJECT_HEAD
  /* method name */
  char *name;
  /* method owner */
  TypeObject *owner;
  /* method type descriptor */
  TypeDesc *desc;
} ProtoObject;

extern TypeObject method_type;
extern TypeObject proto_type;
#define Method_Check(ob) (OB_TYPE(ob) == &method_type)
#define Proto_Check(ob) (OB_TYPE(ob) == &Proto_Type)
void init_method_type(void);
void init_proto_type(void);
Object *CMethod_New(MethodDef *m);
Object *Method_New(char *name, Object *code);
Object *Method_Call(Object *self, Object *ob, Object *args);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_FIELD_OBJECT_H_ */

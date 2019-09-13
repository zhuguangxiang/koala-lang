/*
 MIT License

 Copyright (c) 2018 James, https://github.com/zhuguangxiang

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
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

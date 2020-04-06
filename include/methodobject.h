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

typedef enum {
  KFUNC_KIND, CFUNC_KIND, JITFUNC_KIND,
} MethodKind;

typedef struct methodobject {
  OBJECT_HEAD
  /* method name */
  char *name;
  /* method owner */
  Object *owner;
  /* method type descriptor */
  TypeDesc *desc;
  /* cfunc or kfunc, or jited-func ? */
  MethodKind kind;
  /* cfunc or kfunc pointer */
  void *ptr;
  /* jited func pointer */
  void *jitptr;
  /* backup kfunc */
  void *backptr;
} MethodObject;

typedef struct protoobject {
  OBJECT_HEAD
  /* proto name */
  char *name;
  /* proto owner */
  Object *owner;
  /* proto type descriptor */
  TypeDesc *desc;
} ProtoObject;

extern TypeObject method_type;
extern TypeObject proto_type;
#define method_check(ob) (OB_TYPE(ob) == &method_type)
#define proto_check(ob) (OB_TYPE(ob) == &proto_type)
void init_method_type(void);
void init_proto_type(void);
Object *cmethod_new(MethodDef *m);
Object *method_new(char *name, Object *code);
Object *nmethod_new(char *name, TypeDesc *desc, void *ptr);
Object *method_call(Object *self, Object *ob, Object *args);
Object *method_getcode(Object *self);
Object *proto_new(char *name, TypeDesc *desc);
void method_update_jit(MethodObject *meth, void *fn, void *jitptr);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_FIELD_OBJECT_H_ */

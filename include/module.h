/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangXiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_MODULE_H_
#define _KOALA_MODULE_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

/* new module */
Object *module_new(char *path);

/* add type to module */
int module_add_type(Object *mod, TypeObject *type);

/* add variable to module */
int module_add_var(Object *mod, FieldObject *field);
void module_add_vardef(Object *mod, FieldDef *f);
void module_add_vardefs(Object *mod, FieldDef *def);

/* add function to module */
int module_add_func(Object *mod, MethodObject *meth);
void module_add_funcdef(Object *mod, MethodDef *f);
void module_add_funcdefs(Object *mod, MethodDef *def);

/* show module */
void module_show(Object *mod);

/* install module */
int module_install(Object *mod);

void init_core_modules(void);

void fini_modules(void);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_MODULE_H_ */

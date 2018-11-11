
#ifndef _KOALA_MODULEOBJECT_H_
#define _KOALA_MODULEOBJECT_H_

#include "codeobject.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct moduleobject {
  OBJECT_HEAD
  char *name;
  Object *consts;
  HashTable *table;
  int varcnt;
  Object *values;
} ModuleObject;

/* Exported APIs */
extern Klass Module_Klass;
#define OBJ_TO_MOD(ob) OB_TYPE_OF(ob, ModuleObject, Module_Klass)
Object *Module_New(char *name);
void Module_Free(Object *ob);
#define Module_Set_Consts(ob, _consts) do { \
  ModuleObject *mob = (ModuleObject *)ob; \
  mob->consts = _consts; \
} while (0)
int Module_Add_Var(Object *ob, char *name, TypeDesc *desc, int bconst);
int Module_Add_Func(Object *ob, char *name, Object *code);
int Module_Add_CFunc(Object *ob, FuncDef *f);
int Module_Add_Class(Object *ob, Klass *klazz);
int Module_Add_Trait(Object *ob, Klass *klazz);
TValue Module_Get_Value(Object *ob, char *name);
int Module_Set_Value(Object *ob, char *name, TValue *val);
Object *Module_Get_Function(Object *ob, char *name);
Klass *Module_Get_Class(Object *ob, char *name);
Klass *Module_Get_Trait(Object *ob, char *name);
Klass *Module_Get_ClassOrTrait(Object *ob, char *name);
int Module_Add_CFunctions(Object *ob, FuncDef *funcs);
#define Module_Name(ob) (((ModuleObject *)(ob))->name)

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_MODULEOBJECT_H_ */

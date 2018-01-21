
#ifndef _KOALA_MODULEOBJECT_H_
#define _KOALA_MODULEOBJECT_H_

#include "methodobject.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct moduleobject {
  OBJECT_HEAD
  char *name;
  HashTable *stable;
  ItemTable *itable;
  int avail_index;
  int size;
  TValue locals[0];
} ModuleObject;

/* Exported APIs */
extern Klass Module_Klass;
Object *Module_New(char *name, char *path, int nr_locals);
int Module_Add_Var(Object *ob, char *name, char *desc, int access);
int Module_Add_Func(Object *ob, char *name, char *rdesc, char *pdesc,
                    int access, Object *method);
int Module_Add_CFunc(Object *ob, FuncStruct *f);
int Module_Add_Class(Object *ob, Klass *klazz, int access);
int Module_Add_Interface(Object *ob, Klass *klazz, int access);
TValue Module_Get_Value(Object *ob, char *name);
void Module_Set_Value(Object *ob, char *name, TValue *val);
Object *Module_Get_Function(Object *ob, char *name);
Klass *Module_Get_Class(Object *ob, char *name);
Klass *Module_Get_Intf(Object *ob, char *name);
int Module_Add_CFunctions(Object *ob, FuncStruct *funcs);
void Module_Display(Object *ob);
static inline ItemTable *Module_ItemTable(Object *ob)
{
  ModuleObject *mob = OB_TYPE_OF(ob, ModuleObject, Module_Klass);
  return mob->itable;
}

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_MODULEOBJECT_H_ */


#ifndef _KOALA_MODULEOBJECT_H_
#define _KOALA_MODULEOBJECT_H_

#include "object.h"

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
void Init_Module_Klass(Object *ob);
Object *Module_New(char *name, char *path, int nr_locals);
int Module_Add_Var(Object *ob, char *name, char *desc, uint8 access);
int Module_Add_Func(Object *ob, char *name, char *rdesc, char *pdesc,
                    uint8 access, Object *method);
int Module_Add_Class(Object *ob, Klass *klazz, uint8 access);
int Module_Add_Interface(Object *ob, Klass *klazz, uint8 access);
TValue Module_Get_Value(Object *ob, char *name);
void Module_Set_Value(Object *ob, char *name, TValue *val);
Object *Module_Get_Function(Object *ob, char *name);
Object *Module_Get_Class(Object *ob, char *name);
Object *Module_Get_Interface(Object *ob, char *name);
int Module_Add_CFunctions(Object *ob, FunctionStruct *funcs);
void Module_Display(Object *ob);
static inline ItemTable *Module_ItemTable(Object *ob)
{
  OB_ASSERT_KLASS(ob, Module_Klass);
  return ((ModuleObject *)ob)->itable;
}

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_MODULEOBJECT_H_ */

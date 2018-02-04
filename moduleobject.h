
#ifndef _KOALA_MODULEOBJECT_H_
#define _KOALA_MODULEOBJECT_H_

#include "methodobject.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct moduleobject {
  OBJECT_HEAD
  char *name;
  STable stable;
  Object *tuple;
} ModuleObject;

/* Exported APIs */
extern Klass Module_Klass;
#define OBJ_TO_MOD(ob) OB_TYPE_OF(ob, ModuleObject, Module_Klass)
void Init_Module_Klass(Object *ob);
Object *Module_New(char *name, char *path);
void Module_Free(Object *ob);
int Module_Add_Var(Object *ob, char *name, TypeDesc *desc);
int Module_Add_Const(Object *ob, char *name, TypeDesc *desc);
int Module_Add_Func(Object *ob, char *name, ProtoInfo *proto, Object *meth);
int Module_Add_CFunc(Object *ob, FuncDef *f);
int Module_Add_Class(Object *ob, Klass *klazz);
int Module_Add_Interface(Object *ob, Klass *klazz);
TValue Module_Get_Value(Object *ob, char *name);
TValue Module_Get_Value_ByIndex(Object *ob, int index);
int Module_Set_Value(Object *ob, char *name, TValue *val);
int Module_Set_Value_ByIndex(Object *ob, int index, TValue *val);
Object *Module_Get_Function(Object *ob, char *name);
Klass *Module_Get_Class(Object *ob, char *name);
Klass *Module_Get_Intf(Object *ob, char *name);
Klass *Module_Get_Klass(Object *ob, char *name);
Symbol *Module_Get_Symbol(Object *ob, char *name);
int Module_Add_CFunctions(Object *ob, FuncDef *funcs);
void Module_Show(Object *ob);
#define Module_Name(ob) (((ModuleObject *)(ob))->name)
#define Module_AtomTable(ob) (((ModuleObject *)(ob))->stable.atable)
#define Module_STable(ob) (&((ModuleObject *)(ob))->stable)
STable *Object_STable(Object *ob);
AtomTable *Object_ItemTable(Object *ob);
STable *Module_Get_STable(Object *ob);
#ifdef __cplusplus
}
#endif
#endif /* _KOALA_MODULEOBJECT_H_ */

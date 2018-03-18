
#ifndef _KOALA_MODULEOBJECT_H_
#define _KOALA_MODULEOBJECT_H_

#include "codeobject.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct moduleobject {
	OBJECT_HEAD
	char *name;
	STable stbl;
	Object *tuple;
} ModuleObject;

/* Exported APIs */
extern Klass Module_Klass;
#define OBJ_TO_MOD(ob) OB_TYPE_OF(ob, ModuleObject, Module_Klass)
Object *Module_New(char *name, AtomTable *atbl);
void Module_Free(Object *ob);
int Module_Add_Var(Object *ob, char *name, TypeDesc *desc, int bconst);
int Module_Add_Func(Object *ob, char *name, Proto *proto, Object *code);
int Module_Add_CFunc(Object *ob, FuncDef *f);
int Module_Add_Class(Object *ob, Klass *klazz);
int Module_Add_Interface(Object *ob, Klass *klazz);
TValue Module_Get_Value(Object *ob, char *name);
int Module_Set_Value(Object *ob, char *name, TValue *val);
Object *Module_Get_Function(Object *ob, char *name);
Klass *Module_Get_Class(Object *ob, char *name);
Klass *Module_Get_Intf(Object *ob, char *name);
int Module_Add_CFunctions(Object *ob, FuncDef *funcs);
void Module_Show(Object *ob);
#define Module_Name(ob) (((ModuleObject *)(ob))->name)
#define Module_AtomTable(ob) (((ModuleObject *)(ob))->stbl.atbl)
/* for compiler only */
STable *Module_To_STable(Object *ob, AtomTable *atbl, char *path);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_MODULEOBJECT_H_ */

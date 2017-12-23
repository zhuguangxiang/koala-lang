
#ifndef _KOALA_MODULEOBJECT_H_
#define _KOALA_MODULEOBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct moduleobject {
  OBJECT_HEAD
  char *name;
  int avail;
  Object *table;
  Object *tuple;
} ModuleObject;

/* Exported symbols */
extern Klass Module_Klass;
void Init_Module_Klass(void);
Object *Module_New(char *name, int nr_vars);
int Module_Add_Variable(Object *ob, char *name, char *desc,
                        uint8 access, int k);
int Module_Add_Function(Object *ob, char *name, char *desc, char *pdesc,
                        uint8 access, Object *method);
int Module_Add_Klass(Object *ob, Klass *klazz, uint8 access, int intf);
int Module_Get(Object *ob, char *name, TValue *k, TValue *v);
Object *Load_Module(char *path_name);
void Module_Display(Object *ob);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_MODULEOBJECT_H_ */

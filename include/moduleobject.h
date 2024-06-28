/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_MODULE_OBJECT_H_
#define _KOALA_MODULE_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _RelocInfo {
    /* module name */
    const char *name;
    /* module object */
    Object *module;
} RelocInfo;

typedef struct _ModuleObject {
    OBJECT_HEAD
    /* all symbol names */
    Vector names;
    /* all symbol values */
    Vector values;
    /* constants */
    Vector consts;
    /* relocations */
    Vector relocs;
} ModuleObject;

extern TypeObject module_type;
#define IS_MODULE(ob) IS_TYPE((ob), &module_type)

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_MODULE_OBJECT_H_ */

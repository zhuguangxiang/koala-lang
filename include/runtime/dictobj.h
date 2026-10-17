/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_DICT_OBJECT_H_
#define _KOALA_DICT_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _DictObject {
    OBJECT_HEAD
    HashMap map;
} DictObject;

extern TypeObject dict_type;
#define IS_DICT(ob) IS_TYPE((ob), &dict_type)

Object *kl_new_dict(void);

/* Register the native methods of 'DictKeysIter' (std/builtin/dict.kl). */
void dict_keys_iter_register(NativeLib *lib);

/* Register the native methods of 'DictValuesIter' (std/builtin/dict.kl). */
void dict_values_iter_register(NativeLib *lib);

/* Register the native methods of 'DictViewIter' (std/builtin/dict.kl). */
void dict_view_iter_register(NativeLib *lib);

/* Register the native methods of 'DictRevIter' (std/builtin/dict.kl). */
void dict_rev_iter_register(NativeLib *lib);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_DICT_OBJECT_H_ */

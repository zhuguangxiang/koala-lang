/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_ITER_OBJECT_H_
#define _KOALA_ITER_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Iterable object is a trait. */
extern TraitObject iterable_type;
#define IS_ITERABLE(ob) IS_TYPE((ob), &iterable_type)

/* Iterator object is a trait. */
extern TraitObject iterator_type;
#define IS_ITERATOR(ob) IS_TYPE((ob), &iterator_type)

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_ITER_OBJECT_H_ */

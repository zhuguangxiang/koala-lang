/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_BUILTIN_H_
#define _KOALA_BUILTIN_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

extern TypeObject int8_type;
extern TypeObject int16_type;
extern TypeObject int32_type;
extern TypeObject int64_type;
extern TypeObject uint8_type;
extern TypeObject uint16_type;
extern TypeObject uint32_type;
extern TypeObject uint64_type;
#define int_type int64_type
extern TypeObject float16_type;
extern TypeObject float32_type;
extern TypeObject float64_type;

extern TypeObject Iterable_type;
extern TypeObject Iterator_type;
extern TypeObject Collection_type;
extern TypeObject Sequence_type;
extern TypeObject MutableSequence_type;
extern TypeObject Number_type;

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_BUILTIN_H_ */

/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_BOOL_OBJECT_H_
#define _KOALA_BOOL_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

extern TypeObject bool_type;
extern Value true_value;
extern Value false_value;

/* Rich comparison opcodes */
#define CMP_LT 0
#define CMP_LE 1
#define CMP_EQ 2
#define CMP_NE 3
#define CMP_GT 4
#define CMP_GE 5

#define RETURN_TRUE  return bool_value(1)
#define RETURN_FALSE return bool_value(0)

/*
 * Macro for implementing rich comparisons
 *
 * C-comparison to Koala's rich comparison
 */

// clang-format off

#define RETURN_RICHCOMPARE(val, op) do {                     \
    switch (op) {                                            \
    case CMP_EQ: if ((val) == 0) RETURN_TRUE; RETURN_FALSE;  \
    case CMP_NE: if ((val) != 0) RETURN_TRUE; RETURN_FALSE;  \
    case CMP_LT: if ((val) < 0) RETURN_TRUE; RETURN_FALSE;   \
    case CMP_GT: if ((val) > 0) RETURN_TRUE; RETURN_FALSE;   \
    case CMP_LE: if ((val) <= 0) RETURN_TRUE; RETURN_FALSE;  \
    case CMP_GE: if ((val) >= 0) RETURN_TRUE; RETURN_FALSE;  \
    default:                                                 \
        UNREACHABLE();                                       \
    }                                                        \
} while (0)

// clang-format on

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_BOOL_OBJECT_H_ */

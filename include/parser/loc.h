/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_LOC_H_
#define _KOALA_LOC_H_

#ifdef __cplusplus
extern "C" {
#endif

/* location */
typedef struct _Loc {
    int line;
    int col;
    int last_line;
    int last_col;
} Loc;

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_LOC_H_ */

/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 */

#ifndef _KOALA_H_
#define _KOALA_H_

#ifdef __cplusplus
extern "C" {
#endif

void koala_init(void);
void koala_fini(void);

void koala_cmd(void);
void koala_compile(char *path);
void koala_run(char *path);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_H_ */

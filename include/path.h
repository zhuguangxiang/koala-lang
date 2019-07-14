/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_PATH_H_
#define _KOALA_PATH_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Convert a path string to null-terminated string array. */
char **path_toarr(char *path, int len);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_PATH_H_ */

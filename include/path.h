/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_PATH_H_
#define _KOALA_PATH_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * convert a path string to dir-name array.
 *
 * path - The path to convert.
 * len  - The path length.
 *
 * Returns an array contains dir-names.
 */
char **path_toarr(char *path, int len);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_PATH_H_ */

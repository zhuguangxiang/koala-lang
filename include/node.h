/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_NODE_H_
#define _KOALA_NODE_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Initialize node system.
 *
 * Returns nothing.
 */
void node_initialize(void);

/*
 * destroy node system.
 *
 * Returns nothing.
 */
void node_destroy(void);

/*
 * add a new leaf node into node system(like file system).
 *
 * pathes - The pathes, null-terminated string array, of the node to install.
 * data   - The node's data pointer.
 *
 * Returns 0 on successful or -1 if exists or memory allocation failed.
 */
int add_leaf(char *pathes[], void *data);

/*
 * get a node's data by its path.
 *
 * pathes - The pathes, null-terminated string array, of a node to get.
 *
 * Returns the node's data or NULL if not exists.
 */
void *get_leaf(char *pathes[]);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_NODE_H_ */

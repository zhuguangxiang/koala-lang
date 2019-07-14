/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 *
 * The simple tree system, like file system, to store loaded modules.
 */

#ifndef _KOALA_NODE_H_
#define _KOALA_NODE_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Add a new leaf node with data into node tree. The pathes array is
 * the leaf node's path name, with last one element is null.
 * Returns 0 on successful or -1 if exists or memory allocation failed.
 */
int add_leaf(char *pathes[], void *data);

/*
 * Get a node's data by its pathes with last one is null.
 * Returns the node's data or NULL if not exists.
 */
void *get_leaf(char *pathes[]);

/* Initialize node tree */
void node_init(void);

/* Free node tree and leaf node */
void node_fini(void);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_NODE_H_ */

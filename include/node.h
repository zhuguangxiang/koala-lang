/*
 MIT License

 Copyright (c) 2018 James, https://github.com/zhuguangxiang

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
*/

/*
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

/* Convert a path string to null-terminated string array. */
char **path_toarr(char *path, int len);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_NODE_H_ */

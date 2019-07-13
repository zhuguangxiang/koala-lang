/*
MIT License

Copyright (c) 2018 James, https://github.com/zhuguangxiang

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
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
void add_leaf(char *pathes[], void *data);

/*
 * get a node's data by its path.
 *
 * pathes - The pathes, null-terminated string array, of a node to get.
 *
 * Returns the node's data or NULL if not exists.
 */
void *get_leaf(char *pathes[]);

/*
 * convert a path string to dir-name array.
 *
 * path - The path to convert.
 *
 * Returns an array contains dir-names.
 */
char **path_toarr(char *path);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_NODE_H_ */

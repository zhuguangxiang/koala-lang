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

#ifndef _KOALA_ITERATOR_H_
#define _KOALA_ITERATOR_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* an iterator to be used to loop over a data structure */
struct iterator {
  /* save the data structure to be accessed looply */
  void *iterable;
  /* index of current item, if it has 'index' operation */
  int index;
  /* pointer to current item */
  void *current;
  /*
   * next callback to get next element
   * 1: ->current is valid
   * 0: ->current is invalid
   */
  int (*next)(struct iterator *self);
};

/*
 * Declare an iterator to loop over a loopable data structure.
 *
 * name     - The name of the iterator
 * iterable - The data structure providing the items though which to iterate.
 * next     - The function that provides the next item in the iterator loop.
 *            It must set 'current' to the next item and advance
 *            the loop 'index'.
 *            If it has next item, returns true, otherwise returns false.
 *
 * Returns the iterator or null if memory allocation failed.
 *
 * Examples:
 *   ITERATOR(iter, array, array_next);
 *   iter_for_each_as(&iter, char *, item) {
 *     do something with the 'item'
 *   }
 */
#define ITERATOR(name, iterable, next) \
  struct iterator name = {iterable, 0, NULL, next}

/*
 * Iterate over an iterator.
 *
 * self - The Iterator to be accessed.
 * item - The item's variable, its type is void *.
 */
#define iter_for_each(self, item) \
  for (;(self)->next(self) && ({item = (self)->current; 1;});)

/*
 * Iterate over an iterator with type.
 *
 * self - The Iterator to be accessed.
 * type - The item's type.
 * item - The item's variable.
 */
#define iter_for_each_as(self, type, item) \
  for (;(self)->next(self) && ({item = *(type *)((self)->current); 1;});)

/*
 * Reset an iterator for loop again.
 *
 * self - The Iterator to be accessed.
 */
#define iter_reset(self) \
  (self)->index = 0; (self)->current = NULL;

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_ITERATOR_H_ */

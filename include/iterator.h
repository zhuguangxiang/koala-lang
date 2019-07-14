/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
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
  /* index of item or any other meanings */
  int index;
  /* point to item */
  void *item;
  /* next callback to get next element */
  void *(*next)(struct iterator *self);
};

/*
 * Declare an iterator to loop over a loopable data structure.
 *
 * name     - The name of the iterator
 * iterable - The data structure providing the items though which to iterate.
 * next     - The function that provides the next item in the iterator loop.
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
  for (; (item = (self)->next(self)); )

/*
 * Iterate over an iterator with type.
 *
 * self - The Iterator to be accessed.
 * type - The item's type.
 * item - The item's variable.
 */
#define iter_for_each_as(self, type, item) \
  for (void *__v__; (__v__ = (self)->next(self)) && (item = *(type *)__v__); )

/*
 * Reset an iterator for loop again.
 *
 * self - The Iterator to be accessed.
 */
#define iter_reset(self) \
  (self)->index = 0; (self)->item = NULL;

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_ITERATOR_H_ */

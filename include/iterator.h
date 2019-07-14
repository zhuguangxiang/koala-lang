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
 * The 'iterable' data structure providing the items though which to iterate.
 * The 'next' function that provides the next item in the iterator loop.
 *
 * Examples:
 *   ITERATOR(iter, array, array_next);
 *   iter_for_each_as(&iter, char *, item) {
 *     do something with the 'item'
 *   }
 */
#define ITERATOR(name, iterable, next) \
  struct iterator name = {iterable, 0, NULL, next}

/* Iterate over an iterator. */
#define iter_for_each(self, item) \
  for (; (item = (self)->next(self)); )

/* Iterate over an iterator with type. */
#define iter_for_each_as(self, type, item) \
  for (void *_v_; (_v_ = (self)->next(self)) && (item = *(type *)_v_);)

/* Reset an iterator for loop again. */
#define iter_reset(self) \
  (self)->index = 0; (self)->item = NULL;

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_ITERATOR_H_ */


#ifndef _KOALA_TASK_ATOMIC_H_
#define _KOALA_TASK_ATOMIC_H_

/*
 * set value to [loc] and return old value
 */
int atomic_set(int *loc, int value);

void *atomic_set_pointer(void **loc, void *ptr);

#endif /* _KOALA_TASK_ATOMIC_H_ */

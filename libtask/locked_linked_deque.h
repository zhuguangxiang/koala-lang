
#ifndef _LOCKED_LINKED_DEQUE_H_
#define _LOCKED_LINKED_DEQUE_H_

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct lldq_node {
  void *data;
  struct lldq_node *next;
} lldq_node_t;

typedef struct lldq_deque {
  int count;
  lldq_node_t *head;
  lldq_node_t *tail;
  pthread_mutex_t locker;
} lldq_deque_t;

int lldq_init(lldq_deque_t *deque);
lldq_node_t *lldq_pop_head(lldq_deque_t *deque);
lldq_node_t *lldq_pop_tail(lldq_deque_t *deque);
void lldq_push_head(lldq_deque_t *deque, lldq_node_t *node);
void lldq_push_tail(lldq_deque_t *deque, lldq_node_t *node);

#ifdef __cplusplus
}
#endif
#endif /* _LOCKED_LINKED_DEQUE_H_ */

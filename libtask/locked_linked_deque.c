
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "locked_linked_deque.h"

int lldq_init(lldq_deque_t *deque)
{
  deque->tail = calloc(1, sizeof(lldq_node_t));
  if (!deque->tail) {
    errno = ENOMEM;
    return -1;
  }
  deque->head = deque->tail;
  deque->count = 0;
  pthread_mutex_init(&deque->locker, NULL);
  return 0;
}

lldq_node_t *lldq_pop_head(lldq_deque_t *deque)
{
  pthread_mutex_lock(&deque->locker);
  lldq_node_t *head = deque->head;
  lldq_node_t *first = head->next;
  if (first) {
    head->next = first->next;
    first->next = NULL;
    deque->count--;
  }
  if (first == deque->tail) {
    deque->tail = deque->head;
  }
  pthread_mutex_unlock(&deque->locker);
  return first;
}

lldq_node_t *lldq_pop_tail(lldq_deque_t *deque);

void lldq_push_head(lldq_deque_t *deque, lldq_node_t *node);

void lldq_push_tail(lldq_deque_t *deque, lldq_node_t *node)
{
  pthread_mutex_lock(&deque->locker);
  lldq_node_t *tail = deque->tail;
  tail->next = node;
  deque->tail = node;
  deque->count++;
  pthread_mutex_unlock(&deque->locker);
}

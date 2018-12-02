/*
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

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

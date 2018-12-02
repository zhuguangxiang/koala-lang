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

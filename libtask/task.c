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
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "task_scheduler.h"
#include "task_atomic.h"

/* global variables */
#define SCHED_STATE_NONE    0
#define SCHED_STATE_STARTED 1
#define SCHED_STATE_ERROR   2
static int sched_state = SCHED_STATE_NONE;

static int num_pthreads;
static pthread_t *pthreads;
static pthread_key_t local_pthread_key;

static task_processor_t *processors;
static int sched_shutdown = 0;
static uint64_t task_idgen = 1;

/* get current processor(current pthread) */
task_processor_t *current_processor(void)
{
	return pthread_getspecific(local_pthread_key);
}

/* task routine entry function */
static void *task_go_routine(void *arg)
{
	task_t *task = arg;
	void *result = task->entry(task->arg);
	printf("result:%lu\n", (intptr_t)result);

	/* do something after task is finished. */
	task->result = result;
	if (task->join_state != TASK_DETACHED) {
		int old_state = __sync_lock_test_and_set((int *)&task->join_state, TASK_WAIT_FOR_JOINER);
		if (old_state == TASK_JOINABLE) {
			/* this state asserts no task joins us */
			assert(!task->join_task);
			printf("here ??\n");
		} else if (old_state == TASK_WAIT_TO_JOIN) {
			/* a joining task is waiting for us to finish. */
			task_t *join_task = NULL;
			while(!join_task) {
				join_task = __sync_lock_test_and_set((void **)&task->join_task, NULL);
			}
			assert(join_task);
			assert(join_task->state == TASK_STATE_WAITING);
			printf("task-%lu is joined by task-%lu, wakeup it.\n", task->id, join_task->id);
			task->join_task = NULL;
			join_task->state = TASK_STATE_READY;
			join_task->result = result;
			schedule(current_processor()->scheduler, join_task);
		} else {
			/* it's TASK_WAIT_FOR_JOINER - that's an error!! */
			assert(0);
		}
	}
	task->state = TASK_STATE_DONE;
	task_yield();
	/* !!never go here!! */
	assert(0);
}

/* switch to new task */
static void task_switch_to(task_processor_t *proc, task_t *to)
{
	task_t *from = proc->current;
	if (from->state == TASK_STATE_RUNNING) {
		from->state = TASK_STATE_READY;
		if (from != proc->idle)
			schedule(proc->scheduler, from);
	} else if (from->state == TASK_STATE_DONE) {
		printf("task-%lu is done\n", from->id);
	} else if (from->state == TASK_STATE_WAITING) {
		printf("task-%lu is waiting\n", from->id);
	} else {
		assert(0);
	}

	assert(to->state == TASK_STATE_READY);
	to->state = TASK_STATE_RUNNING;
	proc->current = to;
	task_context_swap(&from->context, &to->context);
}

/*
 * pthread routine per processor
 * get next task from current processor and run it
 */
static void *pthread_routine(void *arg)
{
	pthread_setspecific(local_pthread_key, arg);
	task_processor_t *proc = arg;
	task_t *task;
	while (!sched_shutdown) {
		printf("pthread_routine\n");
		sched_load_balance(proc->scheduler);
		task = sched_get_task(proc->scheduler);
		if (task) {
			printf("task-%lu in pthread\n", task->id);
			task_switch_to(proc, task);
		} else {
			/* printf("sched-%d, no more tasks, sleep 1s\n", sched->id); */
			assert(proc->current == proc->idle);
			assert(proc->current->state == TASK_STATE_RUNNING);
			sleep(1);
		}
	}
	return NULL;
}

/* idle task per processor */
static task_t *task_create_idle()
{
	task_t *task = calloc(1, sizeof(task_t));
	if (!task) {
		errno = ENOMEM;
		return NULL;
	}

	task->state = TASK_STATE_RUNNING;
	task_context_save(&task->context);

	return task;
}

/* initialize 'index' processor(pthread) */
static void init_proc(int index)
{
	task_t *task = task_create_idle();
	processors[index].current = task;
	processors[index].idle = task;
	processors[index].id = index;
	processors[index].scheduler = get_scheduler(index);
}

/* initialize main scheduler(pthread) */
static void init_main_proc(void)
{
	init_proc(0);
	pthread_key_create(&local_pthread_key, NULL);
	pthread_setspecific(local_pthread_key, &processors[0]);
}

int task_init_processors(int num_threads)
{
	if (sched_state != SCHED_STATE_NONE) {
		errno = EINVAL;
		return -1;
	}

	if (init_scheduler(num_threads))
		return -1;

	num_pthreads = num_threads;
	processors = calloc(num_threads, sizeof(task_processor_t));
	assert(processors);
	pthreads = calloc(num_threads, sizeof(pthread_t));
	assert(pthreads);

	/* main processor */
	init_main_proc();
	pthreads[0] = pthread_self();

	for (int i = 1; i < num_threads; i++) {
		init_proc(i);
		pthread_create(&pthreads[i], NULL, pthread_routine, &processors[i]);
	}

	return 0;
}

/* create an task and run it */
task_t *task_create(task_attr_t *attr, task_entry_t entry, void *arg)
{
	task_t *task = calloc(1, sizeof(task_t));
	if (!task) {
		errno = ENOMEM;
		return NULL;
	}

	task->entry = entry;
	task->arg = arg;
	task->state = TASK_STATE_READY;
	task->id = task_idgen++;
	task_context_init(&task->context, attr->stacksize, task_go_routine, task);

	schedule(current_processor()->scheduler, task);
	printf("task %lu created\n", task->id);
	return task;
}

int task_yield(void)
{
	task_processor_t *proc = current_processor();
	sched_load_balance(proc->scheduler);
	task_t *task = sched_get_task(proc->scheduler);
	if (!task) {
		task_t *current = proc->current;
		if (current->state == TASK_STATE_DONE ||
				current->state == TASK_STATE_WAITING) {
			task = proc->idle;
			printf("idle state:%d\n", task->state);
			task_switch_to(proc, task);
		}
	} else {
		task_switch_to(proc, task);
	}
	return 0;
}

int task_join(task_t *task, void **result)
{
	if (result) *result = NULL;

	if (task->join_state == TASK_DETACHED) {
		printf("task cannot be joined.\n");
		return -1;
	}

	int old_state = __sync_lock_test_and_set((int *)&task->join_state, TASK_WAIT_TO_JOIN);
	if (old_state == TASK_JOINABLE) {
		/* need to wait until other task finished */
		task_processor_t *proc = current_processor();
		task_t *current = proc->current;
		current->state = TASK_STATE_WAITING;
		task->join_task = current;
		printf("task-%lu is not finished\n", task->id);
		task_yield();
		assert(current->state == TASK_STATE_RUNNING);
		if (result) *result = current->result;
	} else if (old_state == TASK_WAIT_FOR_JOINER) {
		/* other task is finished */
		printf("task-%lu is finished\n", task->id);
		if (result) *result = task->result;
	} else {
		/* it's TASK_WAIT_TO_JOIN or TASK_DETACHED - that's an error!! */
		assert(0);
	}
	return 0;
}

int task_detach(task_t *task);

/* get current task(like pthread_self()) */
task_t *current_task(void)
{
	return current_processor()->current;
}

/*===-- task.c - Koala Coroutine Library --------------------------*- C -*-===*\
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) 2020 James, https://github.com/zhuguangxiang                 *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* This file implements koala coroutine.                                      *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "task.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

/* linked locked deque */
typedef struct lldq_deque {
    uint32_t count;
    lldq_node_t *head;
    lldq_node_t *tail;
    lldq_node_t dummy;
    uint64_t steal_count;
    pthread_mutex_t lock;
} lldq_deque_t;

int lldq_init(lldq_deque_t *deque)
{
    deque->dummy.next = NULL;
    deque->tail = &deque->dummy;
    deque->head = deque->tail;
    deque->count = 0;
    pthread_mutex_init(&deque->lock, NULL);
    return 0;
}

lldq_node_t *lldq_pop_head(lldq_deque_t *deque)
{
    pthread_mutex_lock(&deque->lock);
    lldq_node_t *head = deque->head;
    lldq_node_t *first = head->next;
    if (first) {
        head->next = first->next;
        first->next = NULL;
        deque->count--;
    }
    if (first == deque->tail) { deque->tail = deque->head; }
    pthread_mutex_unlock(&deque->lock);
    return first;
}

lldq_node_t *lldq_pop_tail(lldq_deque_t *deque);

void lldq_push_head(lldq_deque_t *deque, lldq_node_t *node);

void lldq_push_tail(lldq_deque_t *deque, lldq_node_t *node)
{
    pthread_mutex_lock(&deque->lock);
    lldq_node_t *tail = deque->tail;
    tail->next = node;
    deque->tail = node;
    node->next = NULL;
    deque->count++;
    pthread_mutex_unlock(&deque->lock);
}

/* task processor per thread */
typedef struct task_processor {
    task_t *volatile current;
    task_t idle_task;
    lldq_deque_t ready_deque;
    pthread_t pid;
} task_processor_t;

static lldq_deque_t global_deque;
static task_processor_t procs[TASK_MAX_PROCS];
static uint64_t task_idgen = 1000;
static int task_shutdown = 0;
static __thread task_processor_t *current;

task_t *current_task(void)
{
    return current->current;
}

/* idle task per processor */
static inline void init_idle_task(task_t *task)
{
    task->state = TASK_STATE_RUNNING;
    task->id = ++task_idgen;
    task_context_save(&task->context);
}

/* switch to new task */
static void task_switch_to(task_t *to)
{
    task_t *from = current_task();
    if (from->state == TASK_STATE_RUNNING) {
        from->state = TASK_STATE_READY;
        if (from != &current->idle_task) {
            lldq_push_tail(&current->ready_deque, &from->dq_node);
        }
        printf("task-%lu from running -> ready\n", from->id);
    }
    else if (from->state == TASK_STATE_DONE) {
        printf("task-%lu: done\n", from->id);
    }
    else if (from->state == TASK_STATE_WAITING) {
        printf("task-%lu: waiting\n", from->id);
    }
    else {
        assert(0);
    }

    printf("SWITCH to task-%lu\n", to->id);
    assert(to->state == TASK_STATE_READY);
    to->state = TASK_STATE_RUNNING;
    current->current = to;
    task_context_switch(&from->context, &to->context);
}

/*
 * pthread routine per processor
 * get next task from current processor and run it
 */
static void *pthread_routine(void *arg)
{
    current = arg;

    printf("pthread_routine\n");
    // sleep(3);
    task_yield();

    /* !! NEVER go here !! */
    abort();
    return NULL;
}

void init_task_procs(void)
{
    for (int i = 0; i < TASK_MAX_PROCS; i++) {
        lldq_init(&procs[i].ready_deque);
        init_idle_task(&procs[i].idle_task);
        procs[i].current = &procs[i].idle_task;
    }

    current = &procs[0];
    current->pid = pthread_self();

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    for (int i = 1; i < TASK_MAX_PROCS; i++) {
        pthread_create(&procs[i].pid, &attr, pthread_routine, &procs[i]);
    }
}

/* task routine */
static void *task_routine(void *arg)
{
    task_t *task = arg;
    void *result = task->entry(task->arg);
    task->result = result;
    task->state = TASK_STATE_DONE;
    printf("task-%lu: finished, result:%lu\n", task->id, (intptr_t)result);
    task_yield();

    /* !! NEVER go here !! */
    abort();
    return NULL;
}

task_t *task_create(task_entry_t entry, void *arg)
{
    task_t *task = calloc(1, sizeof(task_t));
    if (!task) {
        errno = ENOMEM;
        return NULL;
    }

    task->entry = entry;
    task->arg = arg;
    task->state = TASK_STATE_READY;
    task->id = ++task_idgen;

    task_context_init(&task->context, 4096, task_routine, task);

    lldq_push_tail(&current->ready_deque, &task->dq_node);

    printf("task-%lu: created\n", task->id);

    return task;
}

void task_yield(void)
{
    task_t *nxt = (task_t *)lldq_pop_head(&current->ready_deque);
    if (nxt) { task_switch_to(nxt); }
    else {
        task_t *task = current_task();
        if (task->state == TASK_STATE_DONE) {
            printf("task-%lu: done\n", task->id);
        }
        printf("No more tasks in proc and exit.\n");
        pthread_exit(NULL);
    }
}

void task_sleep(int ms)
{
}

#ifdef __cplusplus
}
#endif

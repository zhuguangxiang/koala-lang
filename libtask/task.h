/*===-- vm.h - Koala Virtual Machine ------------------------------*- C -*-===*\
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) 2020 James, https://github.com/zhuguangxiang                 *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* This header declares koala virtual machine structures and interfaces.      *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_TASK_H_
#define _KOALA_TASK_H_

#include "task_context.h"
#include <pthread.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* linked locked deque node */
typedef struct lldq_node {
    struct lldq_node *next;
} lldq_node_t;

/* task state */
typedef enum {
    TASK_STATE_RUNNING = 1,
    TASK_STATE_READY = 2,
    TASK_STATE_WAITING = 3,
    TASK_STATE_DONE = 4,
} task_state_t;

/* task */
typedef struct task {
    lldq_node_t dq_node;
    task_context_t context;
    task_entry_t entry;
    void *arg;
    task_state_t volatile state;
    uint64_t id;
    void *volatile result;
    void *data;
} task_t;

#define TASK_MAX_PROCS 4

void init_task_procs(void);
task_t *current_task(void);
task_t *task_create(task_entry_t entry, void *arg);
void task_yield(void);
void task_sleep(int ms);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_TASK_H_ */

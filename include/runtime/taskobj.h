/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_TASK_OBJECT_H_
#define _KOALA_TASK_OBJECT_H_

#include "kosched.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TaskObject.state */
#define TASK_PENDING   0 /* spawned, not started */
#define TASK_RUNNING   1
#define TASK_DONE      2
#define TASK_FAILED    3
#define TASK_CANCELLED 4

typedef struct _TaskObject {
    OBJECT_HEAD
    /* NULL once dead and recycled */
    KoalaState *ks;
    int state;
    /* entry function and args until started; args shared with ks->co_args */
    Object *fn;
    TValue *args;
    int nargs;
    TValue result;
    /* mm-allocated Exception when FAILED, NOT a GC block (kl_exc_mark) */
    Object *exc;
    /* WaitRecord.link of joiners */
    List joiners;
    struct _GroupObject *group;
    /* on group->children */
    List group_link;
    uint8_t joined;
    /* on sched->tasks, for exit-time error reporting */
    List task_link;
} TaskObject;

extern TypeObject task_type;
#define IS_TASK(ob) IS_TYPE((ob), &task_type)

/* Create a task and spawn its coroutine; runnable but not started. */
Object *kl_new_task(Object *fn, TValue *args, int nargs);

/* Wake every joiner with KL_PARK_OK; called when the task finishes. */
void kl_task_wake_joiners(TaskObject *t);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_TASK_OBJECT_H_ */

/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_GROUP_OBJECT_H_
#define _KOALA_GROUP_OBJECT_H_

#include "kosched.h"
#include "taskobj.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Structured concurrency: `with task.group() as g { g.spawn(...) }`.
 * __exit__ parks until every child is done. A failing child cancels its
 * siblings; the first error is re-raised in the parent. task.timeout(d) is
 * a group whose deadline timer cancels the children and raises Timeout.
 */
typedef struct _GroupObject {
    OBJECT_HEAD
    /* TaskObject.group_link of live children */
    List children;
    int nlive;
    /* the parent's WaitRecord while parked in __exit__ (0 or 1) */
    List waitq;
    /* strdup of the first child error message */
    char *first_err;
    int failed;
    int timed_out;
    /* the group itself cancelled the children (deadline or parent cancel):
     * their Cancelled errors are not child failures */
    int cancel_requested;
    /* deadline timer, idx == -1 when none */
    TimerNode timer;
} GroupObject;

extern TypeObject group_type;
#define IS_GROUP(ob) IS_TYPE((ob), &group_type)

Object *kl_new_group(void);

/* Spawn a child into the group. */
Object *kl_group_spawn(GroupObject *g, Object *fn, TValue *args, int nargs);

/* Arm the deadline timer: cancel the children when it fires. */
void kl_group_set_deadline(GroupObject *g, int64_t ns);

/* Called when a child finishes (or is cancelled before starting). */
void kl_group_child_done(GroupObject *g, TaskObject *t);

/* Cancel every live child. */
void kl_group_cancel_children(GroupObject *g);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_GROUP_OBJECT_H_ */


#ifndef _KOALA_TASK_SIGNAL_H_
#define _KOALA_TASK_SIGNAL_H_

#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct task_signal {
	void *msg;
	task_t *waiter;
} task_signal_t;

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_TASK_SIGNAL_H_ */

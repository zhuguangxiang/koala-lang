/*===-- task_event.h - Event For Coroutine ------------------------*- C -*-===*\
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) 2020 James, https://github.com/zhuguangxiang                 *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* This header declares coroutine's event structures and interfaces.          *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_TASK_EVENT_H_
#define _KOALA_TASK_EVENT_H_

#ifdef __cplusplus
extern "C" {
#endif

/* initialize event system */
void init_event(void);

/* poll event by TIME_RESOLUTION_MS interval */
void event_poll(void);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_TASK_EVENT_H_ */

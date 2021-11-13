/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_TASK_EVENT_H_
#define _KOALA_TASK_EVENT_H_

#ifdef __cplusplus
extern "C" {
#endif

/* initialize event system */
void init_event(void);

/* finalize event system */
void fini_event(void);

/* poll event by TIME_RESOLUTION_MS interval */
void event_poll(void);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_TASK_EVENT_H_ */

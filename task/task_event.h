/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangXiang https://github.com/zhuguangxiang               *|
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

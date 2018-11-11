
#if defined(SCHED_LOCKED)
#include "task_scheduler_locked.c"
#elif defined(SCHED_LOCKFREE)
#elif defined(SCHED_WSD)
#else
#endif

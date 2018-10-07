
#if defined(ARCH_x86)
#include "task_atomic_x86.c"
#elif defined(ARCH_x86_64)
#include "task_atomic_x86_64.c"
#else
#error please define atomic functions in task_atomic.h
#endif

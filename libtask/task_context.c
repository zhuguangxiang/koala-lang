
#if defined(SWITCH_x86)
#include "task_context_x86.c"
#elif defined(SWITCH_x86_64)
#include "task_context_x86_64.c"
#elif defined(SWITCH_UCONTEXT)
#include "task_context_ucontext.c"
#else
#error please implement task context functions in task_context.h
#endif

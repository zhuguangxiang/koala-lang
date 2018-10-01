
#ifndef _KOALA_TASK_CONTEXT_H_
#define _KOALA_TASK_CONTEXT_H_

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_STACK_SIZE 8192000

typedef void *(*task_func)(void *);

typedef struct task_context {
  void *ctx_stack;
  int ctx_stacksize;
  void **ctx_stack_ptr;
} task_context_t;

int task_context_init(task_context_t *ctx, task_func entry, void *para);
int task_context_save(task_context_t *ctx);
void task_context_detroy(task_context_t *ctx);
void task_context_swap(task_context_t *from, task_context_t *to);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_TASK_CONTEXT_H_ */

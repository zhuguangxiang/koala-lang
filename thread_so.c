
#include <pthread.h>
#include <stdio.h>

__thread int tval = 200;

void *thread_callback2(void *arg)
{
    printf("thread local2:%d\n", tval);
    return NULL;
}

void *thread_callback_fn(void *arg)
{
    tval = 100;
    pthread_t pid;
    pthread_create(&pid, NULL, thread_callback2, NULL);
    pthread_detach(pid);

    printf("thread local:%d\n", tval);
    return NULL;
}

void call_back(void)
{
    pthread_t pid;
    pthread_create(&pid, NULL, thread_callback_fn, NULL);
    pthread_detach(pid);
}

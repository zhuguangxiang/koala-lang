/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "gc.h"
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/mman.h>
#include <unistd.h>
#include "log.h"
#include "run.h"

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------DATA-----------------------------------*/

/* CMS algorithm */

/* We use like JVM solution to stop the mutators. */
static int _pagesize;
char *__gc_stw_check_ptr;
static int _gc_check_ptr_invalid;

/* gc state */
int __gc_state;

/* waiting for continuing to run after GC_STW */
static pthread_mutex_t _gc_mutex;
static pthread_cond_t _gc_cond;

/* memory allocator */
static size_t _gc_max_size;
static size_t _gc_trigger_size;
static size_t _gc_used_size;
static LLDeque _gc_list;
static LLDeque _gc_missing_list;
static pthread_spinlock_t _gc_spin_lock;

/*-------------------------------------API-----------------------------------*/

static inline void invalid_stw_check_ptr(void)
{
    if (!_gc_check_ptr_invalid) {
        _gc_check_ptr_invalid = 1;
        mprotect(__gc_stw_check_ptr, _pagesize, PROT_NONE);
    }
}

void *gc_alloc(int size, GcMarkFunc mark)
{
    ASSERT(__gc_state != GC_WAIT_STW && __gc_state != GC_WAIT_STW_2 &&
           __gc_state != GC_FULL);

    /* aligned pointer size */
    size_t mm_size = ALIGN_PTR(sizeof(GcHdr) + size);

try_again:

    pthread_spin_lock(&_gc_spin_lock);

    _gc_used_size += mm_size;
    if (_gc_used_size >= _gc_trigger_size) {
        __gc_state = GC_WAIT_STW;
        invalid_stw_check_ptr();
    }

    if (_gc_used_size >= _gc_max_size) {
        __gc_state = GC_FULL;
        invalid_stw_check_ptr();
    }

    pthread_spin_unlock(&_gc_spin_lock);

    if (__gc_state == GC_WAIT_STW || __gc_state == GC_FULL) {
        /* waiting for resuming to run */
        pthread_mutex_lock(&_gc_mutex);
        while (__gc_state == GC_WAIT_STW || __gc_state == GC_WAIT_STW_2 ||
               __gc_state == GC_FULL) {
            pthread_cond_wait(&_gc_cond, &_gc_mutex);
        }
        pthread_mutex_unlock(&_gc_mutex);
        goto try_again;
    }

    GcHdr *hdr = malloc(mm_size);
    ASSERT(hdr);
    lldq_init_node(&hdr->link);
    hdr->mark = mark;
    hdr->size = size;
    hdr->age = 0;
    if (__gc_state == GC_DONE) {
        hdr->color = GC_COLOR_WHITE;
        lldq_push_tail(&_gc_list, &hdr->link);
    } else {
        hdr->color = GC_COLOR_BLACK;
        lldq_push_tail(&_gc_missing_list, &hdr->link);
    }
    return (void *)(hdr + 1);
}

static void segment_fault_handler(int sig, siginfo_t *si, void *unused)
{
    ASSERT(__gc_state == GC_WAIT_STW || __gc_state == GC_WAIT_STW_2 ||
           __gc_state == GC_FULL);

    ThreadState *ts = __ts();
    ASSERT(ts->state == TS_RUNNING);
    ts->state = TS_GC_STW;

    log_error("Got SIGSEGV at address: %p, from: Thread-%d", si->si_addr,
              ts->id);
    if (si->si_addr != __gc_stw_check_ptr) {
        log_fatal("Segmentation fault\n");
        abort();
    }

    /* waiting for resuming to run */
    pthread_mutex_lock(&_gc_mutex);
    while (__gc_state == GC_WAIT_STW || __gc_state == GC_WAIT_STW_2 ||
           __gc_state == GC_FULL) {
        pthread_cond_wait(&_gc_cond, &_gc_mutex);
    }
    pthread_mutex_unlock(&_gc_mutex);

    ts->state = TS_RUNNING;
}

void init_gc_system(size_t max_mem_size, double load_factor)
{
    _pagesize = sysconf(_SC_PAGE_SIZE);
    char *addr =
        mmap(NULL, _pagesize, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (!addr) {
        perror("mmap");
        exit(-1);
    }
    __gc_stw_check_ptr = addr;
    _gc_check_ptr_invalid = 0;

    /* prepare segment fault handler */
    struct sigaction sa = { 0 };

    sa.sa_flags = SA_SIGINFO;
    if (sigemptyset(&sa.sa_mask)) {
        perror("sigemptyset");
        exit(-1);
    }

    sa.sa_sigaction = segment_fault_handler;

    if (sigaction(SIGSEGV, &sa, NULL)) {
        perror("sigaction");
        exit(-1);
    }

    __gc_state = GC_DONE;

    pthread_mutex_init(&_gc_mutex, NULL);
    pthread_cond_init(&_gc_cond, NULL);

    _gc_used_size = 0;
    lldq_init(&_gc_list);
    lldq_init(&_gc_missing_list);
    pthread_spin_init(&_gc_spin_lock, 0);

    _gc_max_size = max_mem_size;
    _gc_trigger_size = (size_t)(max_mem_size * load_factor);
}

#ifdef __cplusplus
}
#endif
/*
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

static const char *_gc_state_strs[] = {
    "GC_DONE", "GC_MARK_ROOTS", "GC_CO_MARK", "GC_REMARK", "GC_CO_SWEEP", "GC_FULL",
};

/* We use like JVM solution to stop the mutators. */
static int _pagesize;
volatile char *__gc_check_ptr;

/* gc state */
static volatile GcState _gc_state;

/* alloc failed reason */
static volatile int _failed_minor;
static volatile int _failed_major;

/* waiting for continuing to run after GC_STW */
static pthread_mutex_t _mutator_wait_mutex;
static pthread_cond_t _mutator_wait_cond;
static volatile int _mutator_wait_flag;

/* memory allocator */
static size_t _gc_max_size;
static size_t _gc_minor_size;
static volatile size_t _gc_used_size;

/* protect _gc_state and _gc_used_size */
static pthread_spinlock_t _gc_spin_lock;

/* Objects are allocated in GC_DONE */
static LLDeque _gc_list;
/* Objects are allocated in GC_CO_MARK and GC_CO_SWEEP */
static LLDeque _gc_remark_list;
/* Objects are permanent, never reclaimed. */
static LLDeque _gc_perm_list;

/* gc worker thread */
static sem_t _gc_worker_sema;
static pthread_t _gc_pid;

/*-------------------------------------API-----------------------------------*/

static inline void clear_failed(void)
{
    _failed_minor = 0;
    _failed_major = 0;
}

/* clang-format off */
#define _switch(new_state) do { \
    log_info("[Collector]%s -> %s", _gc_state_strs[_gc_state], \
             _gc_state_strs[new_state]); \
    _gc_state = new_state; \
} while (0)
/* clang-format on */

static inline void gc_worker_wait(void) { sem_wait(&_gc_worker_sema); }
static inline void gc_worker_wakeup(void) { sem_post(&_gc_worker_sema); }

static inline void mutator_wait(void)
{
    pthread_mutex_lock(&_mutator_wait_mutex);
    while (_mutator_wait_flag) {
        pthread_cond_wait(&_mutator_wait_cond, &_mutator_wait_mutex);
    }
    pthread_mutex_unlock(&_mutator_wait_mutex);
}

static inline void enable_stw(void)
{
    pthread_mutex_lock(&_mutator_wait_mutex);
    _mutator_wait_flag = 1;
    mprotect((void *)__gc_check_ptr, _pagesize, PROT_NONE);
    pthread_mutex_unlock(&_mutator_wait_mutex);
    log_info("[Collector][%s]Enable STW", _gc_state_strs[_gc_state]);
}

static inline void disable_stw_wakeup_threads(void)
{
    pthread_mutex_lock(&_mutator_wait_mutex);

    log_info("[Collector][%s]Disable STW and wakeup mutators", _gc_state_strs[_gc_state]);
    _mutator_wait_flag = 0;
    mprotect((void *)__gc_check_ptr, _pagesize, PROT_READ);
    pthread_cond_broadcast(&_mutator_wait_cond);

    pthread_mutex_unlock(&_mutator_wait_mutex);
}

#define gc_incr_age(obj) ++((GcObject *)(obj))->gc_age

static void *free_obj(GcObject *obj)
{
    pthread_spin_lock(&_gc_spin_lock);
    _gc_used_size -= obj->gc_size;
    pthread_spin_unlock(&_gc_spin_lock);
    mm_free(obj);
}

static GcObject *alloc_obj(int size, int minor, int perm)
{
    ThreadState *ts = __ts;
    UNUSED(ts);

    pthread_spin_lock(&_gc_spin_lock);

    size_t used = _gc_used_size + size;

    if (used >= _gc_max_size) {
        log_info("[Mutator]Thread-%d failed, Major, used: %ld", ts->id, used);
        ++_failed_major;
        goto exit;
    } else if (minor && used >= _gc_minor_size) {
        log_info("[Mutator]Thread-%d failed, Minor, used: %ld", ts->id, used);
        ++_failed_minor;
        goto exit;
    } else {
        /* use memory normally */
        _gc_used_size = used;
        pthread_spin_unlock(&_gc_spin_lock);
    }

    GcObject *obj = malloc(size);
    ASSERT(obj);
    lldq_node_init(&obj->gc_link);
    obj->gc_size = size;
    // default kind
    obj->gc_kind = GC_KIND_OBJECT;
    if (perm) {
        obj->gc_age = -1;
        obj->gc_color = GC_COLOR_WHITE;
        lldq_push_tail(&_gc_perm_list, &obj->gc_link);
        log_info("[Mutator]Thread-%d, permanent object, size: %ld", ts->id, size);
    } else {
        obj->gc_age = 0;
        if (_gc_state == GC_DONE) {
            obj->gc_color = GC_COLOR_WHITE;
            lldq_push_tail(&_gc_list, &obj->gc_link);
        } else {
            obj->gc_color = GC_COLOR_BLACK;
            lldq_push_tail(&_gc_remark_list, &obj->gc_link);
        }
    }

    log_info("[Mutator]Thread-%d, successful, size: %ld", ts->id, size);

    return obj;

exit:
    pthread_spin_unlock(&_gc_spin_lock);
    return NULL;
}

void *_gc_alloc(int size, int perm)
{
    ThreadState *ts = __ts;
    ASSERT(ts->state == TS_RUNNING);

    /* aligned pointer size */
    int mm_size = ALIGN_PTR(size);
    GcObject *obj = NULL;

    /* simple fsm */
    while (1) {
        switch (_gc_state) {
            case GC_DONE: {
                /* normal case */
                obj = alloc_obj(mm_size, 1, perm);
                if (!obj) goto suspend;
                goto done;
            }
            case GC_CO_MARK: /* fall-through */
            case GC_CO_SWEEP: {
                /* normal case */
                obj = alloc_obj(mm_size, 0, perm);
                if (!obj) goto suspend;
                goto done;
            }
            case GC_MARK_ROOTS: /* fall-through */
            case GC_REMARK: /* fall-through */
            case GC_FULL: {
                goto suspend;
            }
            default: {
                ASSERT(0);
                break;
            }
        }

    suspend:
        ts->state = TS_GC_STW;
        log_info("[Mutator]Thread-%d is suspend", ts->id);
        gc_worker_wakeup();
        mutator_wait();
        ts->state = TS_RUNNING;
        log_info("[Mutator]Thread-%d is running", ts->id);
    }

done:

    return obj;
}

void *gc_alloc_array(char kind, size_t len)
{
    static size_t sizes[] = {
        0,
        sizeof(int8_t),
        sizeof(int64_t),
        sizeof(double),
        sizeof(Object *),
        sizeof(Value),
    };
    ASSERT(kind >= GC_KIND_ARRAY_INT8 && kind <= GC_KIND_ARRAY_VALUE);
    int size = sizeof(GcObject) + len * sizes[kind];
    GcObject *obj = gc_alloc(size);
    obj->gc_kind = kind;
    return obj;
}

static void gc_segment_fault_handler(int sig, siginfo_t *si, void *unused)
{
    ThreadState *ts = __ts;
    ASSERT(ts->state == TS_RUNNING);

    if (!_mutator_wait_flag) return;

    log_warn("[Mutator][Signal]Thread-%d got SIGSEGV at address: %p", ts->id,
             si->si_addr);
    if (si->si_addr != __gc_check_ptr) {
        log_fatal("Segmentation fault\n");
        abort();
    }

    /* waiting for resuming to run */
    ts->state = TS_GC_STW;
    log_info("[Mutator][Signal]Thread-%d is suspend", ts->id);

    gc_worker_wakeup();
    mutator_wait();

    ts->state = TS_RUNNING;
    log_info("[Mutator][Signal]Thread-%d is running", ts->id);
}

static void *gc_pthread_func(void *arg)
{
    log_info("[Collector]running");

    QUEUE(que);

    /* simple fsm */
    while (1) {
    main_loop:
        gc_worker_wait();
    next:
        switch (_gc_state) {
            case GC_DONE: {
                if (_failed_major) {
                    clear_failed();
                    _switch(GC_FULL);
                    goto next;
                } else if (_failed_minor) {
                    clear_failed();
                    _switch(GC_MARK_ROOTS);
                    goto next;
                } else {
                    goto main_loop;
                }
            }
            case GC_MARK_ROOTS: {
                enable_stw();
                clear_failed();
                while (!check_all_threads_stw());
                enum_all_roots(&que);
                _switch(GC_CO_MARK);
                goto next;
            }
            case GC_CO_MARK: {
                disable_stw_wakeup_threads();
                /* ignore minor failed */
                if (_failed_major) {
                    _switch(GC_FULL);
                } else {
                    _switch(GC_REMARK);
                }
                clear_failed();
                goto next;
            }
            case GC_REMARK: {
                enable_stw();
                clear_failed();
                while (!check_all_threads_stw());
                _switch(GC_CO_SWEEP);
                goto next;
            }
            case GC_CO_SWEEP: {
                disable_stw_wakeup_threads();

                /* ignore minor failed */
                if (_failed_major) {
                    clear_failed();
                    _switch(GC_FULL);
                    goto next;
                }

                LLDqNode *node = lldq_pop_head(&_gc_list);
                while (node) {
                    GcObject *obj = (GcObject *)node;
                    if (obj->gc_color == GC_COLOR_WHITE) {
                        free_obj(obj);
                    } else if (obj->gc_color == GC_COLOR_BLACK) {
                        _gc_mark(obj, GC_COLOR_WHITE);
                        gc_incr_age(obj);
                    } else {
                        ASSERT(0);
                    }

                    if (_failed_major) {
                        clear_failed();
                        _switch(GC_FULL);
                        goto next;
                    }

                    node = lldq_pop_head(&_gc_list);
                }

                node = lldq_pop_head(&_gc_remark_list);
                while (node) {
                    GcObject *obj = (GcObject *)node;
                    ASSERT(obj->gc_color == GC_COLOR_BLACK);
                    _gc_mark(obj, GC_COLOR_WHITE);
                    gc_incr_age(obj);
                    lldq_push_tail(&_gc_list, node);

                    if (_failed_major) {
                        clear_failed();
                        _switch(GC_FULL);
                        goto next;
                    }

                    node = lldq_pop_head(&_gc_remark_list);
                }

                _switch(GC_DONE);
                goto next;
            }
            case GC_FULL: {
                enable_stw();
                clear_failed();
                while (!check_all_threads_stw());

                LLDqNode *node = lldq_pop_head(&_gc_list);
                while (node) {
                    GcObject *obj = (GcObject *)node;
                    if (obj->gc_color == GC_COLOR_WHITE) {
                        free_obj(obj);
                    } else if (obj->gc_color == GC_COLOR_BLACK) {
                        _gc_mark(obj, GC_COLOR_WHITE);
                        gc_incr_age(obj);
                    } else {
                        ASSERT(0);
                    }
                    node = lldq_pop_head(&_gc_list);
                }

                node = lldq_pop_head(&_gc_remark_list);
                while (node) {
                    GcObject *obj = (GcObject *)node;
                    ASSERT(obj->gc_color == GC_COLOR_BLACK);
                    _gc_mark(obj, GC_COLOR_WHITE);
                    gc_incr_age(obj);
                    lldq_push_tail(&_gc_list, node);
                    node = lldq_pop_head(&_gc_remark_list);
                }

                _switch(GC_DONE);
                disable_stw_wakeup_threads();
                goto next;
            }
            default: {
                ASSERT(0);
                break;
            }
        }
    }

    log_info("[Collector]done");
    return NULL;
}

void init_gc_system(size_t max_mem_size, double factor)
{
    _pagesize = sysconf(_SC_PAGE_SIZE);
    char *addr = mmap(NULL, _pagesize, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (!addr) {
        perror("mmap");
        exit(-1);
    }
    __gc_check_ptr = addr;

    /* prepare segment fault handler */
    struct sigaction sa = { 0 };

    sa.sa_flags = SA_SIGINFO;
    if (sigemptyset(&sa.sa_mask)) {
        perror("sigemptyset");
        exit(-1);
    }

    sa.sa_sigaction = gc_segment_fault_handler;

    if (sigaction(SIGSEGV, &sa, NULL)) {
        perror("sigaction");
        exit(-1);
    }

    _gc_state = GC_DONE;
    _failed_minor = 0;
    _failed_major = 0;

    pthread_mutex_init(&_mutator_wait_mutex, NULL);
    pthread_cond_init(&_mutator_wait_cond, NULL);
    _mutator_wait_flag = 0;

    _gc_max_size = max_mem_size;
    _gc_minor_size = (size_t)(max_mem_size * factor);
    _gc_used_size = 0;

    pthread_spin_init(&_gc_spin_lock, 0);

    lldq_init(&_gc_list);
    lldq_init(&_gc_remark_list);
    lldq_init(&_gc_perm_list);

    sem_init(&_gc_worker_sema, 0, 0);
    pthread_create(&_gc_pid, NULL, gc_pthread_func, NULL);
}

void fini_gc_system(void) {}

#ifdef __cplusplus
}
#endif

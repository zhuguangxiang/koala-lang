
#include "log.h"
#include "thread.h"
#include <unistd.h>
#include <errno.h>

struct scheduler sched;

static inline void init_task_ucontext(ucontext_t *ctx)
{
	getcontext(ctx);
	ctx->uc_link = NULL;
	ctx->uc_stack.ss_sp = malloc(TASK_STACK_SIZE);
	ctx->uc_stack.ss_size = TASK_STACK_SIZE;
	ctx->uc_stack.ss_flags = 0;
}

static inline void free_task_ucontext(ucontext_t *ctx)
{
	free(ctx->uc_stack.ss_sp);
}

void task_wrapper(struct task *tsk)
{
	tsk->run(tsk);
	task_exit(tsk);
}

int task_init(struct task *tsk, char *name, short prio,
							task_func run, void *arg)
{
	memcpy(tsk->name, name, NAME_SIZE - 1);
	tsk->name[NAME_SIZE - 1] = 0;
	init_list_head(&tsk->link);
	init_task_ucontext(&tsk->ctx);
	makecontext(&tsk->ctx, (void (*)(void))task_wrapper, 1, tsk);
	tsk->state = STATE_READY;
	tsk->prio = prio;
	tsk->run = run;
	tsk->arg = arg;
	tsk->thread = NULL;
	pthread_mutex_lock(&sched.lock);
	tsk->id = ++sched.idgen;
	list_add_tail(&tsk->link, &sched.readylist[tsk->prio]);
	pthread_cond_signal(&sched.cond);
	pthread_mutex_unlock(&sched.lock);
	return 0;
}

static void task_add_sleeplist(struct task *tsk)
{
	struct task *t;
	list_for_each_entry(t, &sched.sleeplist, link) {
		if (t->sleep > tsk->sleep)
			break;
	}
	__list_add(&tsk->link, t->link.prev, &t->link);
}

void task_sleep(struct task *tsk, int second)
{
	ASSERT(tsk->state == STATE_RUNNING);
	ASSERT(list_unlinked(&tsk->link));
	tsk->state = STATE_SUSPEND;
	tsk->sleep = sched.clock + second;
	pthread_mutex_lock(&sched.sleeplock);
	task_add_sleeplist(tsk);
	pthread_mutex_unlock(&sched.sleeplock);
	struct thread *thread = tsk->thread;
	ASSERT_PTR(thread);
	swapcontext(&tsk->ctx, &thread->ctx);
}

void task_exit(struct task *tsk)
{
	ASSERT(tsk->state == STATE_RUNNING);
	ASSERT(list_unlinked(&tsk->link));
	tsk->state = STATE_DEAD;
	struct thread *thread = tsk->thread;
	ASSERT_PTR(thread);
	setcontext(&thread->ctx);
}

void task_yield(struct task *tsk)
{
	ASSERT(tsk->state == STATE_RUNNING);
	ASSERT(list_unlinked(&tsk->link));
	tsk->state = STATE_READY;
	pthread_mutex_lock(&sched.lock);
	list_add_tail(&tsk->link, &sched.readylist[tsk->prio]);
	pthread_cond_signal(&sched.cond);
	pthread_mutex_unlock(&sched.lock);
	struct thread *thread = tsk->thread;
	ASSERT_PTR(thread);
	swapcontext(&tsk->ctx, &thread->ctx);
}

void task_suspend(struct task *tsk, int second)
{
	ASSERT(tsk->state == STATE_RUNNING);
	ASSERT(list_unlinked(&tsk->link));
	tsk->state = STATE_SUSPEND;
	tsk->sleep = sched.clock + second;
	pthread_mutex_lock(&sched.sleeplock);
	if (second > 0) {
		task_add_sleeplist(tsk);
	} else {
		list_add_tail(&tsk->link, &sched.suspendlist);
	}
	pthread_mutex_unlock(&sched.sleeplock);
	struct thread *thread = tsk->thread;
	ASSERT_PTR(thread);
	swapcontext(&tsk->ctx, &thread->ctx);
}

static void *task_thread_func(void *arg)
{
	struct thread *thread = arg;
	struct task *tsk;
	struct list_head *node;

	while (1) {
		//printf("%s is running\n", thread->name);
		pthread_mutex_lock(&thread->lock);
		while (!(node = list_first(&thread->runlist))) {
			pthread_cond_wait(&thread->cond, &thread->lock);
		}
		list_del(node);
		thread->runsize--;
		pthread_mutex_unlock(&thread->lock);
		tsk = container_of(node, struct task, link);
		thread->current = tsk;
		tsk->thread = thread;
		swapcontext(&thread->ctx, &tsk->ctx);
		tsk->thread = NULL;
	}

	return NULL;
}

void timer_signal_handler(int signo)
{
	UNUSED_PARAMETER(signo);
	sched.clock++;
	int ret = timer_getoverrun(sched.timer);
	if (ret < 0) {
		error("errno:%d", errno);
		return;
	}
	sched.clock = sched.clock + ret;
	sem_post(&sched.timer_sem);
}

static void *timer_thread_func(void *arg)
{
	UNUSED_PARAMETER(arg);
	while (1) {
		sem_wait(&sched.timer_sem);

		uint64 clock = sched.clock;

		struct list_head *p, *n;
		struct task *tsk;

		pthread_mutex_lock(&sched.sleeplock);
		list_for_each_safe(p, n, &sched.sleeplist) {
			tsk = container_of(p, struct task, link);
			if (tsk->sleep <= clock) {
				list_del(&tsk->link);
				tsk->state = STATE_READY;
				pthread_mutex_lock(&sched.lock);
				list_add_tail(&tsk->link, &sched.readylist[tsk->prio]);
				pthread_cond_signal(&sched.cond);
				pthread_mutex_unlock(&sched.lock);
			} else {
				break;
			}
		}
		pthread_mutex_unlock(&sched.sleeplock);
	}

	return NULL;
}

void sched_timer_init(void)
{
	sem_init(&sched.timer_sem, 0, 0);
	pthread_create(&sched.timer_thread, NULL, timer_thread_func, NULL);

	struct sigaction sa;
	sa.sa_flags = 0;
	sa.sa_handler = timer_signal_handler;
	//sigemptyset(&sa.sa_mask);
	sigaction(SIGUSR1, &sa, NULL);

	struct sigevent se;
	//memset(&se, 0, sizeof(se));
	se.sigev_notify = SIGEV_SIGNAL;
	se.sigev_signo = SIGUSR1;
	timer_create(CLOCK_REALTIME, &se, &sched.timer);

	struct itimerspec ts;
	ts.it_interval.tv_sec = 1;
	ts.it_interval.tv_nsec = 0;
	ts.it_value = ts.it_interval;
	timer_settime(&sched.timer, 0, &ts, NULL);
}

static void task_add_runlist(struct task *tsk)
{
	int id = 0;
	int min = sched.threads[0].runsize;
	int sz;
	for (int i = 1; i < NR_THREADS; i++) {
		sz = sched.threads[i].runsize;
		if (sz < min) {
			min = sz;
			id = i;
		}
	}

	struct thread *thread = &sched.threads[id];

	pthread_mutex_lock(&thread->lock);
	tsk->state = STATE_RUNNING;
	list_add_tail(&tsk->link, &thread->runlist);
	thread->runsize++;
	pthread_cond_signal(&thread->cond);
	pthread_mutex_unlock(&thread->lock);
}

static struct list_head *next_node(struct list_head *head)
{
	struct list_head *node;
	for (int i = 0; i < NR_PRIORITY; i++) {
		node = list_first(head + i);
		if (node) return node;
	}
	return NULL;
}

void *task_mover_thread_func(void *arg)
{
	UNUSED_PARAMETER(arg);
	struct task *tsk;
	struct list_head *node;
	while (1) {
		pthread_mutex_lock(&sched.lock);
		while (!(node = next_node(sched.readylist))) {
			pthread_cond_wait(&sched.cond, &sched.lock);
		}
		list_del(node);
		pthread_mutex_unlock(&sched.lock);
		tsk = container_of(node, struct task, link);
		task_add_runlist(tsk);
	}
}

void sched_init(void)
{
	for (int i = 0; i < NR_PRIORITY; i++)
		init_list_head(&sched.readylist[i]);
	init_list_head(&sched.suspendlist);
	init_list_head(&sched.sleeplist);
	pthread_mutex_init(&sched.lock, NULL);
	pthread_cond_init(&sched.cond, NULL);
	sched.idgen = 0;
	sched_timer_init();
}

void schedule(void)
{
	struct thread *thread;
	for (int i = 0; i < NR_THREADS; i++) {
		thread = sched.threads + i;
		thread->runsize = 0;
		init_list_head(&thread->runlist);
		pthread_mutex_init(&thread->lock, NULL);
		pthread_cond_init(&thread->cond, NULL);
		sprintf(thread->name, "cpu-%d", i);
		pthread_create(&thread->id, NULL, task_thread_func, thread);
	}

	pthread_create(&sched.mover_thread, NULL, task_mover_thread_func, NULL);
}

void thread_forever(void)
{
	while (1) sleep(60);
}

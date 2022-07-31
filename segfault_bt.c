
#include <assert.h>
#include <errno.h>
#include <execinfo.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

struct TLSData {
    int val;
    char *safepoint;
};

__thread struct TLSData data;

int getpagesize(void)
{
    long page_size = sysconf(_SC_PAGESIZE);
    assert(page_size != -1);
    return page_size;
}

static char *statepoint_page;

void init_safepoint_page(void)
{
    // get page size
    int pgsz = getpagesize();

    // map one page
    char *addr =
        (char *)mmap(0, pgsz, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (addr == MAP_FAILED) {
        addr = NULL;
    }

    if (addr == NULL) {
        printf("could not allocate GC synchronization page\n");
        abort();
    }

    statepoint_page = addr;
}

void print_backtrace(void)
{
    char **strings;
    void *array[10];
    int num = backtrace(array, 10);

    printf("backtrace() returned %d addresses\n", num);

    strings = backtrace_symbols(array, num);
    if (strings == NULL) {
        perror("backtrace_symbols");
        return;
    }

    for (int j = 0; j < num; j++) {
        printf("%s\n", strings[j]);
    }

    free(strings);
}

void mprotect_handler(int segno)
{
    mprotect(data.safepoint, getpagesize(), PROT_READ);
    printf("fixup mprotect page fault in pthread:%ld\n", pthread_self());
    // return;
    sleep(10);
    printf("get tls data: %d\n", data.val);
    print_backtrace();
}

static void seg_fault_handler(int sig, siginfo_t *info, void *context)
{
    if (info->si_addr == statepoint_page) {
        printf("safepoint handler\n");
        sleep(1);
        mprotect(statepoint_page, getpagesize(), PROT_READ);
        printf("fixup mprotect page fault in pthread:%ld\n", pthread_self());
        printf("get tls data: %d\n", data.val);
        print_backtrace();
    } else {
        printf("BUG: not safepoint\n");
        abort();
    }
}

void bar(void)
{
    printf("bar called, and will cause page fault\n");
    // read will casue page fault
    char ch = *data.safepoint;
    printf("bar running after mprotect page fault\n");
}

void foo(void)
{
    printf("foo called\n");
    bar();
}

void *thread_entry(void *arg)
{
    printf("pthread :%ld\n", pthread_self());

    data.safepoint = statepoint_page;
    data.val = 200;

    foo();

    return NULL;
}

// -fno-omit-frame-pointer
int main(int argc, char *argv[])
{
    printf("pthread :%ld\n", pthread_self());

    // init TLS data
    init_safepoint_page();

    // page as safepoint page.
    data.safepoint = statepoint_page;
    data.val = 100;

    // get page size
    size_t pgsz = getpagesize();

    printf("page size: %ld\n", pgsz);

    // enable page fault
    mprotect(data.safepoint, pgsz, PROT_NONE);

    // add segment fault handler
    // signal(SIGSEGV, mprotect_handler);
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    sigemptyset(&act.sa_mask);
    act.sa_sigaction = seg_fault_handler;
    act.sa_flags = SA_ONSTACK | SA_SIGINFO;
    if (sigaction(SIGSEGV, &act, NULL) < 0) {
        printf("fatal error: sigaction: %s", strerror(errno));
    }

    pthread_t pid;
    pthread_create(&pid, NULL, thread_entry, NULL);

    // test page fault
    foo();

    return 0;
}

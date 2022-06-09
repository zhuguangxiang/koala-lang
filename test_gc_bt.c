
#include <execinfo.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

int *getHeapRefs(uint64_t *callLocations, int numOfCallLocations);
void readStackMap(uint64_t *t);

struct StackMap {
    int32_t a1;
    int32_t a2;
    int32_t a3;
};

register long rspv asm("rsp");
register long rbpv asm("rbp");

extern struct StackMap *__stackmap_start;

uint8_t *g_new_addr;

void bar()
{
    // char *d = *(char **)arg;
    // printf("address:%p, value:%d\n", *(char **)arg, *d);

    void *array[10];
    int numOfCallLocs = backtrace(array, 10);
    // TODO : Increase the array size if it is not enough

    char **strings;
    printf("backtrace() returned %d addresses\n", numOfCallLocs);

    /* The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
       would produce similar output to the following: */

    strings = backtrace_symbols(array, numOfCallLocs);
    if (strings == NULL) {
        perror("backtrace_symbols");
        exit(EXIT_FAILURE);
    }

    for (int j = 0; j < numOfCallLocs; j++) printf("%s\n", strings[j]);

    free(strings);

    uint64_t *callLocs =
        (uint64_t *)calloc(numOfCallLocs + 1, sizeof(uint64_t));
    *callLocs = numOfCallLocs - 1;
    for (size_t i = 1; i <= numOfCallLocs; i++) {
        *(callLocs + i) = (uint64_t)array[i];
    }

    printf("call loc : %lx\n", *((uint64_t *)callLocs));
    printf("call loc : %lx\n", *((uint64_t *)callLocs + 1));
    printf("call loc : %lx\n", *((uint64_t *)callLocs + 2));
    printf("call loc : %lx\n", *((uint64_t *)callLocs + 3));
    int *offsets = getHeapRefs((uint64_t *)callLocs, *((uint64_t *)callLocs));
    printf("%d\n", *offsets);
    printf("%d\n", *(offsets + 1));
    printf("%d\n", *(offsets + 2));

    printf("(uint8_t*)rsp : %p\n", (uint8_t *)rspv);
    printf("(uint8_t*)rbp : %p\n", (uint8_t *)rbpv);

    uint8_t *foo_rbp = *(uint8_t **)rbpv;
    printf("===%p\n", (((uint64_t *)((uint8_t *)foo_rbp))));

    uint8_t **obj_addr = (uint8_t **)(foo_rbp - 24 + 8 + *(offsets + 1));
    printf("obj_addr===%p, value=%d\n", obj_addr, **obj_addr);
    uint8_t *new_addr = malloc(4);
    printf("obj_addr2===%p\n", new_addr);
    *obj_addr = new_addr;
    g_new_addr = new_addr;

    // printf("===%lx\n", *(((uint64_t *)((uint8_t *)rspv + *(offsets + 1)))));
    /*
    printf("1 : %lu\n",
           *((uint64_t *)*((uint64_t *)((uint8_t *)rsp + *(offsets)))));
    printf("1 : %lu\n",
           *((uint64_t *)*((uint64_t *)((uint8_t *)rbp + *(offsets + 1)))));
    */
}

void foo();

int main(int argc, char *argv[])
{
    readStackMap((uint64_t *)&__stackmap_start);
    foo();
    printf("------------result=%d\n", *g_new_addr);
    return 0;
}

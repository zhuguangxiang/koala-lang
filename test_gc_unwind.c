
#define UNW_LOCAL_ONLY
#include <execinfo.h>
#include <inttypes.h>
#include <libunwind.h>
#include <stdio.h>
#include <stdlib.h>

int *getHeapRefs(uint64_t *callLocations, int numOfCallLocations);
void readStackMap(uint64_t *t);

struct StackMap {
    int32_t a1;
    int32_t a2;
    int32_t a3;
};

extern struct StackMap *__stackmap_start;

uint8_t *g_new_addr;

long sp;

void bar()
{
    unw_cursor_t cursor;
    unw_context_t context;

    // Initialize cursor to current frame for local unwinding.
    unw_getcontext(&context);
    unw_init_local(&cursor, &context);

    void *array[32] = { NULL };
    int i = 1;

    // Unwind frames one by one, going up the frame stack.
    while (unw_step(&cursor) > 0) {
        unw_word_t offset, pc;
        unw_get_reg(&cursor, UNW_REG_IP, &pc);
        if (pc == 0) {
            break;
        }
        printf("0x%lx:", pc);
        array[i++] = (void *)pc;

        if (sp == 0) unw_get_reg(&cursor, UNW_REG_SP, &sp);

        char sym[256];
        if (unw_get_proc_name(&cursor, sym, sizeof(sym), &offset) == 0) {
            printf(" (%s+0x%lx)\n", sym, offset);
        } else {
            printf(" -- error: unable to obtain symbol name for this frame\n");
        }
    }

    int numOfCallLocs = 2;
    uint64_t *callLocs =
        (uint64_t *)calloc(numOfCallLocs + 1, sizeof(uint64_t));
    *callLocs = numOfCallLocs - 1;
    for (size_t i = 1; i <= numOfCallLocs; i++) {
        *(callLocs + i) = (uint64_t)array[i];
    }

    int *offsets = getHeapRefs((uint64_t *)callLocs, *((uint64_t *)callLocs));
    printf("%d\n", *offsets);
    printf("%d\n", *(offsets + 1));
    printf("%d\n", *(offsets + 2));

    uint8_t *foo_rbp = *(uint8_t **)(sp - 24 + 8);

    uint8_t **obj_addr = (uint8_t **)(foo_rbp - *(offsets + 1));

    printf("obj_addr===%p, value=%d\n", *obj_addr, **obj_addr);
    uint8_t *new_addr = malloc(4);
    printf("obj_addr2===%p\n", new_addr);
    *obj_addr = new_addr;
    g_new_addr = new_addr;
}

void foo();

int main(int argc, char *argv[])
{
    readStackMap((uint64_t *)&__stackmap_start);
    foo();
    printf("------------result=%d\n", *g_new_addr);
    return 0;
}
// gcc test_gc_unwind.c stackmap-reader.c test_gc.opt.o

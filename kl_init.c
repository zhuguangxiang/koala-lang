
#include <inttypes.h>
#include <stdio.h>

struct StackMap {
    int32_t a1;
    int32_t a2;
    int32_t a3;
};

void readStackMap(uint64_t *);

extern struct StackMap __stackmap_start;
// extern struct StackMap __stackmap_end;

__attribute__((constructor(101))) static void __kl_init()
{
    printf("stackmap_start: %p\n", &__stackmap_start);
    // printf("stackmap_end: %p\n", &__stackmap_end);
    readStackMap((uint64_t *)&__stackmap_start);
    printf("stackmap_end\n");
}

void __kl_init2()
{
    printf("__kl_init2\n");
}
__asm__(
    ".pushsection .init_array.00102,\"aw\",@init_array; .quad __kl_init2; "
    ".popsection");

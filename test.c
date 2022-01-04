
#include <stdint.h>
#include <stdio.h>

int sum(int n)
{
    int i = 0;
    int sum = 0;
    while (i < n) {
        sum = sum + 1;
        i = i + 1;
    }
    return sum;
}

struct A {
    union {
        uint32_t kind : 2;
        void *ptr;
    };
};

int main()
{
    printf("%d\n", sum(100));

    struct A instance;
    instance.ptr = &instance;
    instance.kind = 2;
    return 0;
}

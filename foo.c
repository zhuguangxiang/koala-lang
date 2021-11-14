
// clang -S -emit-llvm foo.c

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

int main()
{
    printf("%d\n", sum(100));
    return 0;
}

/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 */

#include <stdint.h>
#include <stdio.h>

struct Foo {
    uintptr_t v1;
    uintptr_t v2;
};

struct Foo fib(struct Foo n)
{
    if (n.v2 <= 1) return n;
    struct Foo n1, n2;
    n1.v2 = n.v2 - 1;
    n1 = fib(n1);
    n2.v2 = n.v2 - 2;
    n2 = fib(n2);
    n1.v2 += n2.v2;
    return n1;
}

int main(int argc, char *argv[])
{
    struct Foo n;
    n.v2 = 40;
    n = fib(n);
    printf("%d\n", (int)n.v2);
    printf("%s\n", "汉");
    return 0;
}

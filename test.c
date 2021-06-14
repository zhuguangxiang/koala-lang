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
    // n = fib(n);
    printf("%d\n", (int)n.v2);
    printf("%s\n", "汉");

    /*
        %8s would specify a minimum width of 8 characters. You want to truncate
        at 8, so use %.8s.
    */
    printf("Here are the first 8 chars: %.8s\n",
           "A string that is more than 8 chars");

    const char hello[] = "Hello world";
    printf("message: '%.3s'\n", hello);
    printf("message: '%.*s'\n", 3, hello);
    printf("message: '%.*s'\n", 5, hello);

    printf("Here are the first 8 chars: %*.*s\n", 8, 16,
           "A string that is more than 8 chars");

    return 0;
}


struct Foo {
    char *str;
    int len;
};

#include <assert.h>
#include <stdio.h>

int add(int a, int b)
{
    return 100;
}

int printf2(char *fmt, ...)
{
    assert(0);
    return 0;
}

#define printf printf2

int main()
{
    struct Foo foo;
    foo.str = "hello";
    foo.len = 5;
    printf("Foo(%s, %d)\n", foo.str, foo.len);
    int i = 300;
    char c = (char)i;
    // c = sleep(1);
    printf("%d", c);
    return 0;
}

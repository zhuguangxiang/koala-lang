
struct Foo {
    char *str;
    int len;
};

#include <stdio.h>

int main()
{
    struct Foo foo;
    foo.str = "hello";
    foo.len = 5;
    printf("Foo(%s, %d)\n", foo.str, foo.len);
    return 0;
}

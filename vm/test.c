#include <stdio.h>

typedef signed char KlInt8 __attribute__((aligned(4)));
typedef signed short KlInt16 __attribute__((aligned(4)));
typedef signed int KlInt32 __attribute__((aligned(4)));
typedef signed long KlInt64 __attribute__((aligned(4)));

struct Foo {
    KlInt16 n1;
    KlInt8 n2;
    KlInt32 n3;
    KlInt8 n4;
    KlInt64 n5;
};

struct Bar {
    KlInt8 n1;
    struct Foo foo;
};

int main()
{
    printf("sizeof:%ld\n", sizeof(struct Bar));
    return 0;
}

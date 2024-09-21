
#include "object.h"

Value fib(Value n)
{
    int64_t v = n.ival;
    if (v <= 1) return n;

    Value v1 = IntValue(v - 1);
    Value v2 = IntValue(v - 2);
    v1 = fib(v1);
    v2 = fib(v2);
    return IntValue(v1.ival + v2.ival);
}

int main()
{
    Value v = IntValue(40);
    v = fib(v);
    printf("result: %ld\n", v.ival);
    return 0;
}

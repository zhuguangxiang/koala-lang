
#include <assert.h>
#include <stdio.h>

struct State {
    int top;
    int stack[160];
};

static inline void state_push(struct State *st, int v)
{
    st->stack[++st->top] = v;
}

static inline int state_pop(struct State *st)
{
    // assert(st->top >= 0);
    int v = st->stack[st->top];
    --st->top;
    return v;
}

void demo_fib(struct State *st)
{
    int v = state_pop(st);
    if (v < 2) {
        state_push(st, v);
        return;
    }

    state_push(st, v - 1);
    demo_fib(st);

    int v1 = state_pop(st);

    state_push(st, v - 2);
    demo_fib(st);

    int v2 = state_pop(st);

    state_push(st, v1 + v2);
}

int main()
{
    struct State st = { -1, { 0 } };
    state_push(&st, 40);
    demo_fib(&st);
    int v = state_pop(&st);
    printf("fib(40) = %d\n", v);
    return 0;
}

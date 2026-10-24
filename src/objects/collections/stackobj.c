/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "collections/stackobj.h"
#include "excobj.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Elements are stored by value (TValue); the top is the last slot. */
#define STACK_ELEM_SIZE ((int)sizeof(TValue))

/* Stacks longer than STACK_FMT_MAX only show STACK_FMT_EDGE items at each end. */
#define STACK_FMT_MAX  10
#define STACK_FMT_EDGE 4

// __init__()
static TValue _stack_init(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    StackObject *stack = SELF_AS(stack_type);
    vector_init(&stack->vec, STACK_ELEM_SIZE);
    return no_value;
}

// len() -> int
static TValue _stack_len(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    StackObject *stack = SELF_AS(stack_type);
    return int64_value(vector_size(&stack->vec));
}

// empty() -> bool
static TValue _stack_empty(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    StackObject *stack = SELF_AS(stack_type);
    return bool_value(vector_empty(&stack->vec));
}

// push(v T)
static TValue _stack_push(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    StackObject *stack = SELF_AS(stack_type);
    vector_push_back(&stack->vec, &args[0]);
    return no_value;
}

// pop() -> T
static TValue _stack_pop(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    StackObject *stack = SELF_AS(stack_type);

    if (vector_empty(&stack->vec)) {
        raise_exc_str("pop from empty stack");
        return error_value;
    }

    TValue value;
    vector_pop_back(&stack->vec, &value);
    return value;
}

// peek() -> T
static TValue _stack_peek(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    StackObject *stack = SELF_AS(stack_type);

    TValue *top = vector_top_back(&stack->vec);
    if (top == NULL) {
        raise_exc_str("peek from empty stack");
        return error_value;
    }

    return *top;
}

// clear()
static TValue _stack_clear(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    StackObject *stack = SELF_AS(stack_type);
    vector_clear(&stack->vec);
    return no_value;
}

// fmt(f Formatter)
static TValue _stack_fmt(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    StackObject *stack = SELF_AS(stack_type);
    Formatter *fmt = kl_arg_fmt(0);

    int size = vector_size(&stack->vec);

    buf_write_str(&fmt->buf, "Stack([");

    if (size <= STACK_FMT_MAX) {
        for (int i = 0; i < size; ++i) {
            if (i > 0) buf_write_str(&fmt->buf, ", ");
            kl_call_fmt((TValue *)vector_get_ptr(&stack->vec, i), fmt);
        }
    } else {
        /* Show the first STACK_FMT_EDGE and the last STACK_FMT_EDGE items. */
        for (int i = 0; i < STACK_FMT_EDGE; ++i) {
            if (i > 0) buf_write_str(&fmt->buf, ", ");
            kl_call_fmt((TValue *)vector_get_ptr(&stack->vec, i), fmt);
        }

        buf_write_str(&fmt->buf, ", ..., ");

        for (int i = size - STACK_FMT_EDGE; i < size; ++i) {
            if (i > size - STACK_FMT_EDGE) buf_write_str(&fmt->buf, ", ");
            kl_call_fmt((TValue *)vector_get_ptr(&stack->vec, i), fmt);
        }
    }

    buf_write_str(&fmt->buf, "])");
    return no_value;
}

static MethodDef stack_methods[] = {
    { "__init__", _stack_init },
    { "len", _stack_len },
    { "empty", _stack_empty },
    { "push", _stack_push },
    { "pop", _stack_pop },
    { "peek", _stack_peek },
    { "clear", _stack_clear },
    { "fmt", _stack_fmt },
    { NULL },
};

/* pub class Stack[T] { ... } */
TypeObject stack_type = {
    ._type = &type_type,
    .name = "Stack",
    .flags = TP_FLAGS_CLASS,
    .objsize = sizeof(StackObject),
    .methdefs = stack_methods,
};

Object *kl_new_stack(void)
{
    StackObject *stack = mm_alloc_obj(stack);
    INIT_OBJECT_HEAD(stack, &stack_type, 0);
    vector_init(&stack->vec, STACK_ELEM_SIZE);
    return (Object *)stack;
}

#ifdef __cplusplus
}
#endif
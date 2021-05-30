/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 */

#include <stdio.h>
#include "util/list.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _Foo {
    List node;
    int bar;
} Foo;

#define foo_first(list)     list_first(list, Foo, node)
#define foo_next(pos, list) list_next(pos, node, list)
#define foo_prev(pos, list) list_prev(pos, node, list)
#define foo_last(list)      list_last(list, Foo, node)

void test_doubly_linked_list(void)
{
    List foo_list = LIST_INIT(foo_list);
    Foo foo1 = { LIST_INIT(foo1.node), 100 };
    Foo foo2 = { LIST_INIT(foo2.node), 200 };
    list_push_back(&foo_list, &foo1.node);
    list_push_back(&foo_list, &foo2.node);

    Foo *foo;
    list_foreach(foo, node, &foo_list, printf("%d\n", foo->bar));

    Foo *nxt;
    list_foreach_safe(foo, nxt, node, &foo_list, {
        printf("%d is removed\n", foo->bar);
        list_remove(&foo->node);
    });

    list_pop_back(&foo_list);

    for (foo = foo_last(&foo_list); foo; foo = foo_prev(foo, &foo_list)) {
        printf("%d\n", foo->bar);
    }
}

int main(int argc, char *argv[])
{
    test_doubly_linked_list();
    return 0;
}

#ifdef __cplusplus
}
#endif

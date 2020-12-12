/*===-- test_list.c -----------------------------------------------*- C -*-===*\
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) 2020 James, https://github.com/zhuguangxiang                 *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* Test doubly linked list in `list.h`                                        *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "list.h"
#include <stdio.h>

struct foo {
    ListNode node;
    int bar;
};

void test_doubly_linked_list(void)
{
    List foo_list = LIST_INITIALIZER(foo_list);
    struct foo foo1 = { LIST_NODE_INITIALIZER(foo1.node), 100 };
    struct foo foo2 = { LIST_NODE_INITIALIZER(foo2.node), 200 };
    list_push_back(&foo_list, &foo1.node);
    list_push_back(&foo_list, &foo2.node);
    printf("count:%d\n", list_size(&foo_list));
}

int main(int argc, char *argv[])
{
    test_doubly_linked_list();
    return 0;
}

/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2020-2021 James <zhuguangxiang@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _Foo {
    List node;
    int bar;
} Foo;

#define foo_first(list)     ListFirst(list, Foo, node)
#define foo_next(pos, list) ListNext(pos, node, list)
#define foo_prev(pos, list) ListPrev(pos, node, list)
#define foo_last(list)      ListLast(list, Foo, node)

void test_doubly_linked_list(void)
{
    List foo_list = LIST_INIT(foo_list);
    Foo foo1 = { LIST_INIT(foo1.node), 100 };
    Foo foo2 = { LIST_INIT(foo2.node), 200 };
    ListPushBack(&foo_list, &foo1.node);
    ListPushBack(&foo_list, &foo2.node);

    Foo *foo;
    ListForEach(foo, node, &foo_list, printf("%d\n", foo->bar));

    Foo *nxt;
    ListForEachSafe(foo, nxt, node, &foo_list, {
        printf("%d is removed\n", foo->bar);
        ListRemove(&foo->node);
    });

    ListPopBack(&foo_list);

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

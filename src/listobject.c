/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "listobject.h"
#include "intobject.h"

#ifdef __cplusplus
extern "C" {
#endif

// static Value _list_alloc(Value *args, int nargs)
// {
//     ListObject *list = gc_alloc_obj(list);
//     TypeObject *gt = args->obj;
//     //gc_stack_push(list);
//     if (gt == &int_type) {
//         // for slice, if there is no slice, use mm_alloc.
//         list->array = gc_alloc_array(GC_KIND_INT64, 64);
//     }
//     gc_stack_pop(list);
//     return obj_value(list);
// }

/*
public func __init__(v iterable[T]) { ... }
*/
// static int _list_init(Value *self, Value *args, int nargs)
// {
//     ListObject *list = self->obj;
// }

// static MethodDef _list_methods[] = {

// };

/*
public final class list[T] : iterable[T] { ... }
*/
TypeObject list_type = {
    OBJECT_HEAD_INIT(&type_type), .name = "list",
    .flags = TP_FLAGS_CLASS | TP_FLAGS_FINAL,
    // .alloc = _list_alloc,
    // .init = _list_init,
};

#ifdef __cplusplus
}
#endif

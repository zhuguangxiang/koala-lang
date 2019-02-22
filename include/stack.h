/*
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _KOALA_STACK_H_
#define _KOALA_STACK_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct stack {
  int top;
  int size;
  void **objs;
} Stack;

#define Init_Stack(stk, stksize, _objs) \
({ \
  (stk)->top = -1; \
  (stk)->size = stksize; \
  (stk)->objs = (void **)_objs; \
})

#define Stack_Top(stk) \
({ \
  assert(stk->top >= -1 && stk->top <= stk->size - 1); \
  (stk->top >= 0) ? stk->objs[stk->top] : NULL; \
})

#define Stack_Pop(stk) \
({ \
  assert(stk->top >= -1 && stk->top <= stk->size - 1); \
  (stk->top >= 0) ? stk->objs[stk->top--] : NULL; \
})

#define Stack_Push(stk, v) \
({ \
  assert(stk->top >= -1 && stk->top <= stk->size - 1); \
  stk->objs[++stk->top] = v; \
})

#define Stack_Size(stk) \
({ \
  assert(stk->top >= -1 && stk->top <= stk->size - 1); \
  stk->top + 1; \
})

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_STACK_H_ */

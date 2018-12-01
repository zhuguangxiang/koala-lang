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

#ifndef _KOALA_VM_H_
#define _KOALA_VM_H_

#include "state.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STACK_SIZE 8

struct stack {
	/* stack top index */
	int top;
	/* stack */
	Object *stack[STACK_SIZE];
};

struct frame;

typedef struct koalastate {
	/* koalsstate listnode linked in GlobalState->kslist */
	struct list_head ksnode;
	/* point to GlobalState */
	GlobalState *gs;
	/* stack for calculating */
	struct stack stack;
	/* top function frame */
	struct frame *frame;
	/* local objects for this koalastate  */
	Object *map;
} KoalaState;

struct frame {
	/* back frame */
	struct frame *back;
	/* koalastate */
	KoalaState *ks;
	/* function object */
	CodeObject *code;
	/* instruction's index in the function */
	int pc;
	/* local variables' count */
	int size;
	/* local variables memory */
	Object *locvars[0];
};

int Koala_Run_Code(KoalaState *ks, Object *code, Object *ob, Object *args);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_VM_H_ */

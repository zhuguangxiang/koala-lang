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

#ifndef _KLVM_VALUE_H_
#define _KLVM_VALUE_H_

#if !defined(_KLVM_H_INSIDE_) && !defined(KLVM_COMPILATION)
#error "Only <klvm.h> can be included directly."
#endif

#include "klvm/klvm_type.h"
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------*\
|* Value: Constant, Variable, Function, Instruction, Class, Intf, Enum ...  *|
\*--------------------------------------------------------------------------*/

typedef enum _KLVMValueKind {
    KLVM_VALUE_CONST = 1,
    KLVM_VALUE_VAR,
    KLVM_VALUE_FUNC,
    KLVM_VALUE_INST,
    KLVM_VALUE_CLASS,
    KLVM_VALUE_INTF,
    KLVM_VALUE_ENUM,
    KLVM_VALUE_CLOSURE,
} KLVMValueKind;

// clang-format off

#define KLVM_VALUE_HEAD \
    KLVMValueKind kind; KLVMTypeRef type; const char *name; \
    int tag; int pub; int final;

#define INIT_KLVM_VALUE_HEAD(val, _kind, _type, _name) \
    val->kind = _kind; val->type = _type; val->name = _name; \
    val->tag = -1; val->pub = 0; val->final = 0

// clang-format on

typedef struct _KLVMValue {
    KLVM_VALUE_HEAD
} KLVMValue, *KLVMValueRef;

/* Get value name */
#define KLVMGetName(val) ((val)->name)

/* Get value type */
#define KLVMGetType(val) ((val)->type)

/* Set value name */
#define KLVMSetName(val, _name) (val)->name = _name

/* Set value as final */
#define KLVMSetFinal(val) (val)->final = 1

/* Value is final */
#define KLVMIsFinal(val) (val)->final

/* Set value as public */
#define KLVMSetPub(val) (val)->pub = 1

/* Value is public */
#define KLVMIsPub(val) (val)->pub

/*--------------------------------------------------------------------------*\
|* Module: values container                                                 *|
\*--------------------------------------------------------------------------*/

typedef struct _KLVMModule {
    char *name;
    Vector items;
    KLVMValueRef fn;
} KLVMModule, *KLVMModuleRef;

/* Create a new module */
KLVMModuleRef KLVMCreateModule(char *name);

/* Destroy a module */
void KLVMDestroyModule(KLVMModuleRef m);

/* Print a module */
void KLVMPrintModule(KLVMModuleRef m, FILE *fp);

/* Dump a module */
#define KLVMDumpModule(m)       \
    KLVMPrintModule(m, stdout); \
    fflush(stdout)

/* Get '__init__' func */
KLVMValueRef KLVMGetInitFunc(KLVMModuleRef m);

/* Add a variable to a module */
KLVMValueRef KLVMAddVar(KLVMModuleRef m, char *name, KLVMTypeRef ty);

/* Add a function to a module */
KLVMValueRef KLVMAddFunc(KLVMModuleRef m, char *name, KLVMTypeRef ty);

/* Add a class to a module */
KLVMValueRef KLVMAddClass(KLVMModuleRef m, char *name, KLVMValueRef base,
                          VectorRef traits);

/*--------------------------------------------------------------------------*\
|* Literal constant                                                         *|
\*--------------------------------------------------------------------------*/

typedef struct _KLVMConst {
    KLVM_VALUE_HEAD
    union {
        int8_t i8val;
        int16_t i16val;
        int32_t i32val;
        int64_t i64val;
        float f32val;
        double f64val;
        void *ptr;
    };
} KLVMConst, *KLVMConstRef;

KLVMValueRef KLVMConstInt8(int8_t v);
KLVMValueRef KLVMConstInt16(int16_t v);
KLVMValueRef KLVMConstInt32(int32_t v);
KLVMValueRef KLVMConstInt64(int64_t v);
KLVMValueRef KLVMConstFloat32(float v);
KLVMValueRef KLVMConstFloat64(double v);
KLVMValueRef KLVMConstBool(int8_t v);
KLVMValueRef KLVMConstChar(int32_t ch);
KLVMValueRef KLVMConstStr(char *s);

/*--------------------------------------------------------------------------*\
|* Variable                                                                 *|
\*--------------------------------------------------------------------------*/

typedef struct _KLVMVar {
    KLVM_VALUE_HEAD
    int readonly;
    int local;
} KLVMVar, *KLVMVarRef;

/*--------------------------------------------------------------------------*\
|* Basic Block                                                              *|
\*--------------------------------------------------------------------------*/

typedef struct _KLVMBlock {
    List bb_link;
    KLVMValueRef fn;
    char *label;
    // 'start' and 'end' blocks are dummy block
    short dummy;
    short tag;
    int8_t visited;
    int num_insts;
    List insts;
    List in_edges;
    List out_edges;
} KLVMBlock, *KLVMBlockRef;

/* Append a basic block to the end of a function */
KLVMBlockRef KLVMAppendBlock(KLVMValueRef fn, char *label);

/* Add a basic block after 'bb' */
KLVMBlockRef KLVMAddBlock(KLVMBlockRef bb, char *label);

/* Add a basic block before 'bb' */
KLVMBlockRef KLVMAddBlockBefore(KLVMBlockRef bb, char *label);

/* Delete a basic block */
void KLVMDeleteBlock(KLVMBlockRef bb);

/*--------------------------------------------------------------------------*\
|* Block Edge                                                               *|
\*--------------------------------------------------------------------------*/

typedef struct _KLVMEdge {
    List link;
    KLVMBlockRef src;
    KLVMBlockRef dst;
    List in_link;
    List out_link;
} KLVMEdge, *KLVMEdgeRef;

/* Add an edge */
void KLVMLinkEdge(KLVMBlockRef src, KLVMBlockRef dst);

/*--------------------------------------------------------------------------*\
|* Instruction Visitor                                                      *|
\*--------------------------------------------------------------------------*/

typedef struct _KLVMVisitor {
    KLVMBlockRef bb;
    ListRef it;
} KLVMVisitor, *KLVMVisitorRef;

// clang-format off

/* Set a visitor at the head of 'bb' */
#define KVLMSetVisitor(vst, bb) \
    do { *(vst) = (KLVMVisitor){bb, NULL}; } while (0)

// clang-format on

/* Set visitor at end */
void KLVMSetVisitorAtEnd(KLVMVisitorRef vst);

/* Set visitor at head */
void KLVMSetVisitorAtHead(KLVMVisitorRef vst);

/* Set visitor at 'inst' */
void KLVMSetVisitor(KLVMVisitorRef vst, KLVMValueRef inst);

/* Set visitor before 'inst' */
void KLVMSetVisitorBefore(KLVMVisitorRef vst, KLVMValueRef inst);

/*--------------------------------------------------------------------------*\
|* Function                                                                 *|
\*--------------------------------------------------------------------------*/

typedef struct _KLVMFunc {
    KLVM_VALUE_HEAD
    int attr;
    int tag_index;
    int bb_tag_index;
    int num_params;
    int num_bbs;
    List bb_list;
    List edge_list;
    Vector locals;
    KLVMBlock sbb;
    KLVMBlock ebb;
} KLVMFunc, *KLVMFuncRef;

/* Get function prototype */
KLVMTypeRef KLVMGetFuncType(KLVMValueRef fn);

/* Get function param by index */
KLVMValueRef KLVMGetParam(KLVMValueRef fn, int index);

/* Add a local variable */
KLVMValueRef KLVMAddLocal(KLVMValueRef fn, char *name, KLVMTypeRef ty);

#ifdef __cplusplus
}
#endif

#endif /* _KLVM_VALUE_H_ */

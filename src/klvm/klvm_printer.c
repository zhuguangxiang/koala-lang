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

#include "klvm/klvm_insn.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * https://cs.lmu.edu/~ray/notes/ir/
 */

/* clang-format off */

#define NEW_LINE() fprintf(fp, "\n")
#define INDENT() fprintf(fp, "  ")
#define ASSIGN() fprintf(fp, " = ")
#define COMMA()  fprintf(fp, ", ")

/* clang-format on */

static void KLVMPrintVar(KLVMVarRef var, FILE *fp)
{
    if (KLVMIsPub(var)) fprintf(fp, "pub ");

    if (KLVMIsFinal(var))
        fprintf(fp, "const ");
    else
        fprintf(fp, "var ");

    fprintf(fp, "%s %s", var->name, KLVMTypeToStr(var->type));
}

static void KLVMPrintLocal(KLVMVarRef var, FILE *fp)
{
    fprintf(fp, "local %s %s", var->name, KLVMTypeToStr(var->type));
}

static inline int val_tag(KLVMFuncRef fn, KLVMValueRef val)
{
    if (val->tag < 0) {
        val->tag = fn->tag_index++;
    }
    return val->tag;
}

static inline int bb_tag(KLVMBasicBlockRef bb)
{
    if (bb->tag < 0) {
        KLVMFuncRef fn = (KLVMFuncRef)bb->fn;
        bb->tag = fn->bb_tag_index++;
    }
    return bb->tag;
}

static void KLVMPrintConst(KLVMConstRef v, FILE *fp)
{
    KLVMTypeKind kind = v->type->kind;
    switch (kind) {
        case KLVM_TYPE_INT8:
            fprintf(fp, "%d", v->i8val);
            break;
        case KLVM_TYPE_INT16:
            fprintf(fp, "%d", v->i16val);
            break;
        case KLVM_TYPE_INT32:
            fprintf(fp, "%d", v->i32val);
            break;
        case KLVM_TYPE_INT64:
            fprintf(fp, "%ld", v->i64val);
            break;
        default:
            break;
    }
}

static void KLVMPrintType(KLVMTypeRef ty, FILE *fp)
{
    fprintf(fp, " %s", KLVMTypeToStr(ty));
}

static void KLVMPrintName(KLVMValueRef val, KLVMFuncRef fn, FILE *fp)
{
    if (val->name && val->name[0])
        fprintf(fp, "%%%s", val->name);
    else {
        fprintf(fp, "%%%d", val_tag(fn, val));
    }
}

static void KLVMPrintUse(KLVMFuncRef fn, KLVMUseRef use, FILE *fp)
{
    KLVMValueRef val = use->interval->parent;
    KLVMPrintName(val, fn, fp);
    KLVMPrintType(val->type, fp);
}

static void KLVMPrintOperand(KLVMFuncRef fn, KLVMOperRef oper, FILE *fp)
{
    KLVMOperKind kind = oper->kind;
    switch (kind) {
        case KLVM_OPER_CONST: {
            KLVMConstRef val = oper->konst;
            KLVMPrintConst(val, fp);
            KLVMPrintType(val->type, fp);
            break;
        }
        case KLVM_OPER_USE: {
            KLVMPrintUse(fn, &oper->use, fp);
            break;
        }
        case KLVM_OPER_FUNC: {
            KLVMFuncRef _fn = (KLVMFuncRef)oper->fn;
            fprintf(fp, "@%s", _fn->name);
            break;
        }
        case KLVM_OPER_BLOCK: {
            break;
        }
        default: {
            assert(0);
            break;
        }
    }
}

static void KLVMPrintCopy(KLVMInsnRef insn, KLVMFuncRef fn, FILE *fp)
{
    fprintf(fp, "copy ");
    KLVMPrintOperand(fn, &insn->operands[0], fp);
    fprintf(fp, ", ");
    KLVMPrintOperand(fn, &insn->operands[1], fp);
}

static void KLVMPrintBinary(KLVMInsnRef insn, char *opname, KLVMFuncRef fn,
                            FILE *fp)
{
    KLVMPrintName((KLVMValueRef)insn, fn, fp);

    KLVMPrintType(insn->type, fp);

    ASSIGN();

    fprintf(fp, "%s ", opname);
    KLVMPrintOperand(fn, &insn->operands[0], fp);
    COMMA();
    KLVMPrintOperand(fn, &insn->operands[1], fp);
}

static void KLVMPrintCall(KLVMInsnRef insn, KLVMFuncRef fn, FILE *fp)
{
    KLVMPrintName((KLVMValueRef)insn, fn, fp);

    KLVMPrintType(insn->type, fp);

    ASSIGN();

    fprintf(fp, "call ");

    KLVMPrintOperand(fn, &insn->operands[0], fp);

    fprintf(fp, "(");

    KLVMOperRef oper;
    for (int i = 1; i < insn->num_ops; i++) {
        oper = &insn->operands[i];
        KLVMPrintOperand(fn, oper, fp);
    }

    fprintf(fp, ")");
}

static void KLVMPrintRet(KLVMInsnRef insn, KLVMFuncRef fn, FILE *fp)
{
    fprintf(fp, "ret ");
    KLVMPrintOperand(fn, &insn->operands[0], fp);
}

static void KLVMPrintJmp(KLVMInsnRef insn, FILE *fp)
{
    fprintf(fp, "jmp ");

    KLVMBasicBlockRef bb = insn->operands[0].block;
    if (bb->label && bb->label[0])
        fprintf(fp, "label %%%s", bb->label);
    else
        fprintf(fp, "label %%bb%d", bb_tag(bb));
}

static void KLVMPrintCondJmp(KLVMInsnRef insn, KLVMFuncRef fn, FILE *fp)
{
    fprintf(fp, "br ");
    KLVMPrintOperand(fn, &insn->operands[0], fp);

    KLVMBasicBlockRef bb;

    bb = insn->operands[1].block;
    if (bb->label && bb->label[0])
        fprintf(fp, ", label %%%s", bb->label);
    else
        fprintf(fp, ", label %%bb%d", bb_tag(bb));

    bb = insn->operands[2].block;
    if (bb->label && bb->label[0])
        fprintf(fp, ", label %%%s", bb->label);
    else
        fprintf(fp, ", label %%bb%d", bb_tag(bb));
}

void KLVMPrintInsn(KLVMFuncRef fn, KLVMInsnRef insn, FILE *fp)
{
    switch (insn->op) {
        case KLVM_INSN_COPY:
            KLVMPrintCopy(insn, fn, fp);
            break;
        case KLVM_INSN_ADD:
            KLVMPrintBinary(insn, "add", fn, fp);
            break;
        case KLVM_INSN_SUB:
            KLVMPrintBinary(insn, "sub", fn, fp);
            break;
        case KLVM_INSN_CMP_LE:
            KLVMPrintBinary(insn, "cmple", fn, fp);
            break;
        case KLVM_INSN_CALL:
            KLVMPrintCall(insn, fn, fp);
            break;
        case KLVM_INSN_RET:
            KLVMPrintRet(insn, fn, fp);
            break;
        case KLVM_INSN_JMP:
            KLVMPrintJmp(insn, fp);
            break;
        case KLVM_INSN_COND_JMP:
            KLVMPrintCondJmp(insn, fn, fp);
            break;
        default:
            break;
    }
}

static void KLVMPrintPreds(KLVMBasicBlockRef bb, int spaces, FILE *fp)
{
    fprintf(fp, "%*s = %%", spaces, ";; preds");

    KLVMBasicBlockRef src;
    KLVMEdgeRef edge;
    int i = 0;
    EdgeInForEach(edge, &bb->in_edges, {
        src = edge->src;
        if (i++ == 0)
            fprintf(fp, "%s", src->label);
        else
            fprintf(fp, ", %s", src->label);
    });
}

static void KLVMPrintBlock(KLVMBasicBlockRef bb, FILE *fp)
{
    int cnt;
    if (bb->label && bb->label[0]) {
        cnt = fprintf(fp, "%s:", bb->label);
    } else {
        cnt = fprintf(fp, "bb%d:", bb_tag(bb));
    }

    // print predecessors
    if (ListEmpty(&bb->in_edges)) {
        fprintf(fp, "%*s", 64 - cnt, ";; No preds!");
    } else {
        KLVMFuncRef fn = (KLVMFuncRef)bb->fn;
        KLVMEdgeRef edge = ListFirst(&bb->in_edges, KLVMEdge, in_link);
        if (edge->src != fn->sbb) KLVMPrintPreds(bb, 64 - cnt, fp);
    }

    KLVMInsnRef insn;
    InsnForEach(insn, &bb->insns, {
        NEW_LINE();
        INDENT();
        KLVMPrintInsn((KLVMFuncRef)bb->fn, insn, fp);
    });
}

static void KLVMPrintBlocks(KLVMFuncRef fn, FILE *fp)
{
    // print basic blocks directly(not cfg)

    KLVMBasicBlockRef bb;
    BasicBlockForEach(bb, &fn->bb_list, {
        NEW_LINE();
        KLVMPrintBlock(bb, fp);
    });
}

static void KLVMPrintLocals(KLVMFuncRef fn, FILE *fp)
{
    // print local variables
    KLVMVarRef var;
    int num_locals = VectorSize(&fn->locals);
    for (int i = fn->num_params; i < num_locals; i++) {
        VectorGet(&fn->locals, i, &var);
        NEW_LINE();
        INDENT();
        KLVMPrintLocal(var, fp);
    }
}

static void KLVMPrintFunc(KLVMFuncRef fn, FILE *fp)
{
    fprintf(fp, "func %s", fn->name);

    fprintf(fp, "(");
    KLVMValueRef para;
    for (int i = 0; i < fn->num_params; i++) {
        VectorGet(&fn->locals, i, &para);
        if (i != 0) fprintf(fp, ", ");
        if (para->name && para->name[0])
            fprintf(fp, "%s", para->name);
        else
            fprintf(fp, "%%%d", val_tag(fn, para));
        fprintf(fp, " %s", KLVMTypeToStr(para->type));
    }
    fprintf(fp, ")");

    KLVMTypeRef ret = KLVMProtoRet(fn->type);
    if (ret) fprintf(fp, " %s", KLVMTypeToStr(ret));

    fprintf(fp, " {");

    KLVMPrintLocals(fn, fp);
    KLVMPrintBlocks(fn, fp);

    NEW_LINE();
    fprintf(fp, "}");
    NEW_LINE();
}

void KLVMPrintModule(KLVMModuleRef m, FILE *fp)
{
    fprintf(fp, "\n__name__ := \"%s\"\n\n", m->name);

    KLVMValueRef *item;
    VectorForEach(item, &m->variables, {
        KLVMVarRef var = *(KLVMVarRef *)item;
        KLVMPrintVar(var, fp);
        NEW_LINE();
    });

    VectorForEach(item, &m->functions, {
        KLVMFuncRef fn = *(KLVMFuncRef *)item;
        KLVMPrintFunc(fn, fp);
        NEW_LINE();
    });
}

#ifdef __cplusplus
}
#endif

/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "atom.h"
#include "ir.h"
#include "klc.h"
#include "log.h"
#include "mm.h"
#include "parser.h"

/* emit ir to byte codes */

#ifdef __cplusplus
extern "C" {
#endif

#define EMIT_OP(op) buf_write_byte(buf, (uint8_t)(op))

#define EMIT_BYTE_ARG(val) buf_write_byte(buf, (uint8_t)(val))
#define EMIT_WORD_ARG(val) buf_write_word(buf, (uint16_t)(val))

static void emit_insn_operand(KlrOper *oper, Buffer *buf, KlcFile *filp)
{
    KlrOperKind kind = oper->kind;
    KlrValue *val = oper->use.ref;

    if (kind == KLR_OPER_CONST) {
        KlrConst *v = (KlrConst *)val;
        int kind = v->which;
        switch (kind) {
            case CONST_INT: {
                if (v->len == 1) {
                    EMIT_OP(OP_PUSH_IMM8);
                    EMIT_BYTE_ARG(v->ival);
                } else if (v->len == 2) {
                    EMIT_OP(OP_PUSH_IMM16);
                    EMIT_WORD_ARG(v->ival);
                } else {
                    uint16_t index = klc_add_int(filp, v->ival, v->sign, v->len);
                    EMIT_OP(OP_PUSH_CONST);
                    EMIT_WORD_ARG(index);
                }
                break;
            }
            case CONST_FLT:
                break;
            case CONST_BOOL:
                break;
            case CONST_STR: {
                uint16_t index = klc_add_str(filp, v->sval, v->len);
                EMIT_OP(OP_PUSH_CONST);
                EMIT_WORD_ARG(index);
                break;
            }
            default:
                UNREACHABLE();
                break;
        }
    } else {
        assert(0);
    }
}

static void emit_insn_call(KlrInsn *insn, Buffer *buf, KlcFile *filp)
{
    uint16_t reloc_index = 0;
    KlrValue *val = insn->opers[0].use.ref;
    if (val->kind == KLR_VALUE_EXT_FUNC) {
        KlrExtFunc *ext = (KlrExtFunc *)val;
        reloc_index = klc_add_reloc(filp, ext->owner, ext->name);
    }

    KlrOper *oper;
    for (int i = 1; i < insn->num_opers; i++) {
        // if (i != 1) fprintf(fp, ", ");
        oper = &insn->opers[i];
        emit_insn_operand(oper, buf, filp);
    }

    // OP_CALL num_args, index_reloc
    EMIT_OP(OP_CALL);
    EMIT_BYTE_ARG(insn->num_opers - 1);
    EMIT_WORD_ARG(reloc_index);
}

static void emit_insn(KlrInsn *insn, Buffer *buf, KlcFile *filp)
{
    switch (insn->code) {
        case OP_CALL: {
            emit_insn_call(insn, buf, filp);
            break;
        }
        default: {
            EMIT_OP(insn->code);
            break;
        }
    }
}

void kl_emit_func(ParserState *ps, KlrFunc *fn, KlcFunc *klc_fn)
{
    klr_print_func(fn, stdout);
    klr_alloc_registers(fn);
    klr_print_func(fn, stdout);

    KlcFile *filp = klc_fn->filp;

    BUF(buf);

    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        KlrInsn *insn;
        insn_foreach(insn, bb) {
            emit_insn(insn, &buf, filp);
        }
    }

    int num_locals = vector_size(&fn->locals);
    num_locals += vector_size(&fn->params);
    uint16_t code_index = klc_add_code(filp, num_locals, BUF_LEN(buf), BUF_STR(buf));
    klc_fn->code_index = code_index;

    FINI_BUF(buf);
}

#ifdef __cplusplus
}
#endif

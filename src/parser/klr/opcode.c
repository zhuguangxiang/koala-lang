/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "opcode.h"
#include <stdio.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

char *__op_names[] = {
#include "opcode_list_lowercase.h"
};

OpFormat __op_formats[] = {
#define X(name, fmt) fmt,
#include "opcode_list.h"
#undef X
};

char *tag_mapping[] = {
    "false", "true", "none",      "+0.0",       "-0.0",       "NaN",
    "+inf",  "-inf", "empty_str", "empty_list", "empty_dict",
};

char *intern_tag_name[] = {
    [INTERN_TUPLE] = "tuple",
    [INTERN_RANGE] = "range",
    [INTERN_LIST] = "list",
};

void bytecode_print(uint8_t *code, size_t start, size_t count)
{
    for (size_t pc = start; pc < start + count; pc++) {
        uint32_t insn = *(uint32_t *)(code + pc * 4);
        OpCode opcode = (insn >> 24) & 0xFFu;
        OpFormat fmt = op_format(opcode);
        char *s = op_name(opcode);
        ASSERT(s);
        printf("%04zu:  %08X   %s ", pc, insn, s);
        switch (fmt) {
            case FORMAT_Rx: {
                int Rx = insn & 0xFFFu;
                printf("r%d", Rx);
                break;
            }

            case FORMAT_RTagImm: {
                int R = (insn >> 16) & 0xFFu;
                int tag = (insn >> 8) & 0xFFu;
                int imm = insn & 0xFFu;
                printf("r%d, @%s, #%d", R, intern_tag_name[tag], imm);
                break;
            }

            case FORMAT_RxTag: {
                int Rx = (insn >> 8) & 0xFFFu;
                int imm = insn & 0xFFu;
                printf("r%d, #%s", Rx, tag_mapping[imm]);
                break;
            }

            case FORMAT_RxIdx12: {
                int Rx = (insn >> 12) & 0xFFFu;
                int idx = insn & 0xFFFu;
                printf("r%d, #%d", Rx, idx);
                break;
            }

            case FORMAT_ROff2: {
                int R = (insn >> 16) & 0xFFu;
                int data = (int16_t)(insn & 0xFFFFu);
                printf("r%d, %d", R, data);
                break;
            }

            case FORMAT_RImm2:
            case FORMAT_RIdx2: {
                int R = (insn >> 16) & 0xFFu;
                int data = (int16_t)(insn & 0xFFFFu);
                printf("r%d, #%d", R, data);
                break;
            }

            case FORMAT_RImmOff: {
                int R = (insn >> 16) & 0xFFu;
                int imm = (int8_t)((insn >> 8) & 0xFFu);
                int off = (int8_t)(insn & 0xFFu);
                printf("r%d, #%d, %d", R, imm, off);
                break;
            }

            case FORMAT_RxRx: {
                int Rx1 = (insn >> 12) & 0xFFFu;
                int Rx2 = insn & 0xFFFu;
                printf("r%d, r%d", Rx1, Rx2);
                break;
            }

            case FORMAT_RRImm: {
                int R1 = (insn >> 16) & 0xFFu;
                int R2 = (insn >> 8) & 0xFFu;
                int data = (int8_t)(insn & 0xFFu);
                printf("r%d, r%d, #%d", R1, R2, data);
                break;
            }

            case FORMAT_RROff: {
                int R1 = (insn >> 16) & 0xFFu;
                int R2 = (insn >> 8) & 0xFFu;
                int data = (int8_t)(insn & 0xFFu);
                printf("r%d, r%d, %d", R1, R2, data);
                break;
            }

            case FORMAT_RRR: {
                int R1 = (insn >> 16) & 0xFFu;
                int R2 = (insn >> 8) & 0xFFu;
                int R3 = insn & 0xFFu;
                printf("r%d, r%d, r%d", R1, R2, R3);
                break;
            }

            case FORMAT_JMP: {
                int data = (int16_t)(insn & 0xFFFFu);
                printf("%d", data);
                break;
            }

            case FORMAT_Tag: {
                int imm = insn & 0xFFu;
                printf("#%s", tag_mapping[imm]);
                break;
            }

            case FORMAT_Imm2:
            case FORMAT_Idx2: {
                int data = (int16_t)(insn & 0xFFFFu);
                printf("#%d", data);
                break;
            }

            case FORMAT_Op: {
                // no operands
                break;
            }

            case FORMAT_CALL: {
                int flag = (insn >> 20) & 0xFu;
                int Rx = (insn >> 8) & 0xFFFu;
                int nargs = insn & 0xFFu;
                printf("flg=%d, ", flag);
                if (Rx != 0xFFFu) printf("r%d, ", Rx);
                printf("#%d", nargs);
                if (opcode != OP_TAIL_CALL) {
                    pc++; // skip the next FORMAT_DATA entry
                    insn = *(uint32_t *)(code + pc * 4);
                    printf("\n%04zu:  %08X   data ", pc, insn);
                    if (flag == 0) {
                        printf("(rel32=%d)", (int)insn);
                    } else if (flag == 1) {
                        printf("(import_index=%d)", (int)insn);
                    } else if (flag == 2) {
                        printf("(intf_slot=%d)", (int)insn);
                    }
                }
                break;
            }

            case FORMAT_R_TI_Imm12: {
                int R = (insn >> 16) & 0xFFu;
                int ti = (insn >> 12) & 0xFu;
                int imm12 = insn & 0xFFFu;
                if (ti & 0b100) {
                    // unsigned
                    printf("r%d, ti=0x%x, #%u", R, ti, imm12);
                } else {
                    // signed
                    int simm12 = ((int)(imm12 << 20)) >> 20;
                    printf("r%d, ti=0x%x, #%d", R, ti, simm12);
                }
                break;
            }

            case FORMAT_TI_Imm2: {
                int ti = (insn >> 16) & 0xFu;
                int imm16 = insn & 0xFFFFu;
                if (ti & 0b100) {
                    // unsigned
                    printf("ti=0x%x, #%u", ti, imm16);
                } else {
                    // signed
                    printf("ti=0x%x, #%d", ti, (int16_t)imm16);
                }
                break;
            }

            case FORMAT_RR_TI_MODE: {
                int R1 = (insn >> 16) & 0xFFu;
                int R2 = (insn >> 8) & 0xFFu;
                int ti = (insn >> 2) & 0x3Fu;
                int mode = insn & 0x3u;
                printf("r%d, r%d, ti=0x%x, mode=%d", R1, R2, ti, mode);
                break;
            }

            default: {
                printf("(unknown format)");
                break;
            }
        }
        printf("\n");
    }
}

#ifdef __cplusplus
}
#endif

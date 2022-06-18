/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2018-2022 James <zhuguangxiang@gmail.com>
 */

#ifndef _KLM_OPCODE_H_
#define _KLM_OPCODE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _KLMOpCode KLMOpCode;

enum _KLMOpCode {
    KLM_OP_NONE,
    KLM_OP_COPY,
    KLM_OP_ADD,
    KLM_OP_SUB,
    KLM_OP_MUL,
    KLM_OP_DIV,
    KLM_OP_MOD,
    KLM_OP_BRANCH,
    KLM_OP_JUMP,
    KLM_OP_RET,
    KLM_OP_MAX,
};

#ifdef __cplusplus
}
#endif

#endif /* _KLM_OPCODE_H_ */

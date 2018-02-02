
#ifndef _KOALA_CODEGEN_H_
#define _KOALA_CODEGEN_H_

#include "parser.h"
#include "opcode.h"

#ifdef __cplusplus
extern "C" {
#endif

void expr_generate_code(FuncData *func, struct expr *exp);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_CODEGEN_H_ */

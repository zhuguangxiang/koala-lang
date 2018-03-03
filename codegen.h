
#ifndef _KOALA_CODEGEN_H_
#define _KOALA_CODEGEN_H_

#include "parser.h"

#ifdef __cplusplus
extern "C" {
#endif

void gencode(ParserState *ps, char *out);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_CODEGEN_H_ */

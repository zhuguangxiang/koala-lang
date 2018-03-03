
#ifndef _KOALA_CHECKER_H_
#define _KOALA_CHECKER_H_

#include "parser.h"

#ifdef __cplusplus
extern "C" {
#endif

void check_unused_imports(ParserState *ps);
void check_unused_symbols(ParserState *ps);
int check_call_args(Proto *proto, Vector *vec);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_CHECKER_H_ */

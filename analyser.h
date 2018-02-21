
#ifndef _KOALA_ANALYSER_H_
#define _KOALA_ANALYSER_H_

#include "parser.h"

#ifdef __cplusplus
extern "C" {
#endif

void Analyse(ParserState *ps);
void Check_Unused_Imports(ParserState *ps);
void Check_Unused_Symbols(ParserState *ps);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_ANALYSER_H_ */


#ifndef _KOALA_LEXER_H_
#define _KOALA_LEXER_H_

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LINE_MAX_LEN 256

typedef struct lex_state {
	void *scanner;
	char line[LINE_MAX_LEN];
	int linelen;
	int lastlen;
	int row;
	int col;
} LexerState;

LexerState *Lexer_New(FILE *in);
void Lexer_Destroy(LexerState *ls);

int Lexer_DoYYInput(LexerState *ls, char *buf, int size, FILE *in);
void Lexer_DoUserAction(LexerState *ls, char *token);
int Lexer_Row(void *scanner);
int Lexer_Col(void *scanner);
char *Lexer_Line(void *scanner);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_LEXER_H_ */

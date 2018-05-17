
#include "lexer.h"
#include "parser.h"
#include "koala_yacc.h"
#include "koala_lex.h"

LexerState *Lexer_New(FILE *in)
{
	LexerState *ls = calloc(1, sizeof(LexerState));
	yylex_init_extra(ls, &ls->scanner);
	yyset_in(in, ls->scanner);
	return ls;
}

void Lexer_Destroy(LexerState *ls)
{
	yylex_destroy(ls->scanner);
	free(ls);
}

int Lexer_DoYYInput(LexerState *ls, char *buf, int size, FILE *in)
{
	if (ls->linelen <= 0) {
		if (!fgets(ls->line, LINE_MAX_LEN, in)) {
			if (ferror(in)) clearerr(in);
			return 0;
		} else {
			ls->linelen = strlen(ls->line);
			//ls->line[ls->linelen] = 0;
			ls->row++;
			ls->lastlen = 0;
			ls->col = 0;
		}
	}

	int sz = min(ls->linelen, size);
	memcpy(buf, ls->line, sz);
	ls->linelen -= sz;
	return sz;
}

void Lexer_DoUserAction(LexerState *ls, char *token)
{
	ls->col += ls->lastlen;
	ls->lastlen = strlen(token);
	printf("row:%d, col:%d\n", ls->row, ls->col);
}

int Lexer_Row(void *scanner)
{
	LexerState *ls = yyget_extra(scanner);
	return ls->row;
}

int Lexer_Col(void *scanner)
{
	LexerState *ls = yyget_extra(scanner);
	return ls->col;
}

char *Lexer_Line(void *scanner)
{
	LexerState *ls = yyget_extra(scanner);
	return ls->line;
}

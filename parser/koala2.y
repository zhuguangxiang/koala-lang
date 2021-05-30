/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangXiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

/* prologue */

%{

#include <stdint.h>

#define yyerror(loc, ps, scanner, msg) ((void)0)

%}

%union {
    int8_t  INT8;
    int16_t INT16;
    int32_t INT32;
    int64_t INT64;
    int64_t INT;
    char *  STRING;
}

%token AND
%token OR
%token NOT
%token EQ
%token NE
%token LE
%token GE
%token POWER

%token L_SHIFT
%token R_ANGLE_SHIFT

%token SELF
%token SUPER
%token TRUE
%token FALSE

%token <INT> INT_LITERAL
%token <STRING> STRING_LITERAL
%token <STRING> ID

%locations
%parse-param {void *ps}
%parse-param {void *scanner}
%define api.pure full
%lex-param {void *scanner}
%code provides {
  int yylex(YYSTYPE *yylval, YYLTYPE *yylloc, void *yyscanner);
}

%start units

/* grammar rules */

%%

units
    : unit
    | units unit
    ;

unit
    : expr ';'
    ;

expr
    : cond_expr
    ;

cond_expr
    : or_expr
    | or_expr '?' or_expr ':' or_expr
    ;

or_expr
    : and_expr
    | or_expr OR and_expr
    ;

and_expr
    : bit_or_expr
    | and_expr AND bit_or_expr
    ;

bit_or_expr
    : bit_xor_expr
    | bit_or_expr '|' bit_xor_expr
    ;

bit_xor_expr
    : bit_and_expr
    | bit_xor_expr '^' bit_and_expr
    ;

bit_and_expr
    : equality_expr
    | bit_and_expr '&' equality_expr
    ;

equality_expr
    : relation_expr
    | equality_expr EQ relation_expr
    | equality_expr NE relation_expr
    ;

relation_expr
    : shift_expr
    | relation_expr '<' shift_expr
    | relation_expr '>' shift_expr
    | relation_expr LE shift_expr
    | relation_expr GE shift_expr
    ;

shift_expr
    : add_expr
    | shift_expr R_ANGLE_SHIFT '>' add_expr
    | shift_expr L_SHIFT add_expr
    ;

add_expr
    : multi_expr
    | add_expr '+' multi_expr
    | add_expr '-' multi_expr
    ;

multi_expr
    : unary_expr
    | multi_expr '*' unary_expr
    | multi_expr '/' unary_expr
    | multi_expr '%' unary_expr
    | multi_expr POWER unary_expr
    ;

unary_expr
    : primary_expr
    | '+' unary_expr
    | '-' unary_expr
    | '~' unary_expr
    | NOT unary_expr
    ;

primary_expr
    : call_expr
    | dot_expr
    | index_expr
    | atom_expr
    ;

call_expr
    : primary_expr '(' ')'
    | primary_expr '(' expr_list ')'
    | primary_expr '(' expr_list ';' ')'
    ;

dot_expr
    : primary_expr '.' ID
    | primary_expr '.' INT_LITERAL
    ;

index_expr
    : primary_expr '[' expr ']'
    | primary_expr '[' slice_expr ']'
    ;

slice_expr
    : expr ':' expr
    | ':' expr
    | expr ':'
    | ':'
    ;

atom_expr
    : ID
    | '_'
    | STRING_LITERAL
    | TRUE
    | FALSE
    | SELF
    | SUPER
    | '(' expr ')'
    ;

expr_list
    : expr
    | expr_list ',' expr
    ;

%%

/* epilogue */

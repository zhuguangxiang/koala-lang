/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/* prologue */

%{

#include "atom.h"
#include "buffer.h"
#include "parser.h"
#include "koala_yacc.h"

#define STD_BUILTIN_PATH "std/builtin"
#define PKG_PATH ps->pm->pkg_path

#define yyerror(loc, ps, scanner, msg) ((void)0)

#define line(loc) ((loc).first_line)
#define last_line(loc) ((loc).last_line)
#define col(loc) ((loc).first_column)
#define last_col(loc) ((loc).last_column)

#define loc(l) (Loc){line(l), col(l), last_line(l), last_col(l)}
#define lloc(l, r) (Loc){line(l), col(l), last_line(r), last_col(r)}

static int need_clear(int token)
{
    int tokens[] = {
        ';', IMPORT, LET, VAR, FUNC,
        IF, WHILE, FOR, MATCH, CLASS,
        TRAIT, RETURN
    };

    for (int i = 0; i < COUNT_OF(tokens); i++) {
        if (tokens[i] == token) return 0;
    }
    return 1;
}

#define yy_clear_ok if (need_clear(ps->token)) yyclearin; yyerrok

static void free_optional_type_list(Vector *vec)
{
    TypeSpec *item;
    vector_foreach(item, vec) {
        type_spec_free(item);
    }
    vector_destroy(vec);
}

static void free_expr_list(Vector *vec)
{
    Expr *item;
    vector_foreach(item, vec) {
        expr_free(item);
    }
    vector_destroy(vec);
}

static void free_stmt_list(Vector *vec)
{
    Stmt *item;
    vector_foreach(item, vec) {
        if (item) stmt_free(item);
    }
    vector_destroy(vec);
}

// klass_list: TypeSpec * items
static void free_klass_list(Vector *vec)
{
    free_optional_type_list(vec);
}

static void free_tp_decl(TypeParamDecl *tp)
{
    if (!tp) return;
    if (tp->which == TP_DECL_NORMAL) {
        free_klass_list(tp->bound);
    } else if (tp->which == TP_DECL_CONST) {
        type_spec_free(tp->const_type);
    }
    mm_free(tp);
}

// tp_decl_list / type_param_list: TypeParamDecl * items
static void free_tp_list(Vector *vec)
{
    TypeParamDecl *tp;
    vector_foreach(tp, vec) {
        free_tp_decl(tp);
    }
    vector_destroy(vec);
}

static void free_param_decl(ParamDecl *p)
{
    if (!p) return;
    type_spec_free(p->type);
    expr_free(p->value);
    mm_free(p);
}

// param_list / id_type_arg_list / kw_arg_list: ParamDecl * items
static void free_param_list(Vector *vec)
{
    ParamDecl *p;
    vector_foreach(p, vec) {
        free_param_decl(p);
    }
    vector_destroy(vec);
}

// tuple_decl_list: IdentType items, the type is optional
static void free_ident_type_list(Vector *vec)
{
    IdentType *item;
    vector_foreach_ptr(item, vec) {
        type_spec_free(item->ts);
    }
    vector_destroy(vec);
}

static void yyparse_module(ParserState *ps, Vector *stmts)
{
    Stmt *stmt;
    vector_foreach(stmt, stmts) {
        parse_top_stmt(ps, stmt);
    }
    vector_destroy(stmts);
}

/*
 * let v = expr else { ... }
 * desugared into two sibling stmts (must not be wrapped in a block,
 * otherwise 'v' is invisible to the following stmts):
 *   let v = expr
 *   if v == nil { ... }
 * 'v' is narrowed to non-null by the early-return shadow logic.
 */
static Vector *let_else_stmts(Ident *id, TypeSpec *type, Expr *exp, Vector *block,
                              Loc decl_loc, Loc else_loc, Loc if_loc)
{
    Stmt *decl = stmt_from_var_decl(*id, type, 1, exp);
    stmt_set_loc(decl, decl_loc);
    var_set_where(decl, VAR_LOCAL);

    Expr *lhs = expr_from_ident(id);
    expr_set_loc(lhs, id->loc);
    Expr *rhs = expr_from_lit_none();
    expr_set_loc(rhs, else_loc);
    Expr *cond = expr_from_binary(BINARY_EQ, else_loc, lhs, rhs);
    expr_set_loc(cond, decl_loc);

    Stmt *s = stmt_from_if(cond, block, NULL);
    ((IfStmt *)s)->is_let_else = 1;
    stmt_set_loc(s, if_loc);

    Vector *stmts = vector_create_ptr();
    vector_push_back(stmts, &decl);
    vector_push_back(stmts, &s);
    return stmts;
}

// let (a, b) = t: at least two names, 'let (a) = t' is not a destructuring
static Stmt *tuple_decl_stmt(ParserState *ps, Vector *ids, int which, Expr *exp, Loc ids_loc,
                             Loc loc)
{
    if (vector_size(ids) < 2) {
        kl_error(ids_loc, "destructuring needs at least two names.");
        free_ident_type_list(ids);
        expr_free(exp);
        return NULL;
    }

    Stmt *s = stmt_from_tuple_decl(ids, which, exp);
    stmt_set_loc(s, loc);
    return s;
}

%}

%union {
    char *sval;
    __int128 ival;
    __float128 fval;
    Stmt *stmt;
    Expr *expr;
    TypeSpec *type_spec;
    Vector *vec;
    PrefixFlags prefix_flags;
    Ident ident;
    TypeParamDecl *tpval;
    ParamDecl *param;
    IdentType kls_name;
    AnnotParam *annot_param;
}

%token FROM
%token IMPORT
%token LINK
%token CONST
%token LET
%token VAR
%token FUNC
%token CLASS
%token TRAIT
%token IF
%token ELSE
%token WHILE
%token LOOP
%token FOR
%token MATCH
%token CASE
%token BREAK
%token CONTINUE
%token RETURN
%token IN
%token AS
%token IS
%token PUBLIC
%token INFER
%token STATIC

%token SELF
%token TRUE
%token FALSE
%token NONE

%token UINT8
%token UINT16
%token UINT32
%token UINT64
%token INT8
%token INT16
%token INT32
%token INT64

%token FLOAT16
%token FLOAT32
%token FLOAT64
%token BFLOAT16

%token ARRAY
%token BOOL
%token STRING
%token BYTES
%token ANY
%token LIST
%token MAP
%token TUPLE
%token SET
%token TYPE
%token RANGE

%token LAMBDA_START
%token LAMBDA_START_OR

%token AND
%token OR
%token NOT

%token EQ
%token NE
%token GE
%token LE

%token PLUS_ASSIGN
%token MINUS_ASSIGN
%token MULT_ASSIGN
%token DIV_ASSIGN
%token MOD_ASSIGN
%token AND_ASSIGN
%token OR_ASSIGN
%token XOR_ASSIGN
%token SHL_ASSIGN
%token SHR_ASSIGN
%token DOTDOTDOT

%token L_SHIFT
%token R_SHIFT

%token OPT_DEF
%token OPT_DOT
%token BANG_DOT

%token CONST_PLACEHOLDER

%type<stmt> import_stmt
%type<stmt> link_stmt
%type<stmt> top_stmt
%type<stmt> const_decl
%type<stmt> let_decl
%type<stmt> var_decl
%type<stmt> assignment
%type<stmt> return_stmt
%type<stmt> jump_stmt
%type<stmt> if_stmt
%type<stmt> elseif_stmt
%type<stmt> while_stmt
%type<stmt> for_stmt
%type<stmt> local
%type<stmt> func_decl
%type<stmt> field_decl
%type<stmt> prefix_field_decl
%type<stmt> method_decl
%type<stmt> trait_decl
%type<stmt> class_decl
%type<stmt> func_proto_decl
%type<stmt> trait_method

%type<expr> const_expr
%type<expr> const_tuple_expr
%type<expr> expr
%type<expr> or_expr
%type<expr> in_expr
%type<expr> and_expr
%type<expr> bit_or_expr
%type<expr> bit_xor_expr
%type<expr> bit_and_expr
%type<expr> equality_expr
%type<expr> relation_expr
%type<expr> shift_expr
%type<expr> add_expr
%type<expr> multi_expr
%type<expr> unary_expr
%type<expr> primary_expr
%type<expr> call_expr
%type<expr> dot_expr
%type<expr> index_expr
%type<expr> slice_expr
%type<expr> atom_expr
%type<expr> list_expr
%type<expr> map_expr
%type<expr> map
%type<expr> atom
%type<expr> tuple_expr
%type<expr> lambda_expr
%type<expr> assign_left_expr

%type<type_spec> base_type
%type<type_spec> array_type
%type<type_spec> optional_type
%type<type_spec> param_type
%type<type_spec> type
%type<type_spec> type_opt
%type<type_spec> list_type
%type<type_spec> map_type
%type<type_spec> set_type
%type<type_spec> tuple_type
%type<type_spec> klass_type
%type<type_spec> atom_primitive_type
%type<type_spec> atom_type
%type<type_spec> union_type
%type<type_spec> union_opt_type
%type<type_spec> const_type
%type<type_spec> lambda_type

%type<vec> const_expr_list
%type<vec> id_as_list
%type<vec> top_stmts
%type<vec> specialized_types_list
%type<vec> atom_primitive_type_lists
%type<vec> atom_primitive_type_list
%type<vec> optional_type_list
%type<vec> base_type_list
%type<vec> block
%type<vec> local_list
%type<vec> let_else_stmt
%type<stmt> tuple_decl
%type<vec> tuple_decl_list
%type<vec> map_list
%type<vec> expr_list
%type<vec> index_expr_list
%type<vec> param_list
%type<vec> id_type_arg_list
%type<vec> kw_arg_list
%type<vec> klass_list
%type<vec> extends
%type<vec> class_members_or_empty
%type<vec> field_list
%type<vec> method_list
%type<vec> trait_members_or_empty
%type<vec> trait_method_list
%type<vec> type_param_list
%type<vec> const_tp_list
%type<vec> tp_decl_list
%type<vec> call_arg_list
%type<vec> call_kw_arg_list
%type<vec> id_list
%type<vec> int_lit_list
%type<vec> annot_param_list

%token<sval> ID
%token<ival> INT_LITERAL
%token<fval> FLOAT_LITERAL
%token STRING_LITERAL
%token CHAR_LITERAL
%token BYTES_LITERAL
%type<prefix_flags> annotation
%type<prefix_flags> prefix
%type<prefix_flags> access
%type<annot_param> annot_param
%type<ival> assign_operator
%type<kls_name> class_name
%type<kls_name> trait_name
%type<tpval> type_param_decl
%type<param> kw_arg

/*
 * Free the values bison discards during error recovery (popped states and
 * the discarded lookahead) or when the parse is aborted. A value passed to
 * a reduced rule is never destroyed, its action owns it.
 * Not destroyed: tokens (ID is an atom), class/trait names (interned type),
 * annotations / prefix flags (by-value structs).
 */
%destructor { expr_free($$); } <expr>
%destructor { stmt_free($$); } <stmt>
%destructor { type_spec_free($$); } <type_spec>
%destructor { free_tp_decl($$); } <tpval>
%destructor { free_param_decl($$); } <param>
%destructor { free_expr_list($$); }
    const_expr_list expr_list index_expr_list call_arg_list call_kw_arg_list map_list
%destructor { free_stmt_list($$); }
    top_stmts block local_list let_else_stmt class_members_or_empty field_list method_list
    trait_members_or_empty trait_method_list
%destructor { free_optional_type_list($$); }
    optional_type_list base_type_list atom_primitive_type_list klass_list extends
%destructor { free_tp_list($$); } type_param_list const_tp_list tp_decl_list
%destructor { free_param_list($$); } param_list id_type_arg_list kw_arg_list
%destructor { free_ident_type_list($$); } tuple_decl_list
%destructor { vector_destroy($$); } id_as_list id_list int_lit_list

%locations
%parse-param {ParserState *ps}
%parse-param {void *scanner}
%define api.pure full
%lex-param {void *scanner}
%code provides {
    int yylex(YYSTYPE *yylval_param, YYLTYPE *yylloc, void *yyscanner);
}

%start program

/* grammar rules */

%%

program
    : top_stmts
    {
        yyparse_module(ps, $1);
    }
    | %empty
    {
        // empty program
    }
    ;

import_stmt
    : IMPORT STRING_LITERAL semi
    {
        $$ = stmt_from_import(&ps->sbuf, NULL, NULL);
        stmt_set_loc($$, lloc(@1, @3));
    }
    | IMPORT STRING_LITERAL AS ID semi
    {
        $$ = stmt_from_import(&ps->sbuf, $4, NULL);
        stmt_set_loc($$, lloc(@1, @5));
    }
    | FROM STRING_LITERAL IMPORT id_as_list semi
    {
        $$ = stmt_from_import(&ps->sbuf, NULL, $4);
        stmt_set_loc($$, lloc(@1, @5));
    }
    | IMPORT STRING_LITERAL AS ID error
    {
        kl_error(loc(@5), "expected end of import.");
        yy_clear_ok;
        $$ = NULL;
    }
    | IMPORT STRING_LITERAL AS error
    {
        kl_error(loc(@4), "expected an identifier after 'as'.");
        yy_clear_ok;
        $$ = NULL;
    }
    | IMPORT STRING_LITERAL error
    {
        kl_error(loc(@3), "expected 'as' or end of import.");
        yy_clear_ok;
        $$ = NULL;
    }
    | IMPORT error
    {
        kl_error(loc(@2), "expected a package path string.");
        yy_clear_ok;
        $$ = NULL;
    }
    | FROM STRING_LITERAL IMPORT id_as_list error
    {
        vector_destroy($4);
        kl_error(loc(@5), "expected ',' or end of import.");
        yy_clear_ok;
        $$ = NULL;
    }
    | FROM STRING_LITERAL IMPORT error
    {
        kl_error(loc(@4), "expected an identifier list.");
        yy_clear_ok;
        $$ = NULL;
    }
    | FROM STRING_LITERAL error
    {
        kl_error(loc(@3), "expected 'import'.");
        yy_clear_ok;
        $$ = NULL;
    }
    | FROM error
    {
        kl_error(loc(@2), "expected a package path string.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

id_as_list
    : ID
    {
        $$ = vector_create(sizeof(IdentAsIdent));
        IdentAsIdent item = { 0 };
        Ident id = {$1, loc(@1)};
        item.id = id;
        vector_push_back($$, &item);
    }
    | ID AS ID
    {
        $$ = vector_create(sizeof(IdentAsIdent));
        IdentAsIdent item = { 0 };
        Ident id = {$1, loc(@1)};
        Ident alias_id = {$3, loc(@3)};
        item.id = id;
        item.alias_id = alias_id;
        vector_push_back($$, &item);
    }
    | id_as_list ',' ID
    {
        $$ = $1;
        IdentAsIdent item = { 0 };
        Ident id = {$3, loc(@3)};
        item.id = id;
        vector_push_back($$, &item);
    }
    | id_as_list ',' ID AS ID
    {
        $$ = $1;
        IdentAsIdent item = { 0 };
        Ident id = {$3, loc(@3)};
        Ident alias_id = {$5, loc(@5)};
        item.id = id;
        item.alias_id = alias_id;
        vector_push_back($$, &item);
    }
    ;

link_stmt
    : LINK STRING_LITERAL semi
    {
        $$ = stmt_from_link(&ps->sbuf);
        stmt_set_loc($$, lloc(@1, @2));
    }
    ;

top_stmts
    : top_stmt
    {
        $$ = vector_create_ptr();
        if ($1) vector_push_back($$, &$1);
    }
    | top_stmts top_stmt
    {
        $$ = $1;
        if ($2) vector_push_back($$, &$2);
    }
    ;

top_stmt
    : import_stmt
    {
        $$ = $1;
    }
    | link_stmt
    {
        $$ = $1;
    }
    | const_decl semi
    {
        $$ = $1;
        if ($$) var_set_where($$, VAR_GLOBAL);
    }
    | access const_decl semi
    {
        $$ = $2;
        if ($$) {
            var_set_where($$, VAR_GLOBAL);
            stmt_set_prefix($$, $1);
        }
    }
    | let_decl semi
    {
        $$ = $1;
        if ($$) var_set_where($$, VAR_GLOBAL);
    }
    | access let_decl semi
    {
        $$ = $2;
        if ($$) {
            var_set_where($$, VAR_GLOBAL);
            stmt_set_prefix($$, $1);
        }
    }
    | var_decl semi
    {
        $$ = $1;
        if ($$) var_set_where($$, VAR_GLOBAL);
    }
    | access var_decl semi
    {
        $$ = $2;
        if ($$) {
            var_set_where($$, VAR_GLOBAL);
            stmt_set_prefix($$, $1);
        }
    }
    | func_decl
    {
        $$ = $1;
    }
    | prefix func_decl
    {
        $$ = $2;
        stmt_set_prefix($$, $1);
    }
    | class_decl
    {
        $$ = $1;
    }
    | prefix class_decl
    {
        $$ = $2;
        stmt_set_prefix($$, $1);
    }
    | trait_decl
    {
        $$ = $1;
    }
    | access trait_decl
    {
        $$ = $2;
        stmt_set_prefix($$, $1);
    }
    | semi
    {
        $$ = NULL;
    }
    | prefix error
    {
        if (ps->errors == 0) {
            kl_error(loc(@2), "syntax error");
        }
        yyclearin; yyerrok;
        $$ = NULL;
    }
    | error {
        if (ps->errors == 0) {
            kl_error(loc(@1), "syntax error");
        }
        yyclearin; yyerrok;
        $$ = NULL;
    }
    | type_alias semi
    {
        $$ = NULL;
    }
    | expr semi
    {
        $$ = stmt_from_expr($1);
        stmt_set_loc($$, loc(@1));
    }
    | assignment semi
    {
        $$ = $1;
    }
    | if_stmt
    {
        $$ = $1;
    }
    | while_stmt
    {
        $$ = $1;
    }
    | for_stmt
    {
        $$ = $1;
    }
    | match_stmt
    {
        $$ = NULL;
    }
    ;

type_alias
    : TYPE ID '=' base_type
    {
        type_spec_free($4);
    }
    | TYPE ID '=' lambda_type
    {
        type_spec_free($4);
    }
    ;

semi
    : ';'
    ;

prefix
    : annotation
    {
        $$ = $1;
    }
    | access
    {
        $$ = $1;
    }
    | annotation access
    {
        $$ = $1;
        $$.pub = $2.pub;
    }
    ;

access
    : PUBLIC
    {
        memset(&$$, 0, sizeof($$));
        $$.pub.flag = 1;
        $$.pub.loc = loc(@1);
    }
    ;

annotation
    : '@' ID semi
    {
        memset(&$$, 0, sizeof($$));
        Annotation *ann = &$$.ann;
        ann->ident = $2;
        ann->id_loc = loc(@2);
    }
    | '@' ID '(' specialized_types_list ')' semi
    {
        memset(&$$, 0, sizeof($$));
        Annotation *ann = &$$.ann;
        ann->ident = $2;
        ann->id_loc = loc(@2);
        ann->types = $4;
    }
    | '@' ID '(' STRING_LITERAL ')' semi
    {
        memset(&$$, 0, sizeof($$));
        Annotation *ann = &$$.ann;
        ann->ident = $2;
        ann->id_loc = loc(@2);
        Buffer *buf = &ps->sbuf;
        ann->value = atom_nstr(buf->buf, buf->len);
    }
    | '@' ID '(' annot_param_list ')' semi
    {
        memset(&$$, 0, sizeof($$));
        Annotation *ann = &$$.ann;
        ann->ident = $2;
        ann->id_loc = loc(@2);
        ann->params = $4;
    }
    | '@' ID '(' error
    {
        memset(&$$, 0, sizeof($$));
        kl_error(loc(@4), "expected primitive types list.");
        yy_clear_ok;
    }
    ;

annot_param_list
    : annot_param
    {
        $$ = vector_create_ptr();
        vector_push_back($$, &$1);
    }
    | annot_param_list ',' annot_param
    {
        vector_push_back($1, &$3);
        $$ = $1;
    }
    ;

annot_param
    : ID '=' INT_LITERAL
    {
        AnnotParam *param = mm_alloc_obj(param);
        param->key = $1;
        param->ival = $3;
        param->which = ANNOT_PARAM_INT;
        $$ = param;
    }
    | ID '=' STRING_LITERAL
    {
        AnnotParam *param = mm_alloc_obj(param);
        param->key = $1;
        Buffer *buf = &ps->sbuf;
        param->sval = atom_nstr(buf->buf, buf->len);
        param->which = ANNOT_PARAM_STR;
        $$ = param;
    }
    ;

specialized_types_list
    : atom_primitive_type_list
    {
        $$ = vector_create_ptr();

        void *ts;
        vector_foreach(ts, $1) {
            Vector *tp_args = vector_create_ptr();
            vector_push_back(tp_args, &ts);
            vector_push_back($$, &tp_args);
        }

        vector_destroy($1);
    }
    | atom_primitive_type_lists
    {
        $$ = $1;
    }
    ;

atom_primitive_type_lists
    :  '[' atom_primitive_type_list ']'
    {
        $$ = vector_create_ptr();
        vector_push_back($$, &$2);
    }
    | atom_primitive_type_lists ','  '[' atom_primitive_type_list ']'
    {
        vector_push_back($1, &$4);
        $$ = $1;
    }
    ;

atom_primitive_type_list
    : atom_primitive_type
    {
        $$ = vector_create_ptr();
        vector_push_back($$, &$1);
    }
    | atom_primitive_type_list ',' atom_primitive_type
    {
        vector_push_back($1, &$3);
        $$ = $1;
    }
    ;

optional_type
    : base_type
    {
        $$ = $1;
    }
    | lambda_type
    {
        printf("anonymous func type\n");
        type_spec_free($1);
        $$ = NULL;
    }
    ;

base_type
    : type_opt
    {
        $$ = $1;
    }
    | array_type
    {
        $$ = $1;
    }
    ;

type_opt
    : type
    {
        $$ = $1;
    }
    | type '?'
    {
        $$ = optional_type_spec($1);
        type_spec_loc($$, lloc(@1, @2));
    }
    ;

type
    : list_type
    {
        $$ = $1;
    }
    | map_type
    {
        $$ = $1;
    }
    | tuple_type
    {
        $$ = $1;
    }
    | set_type
    {
        $$ = $1;
    }
    | klass_type
    {
        $$ = $1;
    }
    | atom_type
    {
        $$ = $1;
    }
    ;

lambda_type
    : '|' base_type_list '|' base_type
    {
        printf("optional func-type\n");
        free_optional_type_list($2);
        type_spec_free($4);
        $$ = NULL;
    }
    | '|' base_type_list '|'
    {
        printf("no return func-type\n");
        free_optional_type_list($2);
        $$ = NULL;
    }
    | OR base_type
    {
        printf("no parameter func-type\n");
        type_spec_free($2);
        $$ = NULL;
    }
    | OR
    {
        printf("no return no parameter func-type\n");
        $$ = NULL;
    }
    | anony_type_with_brackets
    {
        printf("anonymous func type with brackets\n");
        $$ = NULL;
    }
    | anony_type_with_brackets '?'
    {
        printf("optional anonymous func type with brackets\n");
        $$ = NULL;
    }
    ;

base_type_list
    : base_type
    {
        $$ = vector_create_ptr();
        vector_push_back($$, &$1);
    }
    | base_type_list ',' base_type
    {
        vector_push_back($1, &$3);
        $$ = $1;
    }
    ;

anony_type_with_brackets
    : LAMBDA_START base_type_list '|' base_type '}'
    {
        printf("optional func-type\n");
        free_optional_type_list($2);
        type_spec_free($4);
    }
    | LAMBDA_START base_type_list '|' '}'
    {
        printf("no return func-type\n");
        free_optional_type_list($2);
    }
    | LAMBDA_START_OR base_type '}'
    {
        printf("no parameter func-type\n");
        type_spec_free($2);
    }
    | LAMBDA_START_OR '}'
    {
        printf("no return no parameter func-type\n");
    }
    ;

union_type
    : type '|' type
    {
        $$ = union_type_spec($1, $3);
    }
    | union_type '|' type
    {
        $$ = $1;
        union_type_spec_add($$, $3);
    }
    | type '|' error
    {
        type_spec_free($1);
        kl_error(loc(@3), "expected type.");
        yy_clear_ok;
        $$ = NULL;
    }
    | union_type '|' error
    {
        type_spec_free($1);
        kl_error(loc(@3), "expected type.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

union_opt_type
    : union_type
    {
        $$ = $1;
    }
    | '(' union_type ')'
    {
        $$ = $2;
    }
    | '(' union_type ')' '?'
    {
        $$ = optional_type_spec($2);
        type_spec_loc($$, lloc(@1, @4));
    }
    ;

list_type
    : LIST '[' optional_type ']'
    {
        NAME_ID(id, "list", loc(@1));
        Vector *args = vector_create_ptr();
        vector_push_back(args, &$3);
        MOD_ID(mod, STD_BUILTIN_PATH, loc(@1));
        $$ = unresolved_type_spec(&mod, id, args);
        type_spec_loc($$, lloc(@1, @4));
    }
    | LIST '[' error
    {
        kl_error(loc(@3), "expected type.");
        yy_clear_ok;
        $$ = NULL;
    }
    | LIST '[' optional_type error
    {
        type_spec_free($3);
        kl_error(loc(@4), "expected ']'.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

array_type
    : ARRAY '[' type ',' int_lit_list ']'
    {
        NAME_ID(id, "array", loc(@1));
        Vector *args = vector_create_ptr();
        vector_push_back(args, &$3);
        MOD_ID(mod, STD_BUILTIN_PATH, loc(@1));
        $$ = unresolved_type_spec(&mod, id, args);
        // $$->unresolved.shapes = $5;
        vector_destroy($5);
        type_spec_loc($$, lloc(@1, @6));
    }
    | '[' int_lit_list ']' type
    {
        NAME_ID(id, "array", loc(@1));
        Vector *args = vector_create_ptr();
        vector_push_back(args, &$4);
        MOD_ID(mod, STD_BUILTIN_PATH, loc(@1));
        $$ = unresolved_type_spec(&mod, id, args);
        // $$->unresolved.shapes = $2;
        vector_destroy($2);
        type_spec_loc($$, lloc(@1, @4));
    }
    | ARRAY '[' error ']'
    {
        kl_error(loc(@3), "expected a type and an integer literal list.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

int_lit_list
    : INT_LITERAL
    {
        $$ = vector_create(sizeof(int));
        if ($1 < 0) {
            kl_error(loc(@1), "expected a non-negative integer literal.");
            yy_clear_ok;
        } else {
            vector_push_back($$, &$1);
        }
    }
    | '_'
    {
        $$ = vector_create(sizeof(int));
        int wildcard = -1;
        vector_push_back($$, &wildcard);
    }
    | int_lit_list ',' INT_LITERAL
    {
        $$ = $1;
        if ($3 < 0) {
            kl_error(loc(@3), "expected a non-negative integer literal.");
            yy_clear_ok;
        } else {
            vector_push_back($$, &$3);
        }
    }
    | int_lit_list ',' '_'
    {
        $$ = $1;
        int wildcard = -1;
        vector_push_back($$, &wildcard);
    }
    | int_lit_list ','
    {
        $$ = $1;
    }
    ;

map_type
    : MAP '[' type ',' optional_type ']'
    {
        NAME_ID(id, "dict", loc(@1));
        Vector *args = vector_create_ptr();
        vector_push_back(args, &$3);
        vector_push_back(args, &$5);
        MOD_ID(mod, STD_BUILTIN_PATH, loc(@1));
        $$ = unresolved_type_spec(&mod, id, args);
        type_spec_loc($$, lloc(@1, @6));
    }
    | MAP '[' error
    {
        kl_error(loc(@3), "expected a type.");
        yy_clear_ok;
        $$ = NULL;
    }
    | MAP '[' type  error
    {
        type_spec_free($3);
        kl_error(loc(@4), "expected a ','.");
        yy_clear_ok;
        $$ = NULL;
    }
    | MAP '[' type ',' error
    {
        type_spec_free($3);
        kl_error(loc(@5), "expected a type.");
        yy_clear_ok;
        $$ = NULL;
    }
    | MAP '[' type ',' optional_type error
    {
        type_spec_free($3);
        type_spec_free($5);
        kl_error(loc(@6), "expected a ']'.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

tuple_type
    : TUPLE '[' optional_type_list ']'
    {
        NAME_ID(id, "tuple", loc(@1));
        MOD_ID(mod, STD_BUILTIN_PATH, loc(@1));
        $$ = unresolved_type_spec(&mod, id, $3);
        type_spec_loc($$, lloc(@1, @4));
    }
    | '(' optional_type_list ')'
    {
        NAME_ID(id, "tuple", loc(@1));
        MOD_ID(mod, STD_BUILTIN_PATH, loc(@1));
        $$ = unresolved_type_spec(&mod, id, $2);
        type_spec_loc($$, lloc(@1, @3));
    }
    | TUPLE '[' error
    {
        kl_error(loc(@3), "expected a type or type-list.");
        yy_clear_ok;
        $$ = NULL;
    }
    | TUPLE '[' optional_type_list error
    {
        free_optional_type_list($3);
        kl_error(loc(@4), "expected ']'.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

set_type
    : SET '[' optional_type ']'
    {
        NAME_ID(id, "hashset", loc(@1));
        Vector *args = vector_create_ptr();
        vector_push_back(args, &$3);
        MOD_ID(mod, STD_BUILTIN_PATH, loc(@1));
        $$ = unresolved_type_spec(&mod, id, args);
        type_spec_loc($$, lloc(@1, @4));
    }
    | SET '[' error
    {
        kl_error(loc(@3), "expected a type.");
        yy_clear_ok;
        $$ = NULL;
    }
    | SET '[' type error
    {
        type_spec_free($3);
        kl_error(loc(@4), "expected ']'.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

klass_type
    : ID
    {
        NAME_ID(id, $1, loc(@1));
        $$ = unresolved_type_spec(NULL, id, NULL);
        type_spec_loc($$, loc(@1));
    }
    | ID '.' ID
    {
        MOD_ID(mod, $1, loc(@1));
        NAME_ID(id, $3, loc(@3));
        $$ = unresolved_type_spec(&mod, id, NULL);
        type_spec_loc($$, lloc(@1, @3));
    }
    | ID '[' optional_type_list ']'
    {
        NAME_ID(id, $1, loc(@1));
        $$ = unresolved_type_spec(NULL, id, $3);
        type_spec_loc($$, lloc(@1, @4));
    }
    | ID '.' ID '[' optional_type_list ']'
    {
        MOD_ID(mod, $1, loc(@1));
        NAME_ID(id, $3, loc(@3));
        $$ = unresolved_type_spec(&mod, id, $5);
        type_spec_loc($$, lloc(@1, @6));
    }
    | ID '.' error
    {
        kl_error(loc(@3), "expected and identifier.");
        yy_clear_ok;
        $$ = NULL;
    }
    | ID '[' error
    {
        kl_error(loc(@3), "expected type or type-list.");
        yy_clear_ok;
        $$ = NULL;
    }
    | ID '[' optional_type_list error
    {
        free_optional_type_list($3);
        kl_error(loc(@4), "expected ']'.");
        yy_clear_ok;
        $$ = NULL;
    }
    | ID '.' ID '[' error
    {
        kl_error(loc(@5), "expected type or type-list.");
        yy_clear_ok;
        $$ = NULL;
    }
    | ID '.' ID '[' optional_type_list error
    {
        free_optional_type_list($5);
        kl_error(loc(@6), "expected ']'.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

atom_primitive_type
    : UINT8
    {
        $$ = uint8_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | UINT16
    {
        $$ = uint16_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | UINT32
    {
        $$ = uint32_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | UINT64
    {
        $$ = uint64_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | INT8
    {
        $$ = int8_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | INT16
    {
        $$ = int16_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | INT32
    {
        $$ = int32_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | INT64
    {
        $$ = int64_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | FLOAT16
    {
        $$ = float16_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | FLOAT32
    {
        $$ = float32_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | FLOAT64
    {
        $$ = float64_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | BFLOAT16
    {
        $$ = bfloat16_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | BOOL
    {
        $$ = bool_type_spec();
        type_spec_loc($$, loc(@1));
    }
    ;

atom_type
    : atom_primitive_type
    {
        $$ = $1;
    }
    | STRING
    {
        $$ = str_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | BYTES
    {
        $$ = bytes_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | ANY
    {
        $$ = any_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | TYPE
    {
        $$ = type_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | RANGE
    {
        $$ = klass_type_spec(STD_BUILTIN_PATH, "range");
        type_spec_loc($$, loc(@1));
    }
    ;

optional_type_list
    : optional_type
    {
        $$ = vector_create_ptr();
        vector_push_back($$, &$1);
    }
    | optional_type_list ',' optional_type
    {
        $$ = $1;
        vector_push_back($$, &$3);
    }
    | optional_type_list ',' error
    {
        free_optional_type_list($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

const_type
    : UINT8
    {
        $$ = uint8_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | UINT16
    {
        $$ = uint16_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | UINT32
    {
        $$ = uint32_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | UINT64
    {
        $$ = uint64_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | INT8
    {
        $$ = int8_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | INT16
    {
        $$ = int16_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | INT32
    {
        $$ = int32_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | INT64
    {
        $$ = int64_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | FLOAT16
    {
        $$ = float16_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | FLOAT32
    {
        $$ = float32_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | FLOAT64
    {
        $$ = float64_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | BFLOAT16
    {
        $$ = bfloat16_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | BOOL
    {
        $$ = bool_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | STRING
    {
        $$ = str_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | tuple_type
    {
        $$ = $1;
    }
    ;

let_decl
    : LET ID '=' expr
    {
        Ident id = {$2, loc(@2)};
        $$ = stmt_from_var_decl(id, NULL, 1, $4);
        stmt_set_loc($$, lloc(@1, @4));
    }
    | LET ID optional_type '=' expr
    {
        Ident id = {$2, loc(@2)};
        $$ = stmt_from_var_decl(id, $3, 1, $5);
        stmt_set_loc($$, lloc(@1, @5));
    }
    | LET error
    {
        kl_error(loc(@2), "expected an identifier.");
        yy_clear_ok;
        $$ = NULL;
    }
    | LET ID error
    {
        kl_error(loc(@3), "expected type or '='.");
        yy_clear_ok;
        $$ = NULL;
    }
    | LET ID '=' error
    {
        kl_error(loc(@4), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    | LET ID optional_type error
    {
        type_spec_free($3);
        kl_error(loc(@4), "expected '='.");
        yy_clear_ok;
        $$ = NULL;
    }
    | LET ID optional_type '=' error
    {
        type_spec_free($3);
        kl_error(loc(@5), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

let_else_stmt
    : LET ID '=' expr ELSE block
    {
        Ident id = {$2, loc(@2)};
        $$ = let_else_stmts(&id, NULL, $4, $6, lloc(@1, @4), loc(@5), lloc(@5, @6));
    }
    | LET ID optional_type '=' expr ELSE block
    {
        Ident id = {$2, loc(@2)};
        $$ = let_else_stmts(&id, $3, $5, $7, lloc(@1, @5), loc(@6), lloc(@6, @7));
    }
    | LET ID '=' expr ELSE error
    {
        expr_free($4);
        kl_error(loc(@6), "expected '{' after 'else' in let-else.");
        yy_clear_ok;
        $$ = NULL;
    }
    | LET ID optional_type '=' expr ELSE error
    {
        type_spec_free($3);
        expr_free($5);
        kl_error(loc(@7), "expected '{' after 'else' in let-else.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;


const_decl
    : CONST ID '=' const_expr
    {
        Ident id = {$2, loc(@2)};
        $$ = stmt_from_var_decl(id, NULL, 2, $4);
        stmt_set_loc($$, lloc(@1, @4));
    }
    | CONST ID const_type '=' const_expr
    {
        Ident id = {$2, loc(@2)};
        $$ = stmt_from_var_decl(id, $3, 2, $5);
        stmt_set_loc($$, lloc(@1, @5));
    }
    ;

const_expr
    : INT_LITERAL
    {
        if (errno != 0) {
            kl_error(loc(@1), "Number %s is out of int range", ps->sval);
        }
        $$ = expr_from_lit_int(ps->sval, ps->sign, ps->bit_mode, $1);
        expr_set_loc($$, loc(@1));
    }
    | FLOAT_LITERAL
    {
        if (errno != 0) {
            kl_error(loc(@1), "Number %s is out of float64 range", ps->sval);
        }
        $$ = expr_from_lit_float($1);
        expr_set_loc($$, loc(@1));
    }
    | STRING_LITERAL
    {
        $$ = expr_from_lit_str(&ps->sbuf);
        expr_set_loc($$, loc(@1));
    }
    | CHAR_LITERAL
    {
        $$ = expr_from_lit_char(&ps->sbuf);
        expr_set_loc($$, loc(@1));
    }
    | BYTES_LITERAL
    {
        $$ = expr_from_lit_bytes(&ps->sbuf);
        expr_set_loc($$, loc(@1));
    }
    | TRUE
    {
        $$ = expr_from_lit_bool(1);
        expr_set_loc($$, loc(@1));
    }
    | FALSE
    {
        $$ = expr_from_lit_bool(0);
        expr_set_loc($$, loc(@1));
    }
    | const_tuple_expr
    {
        $$ = $1;
        NYI();
    }
    ;

const_tuple_expr
    : '(' const_expr_list ',' ')'
    {
        $$ = expr_from_const_tuple($2);
        expr_set_loc($$, lloc(@1, @4));
    }
    | '(' const_expr_list ',' const_expr ')'
    {
        // (1, "hello", [1,2,3])
        vector_push_back($2, &$4);
        $$ = expr_from_const_tuple($2);
        expr_set_loc($$, lloc(@1, @5));
    }
    ;

const_expr_list
    : const_expr
    {
        $$ = vector_create_ptr();
        vector_push_back($$, &$1);
    }
    | const_expr_list ',' const_expr
    {
        vector_push_back($1, &$3);
        $$ = $1;
    }
    ;

var_decl
    : VAR ID optional_type
    {
        IDENT(id, $2, loc(@2));
        $$ = stmt_from_var_decl(id, $3, 0, NULL);
        stmt_set_loc($$, lloc(@1, @3));
    }
    | VAR ID optional_type '=' expr
    {
        IDENT(id, $2, loc(@2));
        $$ = stmt_from_var_decl(id, $3, 0, $5);
        stmt_set_loc($$, lloc(@1, @5));
    }
    | VAR ID '=' expr
    {
        IDENT(id, $2, loc(@2));
        $$ = stmt_from_var_decl(id, NULL, 0, $4);
        stmt_set_loc($$, lloc(@1, @4));
    }
    | VAR error
    {
        kl_error(loc(@2), "expected an identifier.");
        yy_clear_ok;
        $$ = NULL;
    }
    | VAR ID error
    {
        kl_error(loc(@3), "expected a type or '='.");
        yy_clear_ok;
        $$ = NULL;
    }
    | VAR ID optional_type error
    {
        type_spec_free($3);
        kl_error(loc(@4), "expected ';' or '='.");
        yy_clear_ok;
        $$ = NULL;
    }
    | VAR ID '=' error
    {
        kl_error(loc(@4), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    | VAR ID optional_type '=' error
    {
        type_spec_free($3);
        kl_error(loc(@5), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

/*
 * let (a, b) = t
 * var (a, _) = t
 * let (a int8, b int?) = (1, nil)
 * only for local variables; '_' binds nothing.
 * Each name may have a type, it is the expected type of that element.
 */
tuple_decl
    : LET '(' tuple_decl_list ')' '=' expr
    {
        $$ = tuple_decl_stmt(ps, $3, VAR_DECL_LET, $6, lloc(@2, @4), lloc(@1, @6));
    }
    | VAR '(' tuple_decl_list ')' '=' expr
    {
        $$ = tuple_decl_stmt(ps, $3, VAR_DECL_VAR, $6, lloc(@2, @4), lloc(@1, @6));
    }
    | LET '(' tuple_decl_list error
    {
        free_ident_type_list($3);
        kl_error(loc(@4), "expected ')'.");
        yy_clear_ok;
        $$ = NULL;
    }
    | VAR '(' tuple_decl_list error
    {
        free_ident_type_list($3);
        kl_error(loc(@4), "expected ')'.");
        yy_clear_ok;
        $$ = NULL;
    }
    | LET '(' tuple_decl_list ')' error
    {
        free_ident_type_list($3);
        kl_error(loc(@5), "expected '='.");
        yy_clear_ok;
        $$ = NULL;
    }
    | VAR '(' tuple_decl_list ')' error
    {
        free_ident_type_list($3);
        kl_error(loc(@5), "expected '='.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

tuple_decl_list
    : ID
    {
        IdentType it = {{$1, loc(@1)}, NULL};
        $$ = vector_create(sizeof(IdentType));
        vector_push_back($$, &it);
    }
    | ID optional_type
    {
        IdentType it = {{$1, loc(@1)}, $2};
        $$ = vector_create(sizeof(IdentType));
        vector_push_back($$, &it);
    }
    | '_'
    {
        IdentType it = {{"_", loc(@1)}, NULL};
        $$ = vector_create(sizeof(IdentType));
        vector_push_back($$, &it);
    }
    | tuple_decl_list ',' ID
    {
        $$ = $1;
        IdentType it = {{$3, loc(@3)}, NULL};
        vector_push_back($$, &it);
    }
    | tuple_decl_list ',' ID optional_type
    {
        $$ = $1;
        IdentType it = {{$3, loc(@3)}, $4};
        vector_push_back($$, &it);
    }
    | tuple_decl_list ',' '_'
    {
        $$ = $1;
        IdentType it = {{"_", loc(@3)}, NULL};
        vector_push_back($$, &it);
    }
    ;

func_decl
    : func_proto_decl block
    {
        $$ = $1;
        ((FuncDeclStmt *)$$)->body = $2;
    }
    | func_proto_decl '=' expr semi
    {
        $$ = $1;
        Vector *block = vector_create_ptr();
        Stmt *s = stmt_from_expr($3);
        stmt_set_loc(s, loc(@3));
        vector_push_back(block, &s);
        ((FuncDeclStmt *)$$)->body = block;
    }
    ;

func_proto_decl
    : FUNC ID '(' param_list ')' optional_type
    {
        IDENT(id, $2, loc(@2));
        $$ = stmt_from_func_decl(id, $4, $6, NULL);
        stmt_set_loc($$, lloc(@1, @6));
    }
    | FUNC ID '(' param_list ')'
    {
        IDENT(id, $2, loc(@2));
        $$ = stmt_from_func_decl(id, $4, NULL, NULL);
        stmt_set_loc($$, lloc(@1, @5));
    }
    | FUNC ID '(' ')' optional_type
    {
        IDENT(id, $2, loc(@2));
        $$ = stmt_from_func_decl(id, NULL, $5, NULL);
        stmt_set_loc($$, lloc(@1, @5));
    }
    | FUNC ID '(' ')'
    {
        IDENT(id, $2, loc(@2));
        $$ = stmt_from_func_decl(id, NULL, NULL, NULL);
        stmt_set_loc($$, lloc(@1, @4));
    }
    | FUNC ID '[' tp_decl_list ']' '(' param_list ')' optional_type
    {
        IDENT(id, $2, loc(@2));
        $$ = stmt_from_func_decl(id, $7, $9, $4);
        stmt_set_loc($$, lloc(@1, @9));
    }
    | FUNC ID '[' tp_decl_list ']' '(' param_list ')'
    {
        IDENT(id, $2, loc(@2));
        $$ = stmt_from_func_decl(id, $7, NULL, $4);
        stmt_set_loc($$, lloc(@1, @8));
    }
    | FUNC ID '[' tp_decl_list ']' '(' ')' optional_type
    {
        IDENT(id, $2, loc(@2));
        $$ = stmt_from_func_decl(id, NULL, $8, $4);
        stmt_set_loc($$, lloc(@1, @8));
    }
    | FUNC ID '[' tp_decl_list ']' '(' ')'
    {
        IDENT(id, $2, loc(@2));
        $$ = stmt_from_func_decl(id, NULL, NULL, $4);
        stmt_set_loc($$, lloc(@1, @7));
    }
    | FUNC ID error
    {
        kl_error(loc(@3), "expected a function prototype.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

param_list
    : id_type_arg_list
    {
        $$ = $1;
    }
    | kw_arg_list
    {
        $$ = $1;
    }
    | id_type_arg_list ',' kw_arg_list
    {
        $$ = $1;
        vector_concat($$, $3);
        vector_destroy($3);
    }
    ;

id_type_arg_list
    : ID param_type
    {
        Ident id = {$1, loc(@1)};
        ParamDecl *p = param_new(lloc(@1, @2), id, $2, NULL);
        $$ = vector_create_ptr();
        vector_push_back($$, &p);
    }
    | ID DOTDOTDOT
    {
        Ident id = {$1, loc(@1)};
        TypeSpec *ts = va_list_type_spec(NULL);
        type_spec_loc(ts, loc(@2));
        ParamDecl *p = param_new(lloc(@1, @2), id, ts, NULL);
        p->va_arg = 1;
        $$ = vector_create_ptr();
        vector_push_back($$, &p);
    }
    | ID DOTDOTDOT param_type
    {
        Ident id = {$1, loc(@1)};
        TypeSpec *ts = va_list_type_spec($3);
        type_spec_loc(ts, loc(@2));
        ParamDecl *p = param_new(lloc(@1, @2), id, ts, NULL);
        p->va_arg = 1;
        $$ = vector_create_ptr();
        vector_push_back($$, &p);
    }
    | id_type_arg_list ',' ID param_type
    {
        $$ = $1;
        Ident id = {$3, loc(@3)};
        ParamDecl *p = param_new(lloc(@3, @4), id, $4, NULL);
        vector_push_back($$, &p);
    }
    | id_type_arg_list ',' ID DOTDOTDOT
    {
        $$ = $1;
        Ident id = {$3, loc(@3)};
        TypeSpec *ts = va_list_type_spec(NULL);
        type_spec_loc(ts, loc(@4));
        ParamDecl *p = param_new(lloc(@3, @4), id, ts, NULL);
        p->va_arg = 1;
        vector_push_back($$, &p);
    }
    | id_type_arg_list ',' ID DOTDOTDOT param_type
    {
        $$ = $1;
        Ident id = {$3, loc(@3)};
        TypeSpec *ts = va_list_type_spec($5);
        type_spec_loc(ts, lloc(@4, @5));
        ParamDecl *p = param_new(lloc(@3, @5), id, ts, NULL);
        p->va_arg = 1;
        vector_push_back($$, &p);
    }
    ;

param_type
    : optional_type
    {
        $$ = $1;
    }
    | union_opt_type
    {
        $$ = $1;
    }
    ;

kw_arg_list
    : kw_arg
    {
        $$ = vector_create_ptr();
        if ($1) vector_push_back($$, &$1);
    }
    | kw_arg_list ',' kw_arg
    {
        $$ = $1;
        if ($3) vector_push_back($$, &$3);
    }
    ;

kw_arg
    : ID '=' expr
    {
        Ident id = {$1, loc(@1)};
        $$ = param_new(lloc(@1, @3), id, NULL, $3);
    }
    | ID optional_type '=' expr
    {
        Ident id = {$1, loc(@1)};
        $$ = param_new(lloc(@1, @4), id, $2, $4);
    }
    | ID union_opt_type '=' expr
    {
        Ident id = {$1, loc(@1)};
        $$ = param_new(lloc(@1, @4), id, $2, $4);
    }
    | ID '=' error
    {
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

class_decl
    : CLASS class_name extends '{' class_members_or_empty '}'
    {
        $$ = stmt_from_klass($2, NULL, $3, $5);
        stmt_set_loc($$, lloc(@1, @6));
    }
    | CLASS class_name '{' class_members_or_empty '}'
    {
        $$ = stmt_from_klass($2, NULL, NULL, $4);
        stmt_set_loc($$, lloc(@1, @5));
    }
    | CLASS class_name '[' tp_decl_list ']' extends '{' class_members_or_empty '}'
    {
        $$ = stmt_from_klass($2, $4, $6, $8);
        stmt_set_loc($$, lloc(@1, @9));
    }
    | CLASS class_name '[' tp_decl_list ']' '{' class_members_or_empty '}'
    {
        $$ = stmt_from_klass($2, $4, NULL, $7);
        stmt_set_loc($$, lloc(@1, @8));
    }
    | CLASS class_name '{' error '}'
    {
        kl_error(loc(@4), "expected field-decl or method-decl.");
        yy_clear_ok;
        $$ = NULL;
    }
    | CLASS class_name '[' error ']' extends '{' class_members_or_empty '}'
    {
        free_klass_list($6);
        free_stmt_list($8);
        kl_error(loc(@4), "expected type-parameter decl-list.");
        yy_clear_ok;
        $$ = NULL;
    }
    | CLASS class_name '[' tp_decl_list ']' extends '{' error '}'
    {
        free_tp_list($4);
        free_klass_list($6);
        kl_error(loc(@8), "expected field-decl or method-decl.");
        yy_clear_ok;
        $$ = NULL;
    }
    | CLASS class_name '[' tp_decl_list ']' '{' error '}'
    {
        free_tp_list($4);
        kl_error(loc(@7), "expected field-decl or method-decl.");
        yy_clear_ok;
        $$ = NULL;
    }
    | CLASS TUPLE '[' INFER ID ']' extends '{' class_members_or_empty '}'
    {
        Ident tp_id = {$5, lloc(@4, @5)};
        TypeParamDecl *tp = infer_type_param_new(lloc(@4, @5), tp_id);
        Vector *tp_list = vector_create_ptr();
        vector_push_back(tp_list, &tp);

        Ident id = {"tuple", loc(@2)};
        TypeSpec *ts = klass_type_spec(STD_BUILTIN_PATH, "tuple");
        IdentType klass_name = {id, ts};
        $$ = stmt_from_klass(klass_name, tp_list, $7, $9);
        stmt_set_loc($$, lloc(@1, @9));
    }
    ;

class_name
    : ID
    {
        Ident id = {$1, loc(@1)};
        TypeSpec *ts = klass_type_spec(PKG_PATH, $1);
        $$ = (IdentType){id, ts};
    }
    | UINT8
    {
        Ident id = {"uint8", loc(@1)};
        TypeSpec *ts = uint8_type_spec();
        $$ = (IdentType){id, ts};
    }
    | UINT16
    {
        Ident id = {"uint16", loc(@1)};
        TypeSpec *ts = uint16_type_spec();
        $$ = (IdentType){id, ts};
    }
    | UINT32
    {
        Ident id = {"uint32", loc(@1)};
        TypeSpec *ts = uint32_type_spec();
        $$ = (IdentType){id, ts};
    }
    | UINT64
    {
        Ident id = {"uint64", loc(@1)};
        TypeSpec *ts = uint64_type_spec();
        $$ = (IdentType){id, ts};
    }
    | INT8
    {
        Ident id = {"int8", loc(@1)};
        TypeSpec *ts = int8_type_spec();
        $$ = (IdentType){id, ts};
    }
    | INT16
    {
        Ident id = {"int16", loc(@1)};
        TypeSpec *ts = int16_type_spec();
        $$ = (IdentType){id, ts};
    }
    | INT32
    {
        Ident id = {"int32", loc(@1)};
        TypeSpec *ts = int32_type_spec();
        $$ = (IdentType){id, ts};
    }
    | INT64
    {
        Ident id = {"int64", loc(@1)};
        TypeSpec *ts = int64_type_spec();
        $$ = (IdentType){id, ts};
    }
    | FLOAT16
    {
        Ident id = {"float16", loc(@1)};
        TypeSpec *ts = float16_type_spec();
        $$ = (IdentType){id, ts};
    }
    | FLOAT32
    {
        Ident id = {"float32", loc(@1)};
        TypeSpec *ts = float32_type_spec();
        $$ = (IdentType){id, ts};
    }
    | FLOAT64
    {
        Ident id = {"float64", loc(@1)};
        TypeSpec *ts = float64_type_spec();
        $$ = (IdentType){id, ts};
    }
    | BFLOAT16
    {
        Ident id = {"bfloat16", loc(@1)};
        TypeSpec *ts = bfloat16_type_spec();
        $$ = (IdentType){id, ts};
    }
    | STRING
    {
        Ident id = {"str", loc(@1)};
        TypeSpec *ts = str_type_spec();
        $$ = (IdentType){id, ts};
    }
    | BYTES
    {
        Ident id = {"bytes", loc(@1)};
        TypeSpec *ts = bytes_type_spec();
        $$ = (IdentType){id, ts};
    }
    | LIST
    {
        Ident id = {"list", loc(@1)};
        TypeSpec *ts = klass_type_spec(STD_BUILTIN_PATH, "list");
        $$ = (IdentType){id, ts};
    }
    | MAP
    {
        Ident id = {"dict", loc(@1)};
        TypeSpec *ts = klass_type_spec(STD_BUILTIN_PATH, "dict");
        $$ = (IdentType){id, ts};
    }
    | SET
    {
        Ident id = {"hashset", loc(@1)};
        TypeSpec *ts = klass_type_spec(STD_BUILTIN_PATH, "hashset");
        $$ = (IdentType){id, ts};
    }
    | RANGE
    {
        Ident id = {"range", loc(@1)};
        TypeSpec *ts = klass_type_spec(STD_BUILTIN_PATH, "range");
        $$ = (IdentType){id, ts};
    }
    | TYPE
    {
        Ident id = {"type", loc(@1)};
        TypeSpec *ts = type_type_spec();
        $$ = (IdentType){id, ts};
    }
    | BOOL
    {
        Ident id = {"bool", loc(@1)};
        TypeSpec *ts = bool_type_spec();
        $$ = (IdentType){id, ts};
    }
    ;

tp_decl_list
    : type_param_list ',' const_tp_list
    {
        vector_concat($1, $3);
        vector_destroy($3);
        $$ = $1;
    }
    | const_tp_list
    {
        $$ = $1;
    }
    | type_param_list
    {
        $$ = $1;
    }
    ;

const_tp_list
    : CONST ID const_type
    {
        Ident id = {$2, loc(@2)};
        TypeParamDecl *tp = const_type_param_new(lloc(@1, @3), id, $3);
        $$ = vector_create_ptr();
        vector_push_back($$, &tp);
    }
    | const_tp_list ',' CONST ID const_type
    {
        $$ = $1;
        Ident id = {$4, loc(@4)};
        TypeParamDecl *tp = const_type_param_new(lloc(@3, @4), id, $5);
        vector_push_back($$, &tp);
    }
    ;

type_param_list
    : type_param_decl
    {
        $$ = vector_create_ptr();
        vector_push_back($$, &$1);
    }
    | type_param_list ',' type_param_decl
    {
        $$ = $1;
        vector_push_back($$, &$3);
    }
    | type_param_list ',' error
    {
        free_tp_list($1);
        kl_error(loc(@3), "expected a type parameter.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

type_param_decl
    : ID
    {
        Ident id = {$1, loc(@1)};
        $$ = type_param_new(loc(@1), id, NULL);
    }
    | ID ':' klass_list
    {
        Ident id = {$1, loc(@1)};
        $$ = type_param_new(lloc(@1, @3), id, $3);
    }
    | INFER ID
    {
        Ident id = {$2, loc(@2)};
        $$ = infer_type_param_new(lloc(@1, @2), id);
    }
    | ID ':' error
    {
        kl_error(loc(@3), "expected up-bound.");
        yy_clear_ok;
        $$ = NULL;
    }
    | ID error
    {
        kl_error(loc(@2), "expected ':' up-bound.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

extends
    : ':' klass_list
    {
        $$ = $2;
    }
    | ':' error
    {
        kl_error(loc(@2), "expected class or trait-list");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

klass_list
    : klass_type
    {
        $$ = vector_create_ptr();
        vector_push_back($$, &$1);
    }
    | klass_list '&' klass_type
    {
        $$ = $1;
        vector_push_back($$, &$3);
    }
    | klass_list '&' error
    {
        free_klass_list($1);
        kl_error(loc(@3), "expected a type.");
        yy_clear_ok;
        $$ = NULL;
    }
    | klass_list error
    {
        free_klass_list($1);
        kl_error(loc(@2), "expected '&' and type.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

class_members_or_empty
    : %empty
    {
        $$ = NULL;
    }
    | field_list
    {
        $$ = $1;
    }
    | method_list
    {
        $$ = $1;
    }
    | field_list method_list
    {
        $$ = $1;
        vector_concat($$, $2);
        vector_destroy($2);
    }
    ;

field_list
    : prefix_field_decl
    {
        $$ = vector_create_ptr();
        vector_push_back($$, &$1);
    }
    | field_list prefix_field_decl
    {
        $$ = $1;
        vector_push_back($$, &$2);
    }
    ;

prefix_field_decl
    : field_decl
    {
        $$ = $1;
    }
    | access field_decl
    {
        $$ = $2;
        stmt_set_prefix($$, $1);
    }
    ;

field_decl
    : var_decl semi
    {
        $$ = $1;
        var_set_where($$, VAR_FIELD);
    }
    | let_decl semi
    {
        $$ = $1;
        var_set_where($$, VAR_FIELD);
    }
    | LET ID optional_type semi
    {
        Ident id = {$2, loc(@2)};
        $$ = stmt_from_var_decl(id, $3, 1, NULL);
        stmt_set_loc($$, lloc(@1, @3));
        var_set_where($$, VAR_FIELD);
    }
    ;

method_list
    : method_decl
    {
        $$ = vector_create_ptr();
        if ($1) vector_push_back($$, &$1);
    }
    | method_list method_decl
    {
        $$ = $1;
        if ($2) vector_push_back($$, &$2);
    }
    ;

method_decl
    : func_decl
    {
        $$ = $1;
    }
    | prefix func_decl
    {
        $$ = $2;
        stmt_set_prefix($$, $1);
    }
    | STATIC func_decl
    {
        $$ = $2;
        SimpleFlag flg = { 1, loc(@1) };
        $$->flags.st = flg;
    }
    | prefix STATIC func_decl
    {
        $$ = $3;
        stmt_set_prefix($$, $1);
        SimpleFlag flg = { 1, loc(@2) };
         $$->flags.st = flg;
    }
    | semi
    {
        $$ = NULL;
    }
    ;

trait_decl
    : TRAIT trait_name '{' trait_members_or_empty '}'
    {
        $$ = stmt_from_trait($2, NULL, NULL, $4);
        stmt_set_loc($$, lloc(@1, @5));
    }
    | TRAIT trait_name extends '{' trait_members_or_empty '}'
    {
        $$ = stmt_from_trait($2, NULL, $3, $5);
        stmt_set_loc($$, lloc(@1, @6));
    }
    | TRAIT trait_name '[' type_param_list ']' '{' trait_members_or_empty '}'
    {
        $$ = stmt_from_trait($2, $4, NULL, $7);
        stmt_set_loc($$, lloc(@1, @8));
    }
    | TRAIT trait_name '[' type_param_list ']' extends '{' trait_members_or_empty '}'
    {
        $$ = stmt_from_trait($2, $4, $6, $8);
        stmt_set_loc($$, lloc(@1, @9));
    }
    ;

trait_name
    : ID
    {
        Ident id = {$1, loc(@1)};
        TypeSpec *ts = klass_type_spec(PKG_PATH, $1);
        $$ = (IdentType){id, ts};
    }
    | ANY
    {
        Ident id = {"any", loc(@1)};
        TypeSpec *ts = any_type_spec();
        $$ = (IdentType){id, ts};
    }
    ;

trait_members_or_empty
    : %empty
    {
        $$ = NULL;
    }
    | trait_method_list
    {
        $$ = $1;
    }
    ;

trait_method_list
    : trait_method
    {
        $$ = vector_create_ptr();
        if ($1) vector_push_back($$, &$1);
    }
    | trait_method_list trait_method
    {
        $$ = $1;
        if ($2) vector_push_back($$, &$2);
    }
    ;

trait_method
    : func_proto_decl
    {
        $$ = $1;
        PrefixFlags flags = { .pub.flag = 1 };
        stmt_set_prefix($$, flags);
    }
    | access func_proto_decl
    {
        $$ = $2;
        $1.pub.flag = 1;
        stmt_set_prefix($$, $1);
    }
    | semi
    {
        $$ = NULL;
    }
    ;

block
    : '{' local_list '}'
    {
        $$ = $2;
    }
    | '{' '}'
    {
        $$ = vector_create_ptr();
    }
    | '{' return_stmt '}'
    {
        $$ = vector_create_ptr();
        vector_push_back($$, &$2);
    }
    | '{' jump_stmt '}'
    {
        $$ = vector_create_ptr();
        vector_push_back($$, &$2);
    }
    | '{' assignment '}'
    {
        $$ = vector_create_ptr();
        vector_push_back($$, &$2);
    }
    | '{' expr '}'
    {
        $$ = vector_create_ptr();
        Stmt *s = stmt_from_expr($2);
        stmt_set_loc(s, loc(@2));
        vector_push_back($$, &s);
    }
    | '{' local_list error
    {
        free_stmt_list($2);
        kl_error(loc(@3), "expected '}'.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

local_list
    : local
    {
        $$ = vector_create_ptr();
        if ($1) vector_push_back($$, &$1);
    }
    | local_list local
    {
        $$ = $1;
        if ($2) vector_push_back($$, &$2);
    }
    | let_else_stmt
    {
        $$ = $1 ? $1 : vector_create_ptr();
    }
    | local_list let_else_stmt
    {
        $$ = $1;
        if ($2) {
            Stmt *s;
            vector_foreach(s, $2) {
                vector_push_back($$, &s);
            }
            vector_destroy($2);
        }
    }
    ;

local
    : expr semi
    {
        $$ = stmt_from_expr($1);
        stmt_set_loc($$, lloc(@1, @2));
    }
    | const_decl semi
    {
        $$ = $1;
        // var_set_where($$, VAR_LOCAL);
    }
    | let_decl semi
    {
        $$ = $1;
        var_set_where($$, VAR_LOCAL);
    }
    | var_decl semi
    {
        $$ = $1;
        var_set_where($$, VAR_LOCAL);
    }
    | tuple_decl semi
    {
        $$ = $1;
    }
    | assignment semi
    {
        $$ = $1;
    }
    | return_stmt semi
    {
        $$ = $1;
    }
    | jump_stmt semi
    {
        $$ = $1;
    }
    | block
    {
        $$ = stmt_from_block($1);
        stmt_set_loc($$, loc(@1));
    }
    | if_stmt
    {
        $$ = $1;
    }
    | while_stmt
    {
        $$ = $1;
    }
    | for_stmt
    {
        $$ = $1;
    }
    | match_stmt
    {
        $$ = NULL;
    }
    | semi
    {
        $$ = NULL;
    }
    ;

assignment
    : assign_left_expr assign_operator expr
    {
        $$ = stmt_from_assignment($2, $1, $3);
        stmt_set_loc($$, lloc(@1, @3));
    }
    | assign_left_expr assign_operator error
    {
        expr_free($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    | '(' expr_list ',' expr ')' assign_operator expr
    {
        // (a, b) = t: the targets are a tuple literal of at least two items
        vector_push_back($2, &$4);
        Expr *lhs = expr_from_tuple($2);
        expr_set_loc(lhs, lloc(@1, @5));
        $$ = stmt_from_tuple_assign($6, loc(@6), lhs, $7);
        stmt_set_loc($$, lloc(@1, @7));
    }
    ;

assign_left_expr
    : ID
    {
        IDENT(id, $1, loc(@1));
        $$ = expr_from_ident(&id);
        expr_set_loc($$, loc(@1));
    }
    | primary_expr '.' ID
    {
        IDENT(id, $3, loc(@3));
        $$ = expr_from_dot($1, &id, DOT_NORMAL);
        expr_set_loc($$, lloc(@1, @3));
    }
    | primary_expr BANG_DOT ID
    {
        IDENT(id, $3, loc(@3));
        $$ = expr_from_dot($1, &id, DOT_BANG);
        expr_set_loc($$, lloc(@1, @3));
    }
    | index_expr
    {
        $$ = $1;
    }
    ;

assign_operator
    : '='
    {
        $$ = OP_ASSIGN;
    }
    | PLUS_ASSIGN
    {
        $$ = OP_PLUS_ASSIGN;
    }
    | MINUS_ASSIGN
    {
        $$ = OP_MINUS_ASSIGN;
    }
    | MULT_ASSIGN
    {
        $$ = OP_MULT_ASSIGN;
    }
    | DIV_ASSIGN
    {
        $$ = OP_DIV_ASSIGN;
    }
    | MOD_ASSIGN
    {
        $$ = OP_MOD_ASSIGN;
    }
    | AND_ASSIGN
    {
        $$ = OP_AND_ASSIGN;
    }
    | OR_ASSIGN
    {
        $$ = OP_OR_ASSIGN;
    }
    | XOR_ASSIGN
    {
        $$ = OP_XOR_ASSIGN;
    }
    | SHL_ASSIGN
    {
        $$ = OP_SHL_ASSIGN;
    }
    | SHR_ASSIGN
    {
        $$ = OP_SHR_ASSIGN;
    }
    ;

return_stmt
    : RETURN
    {
        $$ = stmt_from_return(NULL);
        stmt_set_loc($$, loc(@1));
    }
    | RETURN expr
    {
        $$ = stmt_from_return($2);
        stmt_set_loc($$, lloc(@1, @2));
    }
    ;

jump_stmt
    : BREAK
    {
        $$ = stmt_from_break();
        stmt_set_loc($$, loc(@1));
    }
    | CONTINUE
    {
        $$ = stmt_from_continue();
        stmt_set_loc($$, loc(@1));
    }
    ;

if_stmt
    : IF expr block elseif_stmt
    {
        $$ = stmt_from_if($2, $3, $4);
        stmt_set_loc($$, lloc(@1, @4));
    }
    | IF LET ID '=' expr block elseif_stmt
    {
        IDENT(id, $3, loc(@3));
        $$ = stmt_from_if_let(&id, $5, $6, $7);
        stmt_set_loc($$, lloc(@1, @7));
    }
    | IF '(' LET ID '=' expr ')' block elseif_stmt
    {
        IDENT(id, $4, loc(@4));
        $$ = stmt_from_if_let(&id, $6, $8, $9);
        stmt_set_loc($$, lloc(@1, @9));
    }
    ;

elseif_stmt
    : %empty
    {
        $$ = NULL;
    }
    | ELSE block
    {
        $$ = stmt_from_block($2);
        stmt_set_loc($$, loc(@2));
    }
    | ELSE if_stmt
    {
        $$ = $2;
    }
    ;

while_stmt
    : WHILE expr block
    {
        $$ = stmt_from_while($2, $3);
        stmt_set_loc($$, lloc(@1, @3));
    }
    | WHILE LET ID '=' expr block
    {
        IDENT(id, $3, loc(@3));
        $$ = stmt_from_while_let(&id, $5, $6);
        stmt_set_loc($$, lloc(@1, @6));
    }
    | WHILE '(' LET ID '=' expr ')' block
    {
        IDENT(id, $4, loc(@4));
        $$ = stmt_from_while_let(&id, $6, $8);
        stmt_set_loc($$, lloc(@1, @8));
    }
    | LOOP block
    {
        $$ = stmt_from_while(NULL, $2);
        stmt_set_loc($$, lloc(@1, @2));
    }
    ;

for_stmt
    : FOR id_list IN expr block
    {
        $$ = stmt_from_for($2, $4, $5);
        stmt_set_loc($$, lloc(@1, @5));
    }
    | FOR '(' id_list ')' IN expr block
    {
        $$ = stmt_from_for($3, $6, $7);
        stmt_set_loc($$, lloc(@1, @7));
    }
    /*
     * A bad header skips the tokens up to the loop body, and the body is
     * dropped, so its uses of the loop variables are not reported.
     */
    | FOR id_list error block
    {
        vector_destroy($2);
        free_stmt_list($4);
        kl_error(loc(@3), "expected 'in' after loop variables.");
        yyerrok;
        $$ = NULL;
    }
    | FOR '(' id_list error block
    {
        vector_destroy($3);
        free_stmt_list($5);
        kl_error(loc(@4), "expected ')'.");
        yyerrok;
        $$ = NULL;
    }
    | FOR '(' id_list ')' error block
    {
        vector_destroy($3);
        free_stmt_list($6);
        kl_error(loc(@5), "expected 'in' after loop variables.");
        yyerrok;
        $$ = NULL;
    }
    ;

/*
 * '_' is a placeholder, no variable is bound for it:
 *   for _ in range(n)
 *   for (_, v) in m.items()
 * It is never an ID, the lexer returns '_' token for it.
 */
id_list
    : ID
    {
        Ident id = {$1, loc(@1)};
        $$ = vector_create(sizeof(Ident));
        vector_push_back($$, &id);
    }
    | '_'
    {
        Ident id = {"_", loc(@1)};
        $$ = vector_create(sizeof(Ident));
        vector_push_back($$, &id);
    }
    | id_list ',' ID
    {
        $$ = $1;
        Ident id = {$3, loc(@3)};
        vector_push_back($$, &id);
    }
    | id_list ',' '_'
    {
        $$ = $1;
        Ident id = {"_", loc(@3)};
        vector_push_back($$, &id);
    }
    ;

match_stmt
    : MATCH expr '{' case_list '}'
    {
        expr_free($2);
    }
    ;

case_list
    : case_stmt
    | case_list case_stmt
    ;

case_stmt
    : CASE case_pattern_list ':' case_block case_tail
    ;

case_pattern_list
    : case_pattern
    | case_pattern_list ',' case_pattern
    ;

case_pattern
    : expr
    | IN expr
    {
        expr_free($2);
    }
    | IS type
    {
        type_spec_free($2);
    }
    ;

case_block
    : block
    | expr
    | assignment
    | return_stmt
    ;

case_tail
    : %empty
    | semi
    | ','
    ;

expr
    : or_expr
    {
        $$ = $1;
    }
    | in_expr
    {
        $$ = $1;
    }
    | or_expr OPT_DEF or_expr
    {
        // $$ = expr_from_is_expr($1, loc(@2), $3);
        // expr_set_loc($$, lloc(@1, @3));
        $$ = $1;
        expr_free($3);
    }
    | or_expr OPT_DEF error
    {
        expr_free($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    | or_expr IS type_opt
    {
        $$ = expr_from_is_expr($1, loc(@2), $3);
        expr_set_loc($$, lloc(@1, @3));
    }
    | or_expr AS type_opt
    {
        $$ = expr_from_as_expr($1, loc(@2), $3);
        expr_set_loc($$, lloc(@1, @3));
    }
    ;

in_expr
    : or_expr IN or_expr
    {
        $$ = expr_from_in_expr($1, loc(@2), $3);
        expr_set_loc($$, lloc(@1, @3));
    }
    | or_expr IN error
    {
        expr_free($1);
        kl_error(loc(@3), "expected a type.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

or_expr
    : and_expr
    {
        $$ = $1;
    }
    | or_expr OR and_expr
    {
        $$ = expr_from_binary(BINARY_OR, loc(@2), $1, $3);
        expr_set_loc($$, lloc(@1, @3));
    }
    | or_expr OR error
    {
        expr_free($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

and_expr
    : bit_or_expr
    {
        $$ = $1;
    }
    | and_expr AND bit_or_expr
    {
        $$ = expr_from_binary(BINARY_AND, loc(@2), $1, $3);
        expr_set_loc($$, lloc(@1, @3));
    }
    | and_expr AND error
    {
        expr_free($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

bit_or_expr
    : bit_xor_expr
    {
        $$ = $1;
    }
    | bit_or_expr '|' bit_xor_expr
    {
        $$ = expr_from_binary(BINARY_BIT_OR, loc(@2), $1, $3);
        expr_set_loc($$, lloc(@1, @3));
    }
    | bit_or_expr '|' error
    {
        expr_free($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

bit_xor_expr
    : bit_and_expr
    {
        $$ = $1;
    }
    | bit_xor_expr '^' bit_and_expr
    {
        $$ = expr_from_binary(BINARY_BIT_XOR, loc(@2), $1, $3);
        expr_set_loc($$, lloc(@1, @3));
    }
    | bit_xor_expr '^' error
    {
        expr_free($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

bit_and_expr
    : equality_expr
    {
        $$ = $1;
    }
    | bit_and_expr '&' equality_expr
    {
        $$ = expr_from_binary(BINARY_BIT_AND, loc(@2), $1, $3);
        expr_set_loc($$, lloc(@1, @3));
    }
    | bit_and_expr '&' error
    {
        expr_free($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

equality_expr
    : relation_expr
    {
        $$ = $1;
    }
    | equality_expr EQ relation_expr
    {
        $$ = expr_from_binary(BINARY_EQ, loc(@2), $1, $3);
        expr_set_loc($$, lloc(@1, @3));
    }
    | equality_expr NE relation_expr
    {
        $$ = expr_from_binary(BINARY_NEQ, loc(@2), $1, $3);
        expr_set_loc($$, lloc(@1, @3));
    }
    | equality_expr EQ error
    {
        expr_free($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    | equality_expr NE error
    {
        expr_free($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

relation_expr
    : shift_expr
    {
        $$ = $1;
    }
    | relation_expr '<' shift_expr
    {
        $$ = expr_from_binary(BINARY_LT, loc(@2), $1, $3);
        expr_set_loc($$, lloc(@1, @3));
    }
    | relation_expr '>' shift_expr
    {
        $$ = expr_from_binary(BINARY_GT, loc(@2), $1, $3);
        expr_set_loc($$, lloc(@1, @3));
    }
    | relation_expr LE shift_expr
    {
        $$ = expr_from_binary(BINARY_LE, loc(@2), $1, $3);
        expr_set_loc($$, lloc(@1, @3));
    }
    | relation_expr GE shift_expr
    {
        $$ = expr_from_binary(BINARY_GE, loc(@2), $1, $3);
        expr_set_loc($$, lloc(@1, @3));
    }
    | relation_expr '<' error
    {
        expr_free($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    | relation_expr '>' error
    {
        expr_free($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    | relation_expr LE error
    {
        expr_free($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    | relation_expr GE error
    {
        expr_free($1);
        kl_error(loc(@3), "expected an expression.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

shift_expr
    : add_expr
    {
        $$ = $1;
    }
    | shift_expr R_SHIFT add_expr
    {
        $$ = expr_from_binary(BINARY_SHR, loc(@2), $1, $3);
        expr_set_loc($$, lloc(@1, @3));
    }
    | shift_expr L_SHIFT add_expr
    {
        $$ = expr_from_binary(BINARY_SHL, loc(@2), $1, $3);
        expr_set_loc($$, lloc(@1, @3));
    }
    | shift_expr R_SHIFT error
    {
        expr_free($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    | shift_expr L_SHIFT error
    {
        expr_free($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

add_expr
    : multi_expr
    {
        $$ = $1;
    }
    | add_expr '+' multi_expr
    {
        $$ = expr_from_binary(BINARY_ADD, loc(@2), $1, $3);
        expr_set_loc($$, lloc(@1, @3));
    }
    | add_expr '-' multi_expr
    {
        $$ = expr_from_binary(BINARY_SUB, loc(@2), $1, $3);
        expr_set_loc($$, lloc(@1, @3));
    }
    | add_expr '+' error
    {
        expr_free($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    | add_expr '-' error
    {
        expr_free($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

multi_expr
    : unary_expr
    {
        $$ = $1;
    }
    | multi_expr '*' unary_expr
    {
        $$ = expr_from_binary(BINARY_MUL, loc(@2), $1, $3);
        expr_set_loc($$, lloc(@1, @3));
    }
    | multi_expr '/' unary_expr
    {
        $$ = expr_from_binary(BINARY_DIV, loc(@2), $1, $3);
        expr_set_loc($$, lloc(@1, @3));
    }
    | multi_expr '%' unary_expr
    {
        $$ = expr_from_binary(BINARY_MOD, loc(@2), $1, $3);
        expr_set_loc($$, lloc(@1, @3));
    }
    | multi_expr '*' error
    {
        expr_free($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    | multi_expr '/' error
    {
        expr_free($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    | multi_expr '%' error
    {
        expr_free($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

unary_expr
    : primary_expr
    {
        $$ = $1;
    }
    | '+' unary_expr
    {
        $$ = expr_from_unary(UNARY_PLUS, loc(@1), $2);
        expr_set_loc($$, lloc(@1, @2));
    }
    | '-' unary_expr
    {
        $$ = expr_from_unary(UNARY_NEG, loc(@1), $2);
        expr_set_loc($$, lloc(@1, @2));
    }
    | '~' unary_expr
    {
        $$ = expr_from_unary(UNARY_BIT_NOT, loc(@1), $2);
        expr_set_loc($$, lloc(@1, @2));
    }
    | NOT unary_expr
    {
        $$ = expr_from_unary(UNARY_NOT, loc(@1), $2);
        expr_set_loc($$, lloc(@1, @2));
    }
    | '+' error
    {
        kl_error(loc(@2), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    | '-' error
    {
        kl_error(loc(@2), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    | '~' error
    {
        kl_error(loc(@2), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    | NOT error
    {
        kl_error(loc(@2), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

primary_expr
    : call_expr
    {
        $$ = $1;
    }
    | dot_expr
    {
        $$ = $1;
    }
    | index_expr
    {
        $$ = $1;
    }
    | atom_expr
    {
        $$ = $1;
    }
    | primary_expr NOT
    {
        // only optional type
        $$ = expr_from_bang($1);
        expr_set_loc($$, lloc(@1, @2));
    }
    ;

call_expr
    : primary_expr '(' ')'
    {
        $$ = expr_from_call($1, NULL);
        expr_set_loc($$, lloc(@1, @3));
    }
    | primary_expr '(' call_arg_list ')'
    {
        $$ = expr_from_call($1, $3);
        expr_set_loc($$, lloc(@1, @4));
    }
    | primary_expr '(' error
    {
        expr_free($1);
        kl_error(loc(@3), "expected expr, expr-list or ')'.");
        yy_clear_ok;
        $$ = NULL;
    }
    | primary_expr '(' call_arg_list error
    {
        expr_free($1);
        free_expr_list($3);
        kl_error(loc(@4), "expected an expr list or ')'.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

call_arg_list
    : expr_list
    {
        $$ = $1;
    }
    | call_kw_arg_list
    {
        $$ = $1;
    }
    | expr_list ',' call_kw_arg_list
    {
        $$ = $1;
        vector_concat($$, $3);
        vector_destroy($3);
    }
    ;

call_kw_arg_list
    : ID '=' expr
    {
        Ident id = {$1, loc(@1)};
        Expr *arg = expr_from_keyword(lloc(@1, @3), id, $3);
        $$ = vector_create_ptr();
        vector_push_back($$, &arg);
    }
    | call_kw_arg_list ',' ID '=' expr
    {
        Ident id = {$3, loc(@3)};
        Expr *arg = expr_from_keyword(lloc(@3, @5), id, $5);
        $$ = $1;
        vector_push_back($$, &arg);
    }
    ;

dot_expr
    : primary_expr '.' ID
    {
        IDENT(id, $3, loc(@3));
        $$ = expr_from_dot($1, &id, DOT_NORMAL);
        expr_set_loc($$, lloc(@1, @3));
    }
    | primary_expr '.' ID '?'
    {
        // for type only: z.Foo? in List[z.Foo?]
        printf("NYI: type only: z.Foo?\n");
        expr_free($1);
        $$ = NULL;
        NYI();
    }
    | primary_expr OPT_DOT ID
    {
        IDENT(id, $3, loc(@3));
        $$ = expr_from_dot($1, &id, DOT_OPTIONAL);
        expr_set_loc($$, lloc(@1, @3));
    }
    | primary_expr BANG_DOT ID
    {
        IDENT(id, $3, loc(@3));
        $$ = expr_from_dot($1, &id, DOT_BANG);
        expr_set_loc($$, lloc(@1, @3));
    }
    | primary_expr '.' error
    {
        expr_free($1);
        kl_error(loc(@3), "expected an identifer.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

index_expr
    : primary_expr '[' index_expr_list ']'
    {
        // Foo[1, 2]
        // Foo["hello"]
        // Foo[Bar]()
        // FOO[Bar?]()
        // Foo[Bar, Baz]()
        $$ = expr_from_index($1, $3);
        expr_set_loc($$, lloc(@1, @4));
    }
    | primary_expr '[' index_expr_list ']' '?'
    {
        // for type only: List[List[z.Foo]?]
        printf("NYI: type only: List[List[z.Foo]?]\n");
        expr_free($1);
        free_expr_list($3);
        $$ = NULL;
        NYI();
    }
    | primary_expr '[' error
    {
        expr_free($1);
        kl_error(loc(@3), "expected expr-list.");
        yy_clear_ok;
        $$ = NULL;
    }
    | primary_expr '[' index_expr_list error
    {
        expr_free($1);
        free_expr_list($3);
        kl_error(loc(@4), "expected ']'.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

index_expr_list
    : expr
    {
        $$ = vector_create_ptr();
        vector_push_back($$, &$1);
    }
    | slice_expr
    {
        $$ = vector_create_ptr();
        vector_push_back($$, &$1);
    }
    | index_expr_list ',' expr
    {
        $$ = $1;
        vector_push_back($$, &$3);
    }
    | index_expr_list ',' slice_expr
    {
        $$ = $1;
        vector_push_back($$, &$3);
    }
    ;

slice_expr
    : expr ':' expr ':' expr
    {
        $$ = expr_from_slice($1, $3, $5);
        expr_set_loc($$, lloc(@1, @5));
    }
    | ':' expr ':' expr
    {
        $$ = expr_from_slice(NULL, $2, $4);
        expr_set_loc($$, lloc(@1, @4));
    }
    | expr ':' ':' expr
    {
        $$ = expr_from_slice($1, NULL, $4);
        expr_set_loc($$, lloc(@1, @4));
    }
    | expr ':' expr ':'
    {
        $$ = expr_from_slice($1, $3, NULL);
        expr_set_loc($$, lloc(@1, @3));
    }
    | expr ':' expr
    {
        $$ = expr_from_slice($1, $3, NULL);
        expr_set_loc($$, lloc(@1, @3));
    }
    | ':' expr
    {
        $$ = expr_from_slice(NULL, $2, NULL);
        expr_set_loc($$, lloc(@1, @2));
    }
    | expr ':'
    {
        $$ = expr_from_slice($1, NULL, NULL);
        expr_set_loc($$, lloc(@1, @2));
    }
    | ':' ':' expr
    {
        $$ = expr_from_slice(NULL, NULL, $3);
        expr_set_loc($$, lloc(@1, @3));
    }
    | ':' expr ':'
    {
        $$ = expr_from_slice(NULL, $2, NULL);
        expr_set_loc($$, lloc(@1, @3));
    }
    | expr ':' ':'
    {
        $$ = expr_from_slice($1, NULL, NULL);
        expr_set_loc($$, lloc(@1, @3));
    }
    | ':' ':'
    {
        $$ = expr_from_slice(NULL, NULL, NULL);
        expr_set_loc($$, loc(@1));
    }
    | ':'
    {
        $$ = expr_from_slice(NULL, NULL, NULL);
        expr_set_loc($$, loc(@1));
    }
    ;

atom_expr
    : atom
    {
        $$ = $1;
    }
    | '(' expr ')'
    {
        $$ = $2;
    }
    | '(' error
    {
        kl_error(loc(@2), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    | '(' expr error
    {
        expr_free($2);
        kl_error(loc(@3), "expected ')'.");
        yy_clear_ok;
        $$ = NULL;
    }
    | list_expr
    {
        $$ = $1;
    }
    | map_expr
    {
        $$ = $1;
    }
    | tuple_expr
    {
        $$ = $1;
    }
    | atom_type
    {
        $$ = expr_from_type($1);
        expr_set_loc($$, loc(@1));
    }
    | atom_type '?'
    {
        type_spec_free($1);
        $$ = NULL;
    }
    | ARRAY
    {
        $$ = NULL;
    }
    | lambda_expr
    {
        $$ = $1;
    }
    ;

atom
    : ID
    {
        IDENT(id, $1, loc(@1));
        $$ = expr_from_ident(&id);
        expr_set_loc($$, loc(@1));
    }
    | '_'
    {
        $$ = expr_from_under();
        expr_set_loc($$, loc(@1));
    }
    | INT_LITERAL
    {
        if (errno != 0) {
            kl_error(loc(@1), "Number %s is out of int range", ps->sval);
        }
        $$ = expr_from_lit_int(ps->sval, ps->sign, ps->bit_mode, $1);
        expr_set_loc($$, loc(@1));
    }
    | FLOAT_LITERAL
    {
        if (errno != 0) {
            kl_error(loc(@1), "Number %s is out of float64 range", ps->sval);
        }
        $$ = expr_from_lit_float($1);
        expr_set_loc($$, loc(@1));
    }
    | STRING_LITERAL
    {
        $$ = expr_from_lit_str(&ps->sbuf);
        expr_set_loc($$, loc(@1));
    }
    | CHAR_LITERAL
    {
        $$ = expr_from_lit_char(&ps->sbuf);
        expr_set_loc($$, loc(@1));
    }
    | BYTES_LITERAL
    {
        $$ = expr_from_lit_bytes(&ps->sbuf);
        expr_set_loc($$, loc(@1));
    }
    | TRUE
    {
        $$ = expr_from_lit_bool(1);
        expr_set_loc($$, loc(@1));
    }
    | FALSE
    {
        $$ = expr_from_lit_bool(0);
        expr_set_loc($$, loc(@1));
    }
    | NONE
    {
        $$ = expr_from_lit_none();
        expr_set_loc($$, loc(@1));
    }
    | SELF
    {
        $$ = expr_from_self();
        expr_set_loc($$, loc(@1));
    }
    | CONST_PLACEHOLDER
    {
        $$ = expr_from_const_placeholder(&ps->sbuf);
        expr_set_loc($$, loc(@1));
    }
    ;

list_expr
    : '[' expr_list ']'
    {
        $$ = expr_from_list($2);
        expr_set_loc($$, lloc(@1, @3));
    }
    | '[' expr_list semi ']'
    {
        $$ = expr_from_list($2);
        expr_set_loc($$, lloc(@1, @4));
    }
    | '[' expr_list ',' ']'
    {
        $$ = expr_from_list($2);
        expr_set_loc($$, lloc(@1, @4));
    }
    | '[' ']'
    {
        $$ = expr_from_list(NULL);
        expr_set_loc($$, lloc(@1, @2));
    }
    | LIST
    {
        TypeSpec *ty = klass_type_spec(STD_BUILTIN_PATH, "list");
        type_spec_loc(ty, loc(@1));
        $$ = expr_from_type(ty);
        expr_set_loc($$, loc(@1));
    }
    | SET
    {
        TypeSpec *ty = klass_type_spec(STD_BUILTIN_PATH, "hashset");
        type_spec_loc(ty, loc(@1));
        $$ = expr_from_type(ty);
        expr_set_loc($$, loc(@1));
    }
    | '[' error
    {
        kl_error(loc(@2), "expected expr-list.");
        yy_clear_ok;
        $$ = NULL;
    }
    | '[' expr_list error
    {
        free_expr_list($2);
        kl_error(loc(@3), "expected ',' or ']'.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

map_expr
    : '{' map_list '}'
    {
        $$ = expr_from_map($2);
        expr_set_loc($$, lloc(@1, @3));
    }
    | '{' ':' '}'
    {
        $$ = expr_from_map(NULL);
        expr_set_loc($$, lloc(@1, @3));
    }
    | '{' map_list semi '}'
    {
        $$ = expr_from_map($2);
        expr_set_loc($$, lloc(@1, @4));
    }
    | '{' map_list ',' '}'
    {
        $$ = expr_from_map($2);
        expr_set_loc($$, lloc(@1, @4));
    }
    | MAP
    {
        TypeSpec *ty = klass_type_spec(STD_BUILTIN_PATH, "dict");
        type_spec_loc(ty, loc(@1));
        $$ = expr_from_type(ty);
        expr_set_loc($$, loc(@1));
    }
    | '{' error
    {
        kl_error(loc(@2), "expected key:value pairs.");
        yy_clear_ok;
        $$ = NULL;
    }
    | '{' map_list error
    {
        free_expr_list($2);
        kl_error(loc(@3), "expected ',' or '}'.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

map_list
    : map
    {
        $$ = vector_create_ptr();
        vector_push_back($$, &$1);
    }
    | map_list ',' map
    {
        $$ = $1;
        vector_push_back($$, &$3);
    }
    | map_list ',' error
    {
        free_expr_list($1);
        kl_error(loc(@3), "expected map key-value expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

map
    : expr ':' expr
    {
        $$ = expr_from_map_entry($1, $3);
        expr_set_loc($$, lloc(@1, @3));
    }
    | expr error
    {
        expr_free($1);
        kl_error(loc(@2), "expected ':'.");
        yy_clear_ok;
        $$ = NULL;
    }
    | expr ':' error
    {
        expr_free($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

tuple_expr
    : '(' expr_list ',' ')'
    {
        $$ = expr_from_tuple($2);
        expr_set_loc($$, lloc(@1, @4));
    }
    | '(' expr_list ',' expr ')'
    {
        // (1, "hello", [1,2,3])
        vector_push_back($2, &$4);
        $$ = expr_from_tuple($2);
        expr_set_loc($$, lloc(@1, @5));
    }
    /* | '(' ')'
    {
        $$ = expr_from_tuple(NULL);
        expr_set_loc($$, lloc(@1, @2));
    } */
    | TUPLE
    {
        TypeSpec *ty = klass_type_spec(STD_BUILTIN_PATH, "tuple");
        type_spec_loc(ty, loc(@1));
        $$ = expr_from_type(ty);
        expr_set_loc($$, loc(@1));
    }
    | '(' expr_list ',' error
    {
        free_expr_list($2);
        kl_error(loc(@4), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    | '(' expr_list ',' expr error
    {
        free_expr_list($2);
        expr_free($4);
        kl_error(loc(@5), "expected ')'.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

lambda_expr
    : LAMBDA_START lambda_params '|' expr '}'
    {
        expr_free($4);
        $$ = NULL;
    }
    | LAMBDA_START_OR expr '}'
    {
        expr_free($2);
        $$ = NULL;
    }
    ;

lambda_params
    : lambda_param
    {
    }
    | lambda_params ',' lambda_param
    {
    }
    ;

lambda_param
    : ID
    {
    }
    | ID base_type
    {
        type_spec_free($2);
    }
    ;

expr_list
    : expr
    {
        $$ = vector_create_ptr();
        vector_push_back($$, &$1);
    }
    | expr_list ',' expr
    {
        $$ = $1;
        vector_push_back($$, &$3);
    }
    ;

%%

/* epilogue */

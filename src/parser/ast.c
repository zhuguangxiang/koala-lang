/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "ast.h"
#include "atom.h"

#ifdef __cplusplus
extern "C" {
#endif

Expr *expr_from_lit_int(char *orginal, int sign, int bit_mode, __int128 val)
{
    LitExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_LITERAL_KIND;
    exp->which = LIT_EXPR_INT;
    exp->orginal = orginal; // atom str
    exp->bit_mode = bit_mode;
    exp->sign = sign;
    exp->ival_128 = val;
    exp->ival = 0;
    exp->len = 8;
    exp->ts = sign ? int64_type_spec() : uint64_type_spec();
    return (Expr *)exp;
}

Expr *expr_from_lit_float(double val)
{
    LitExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_LITERAL_KIND;
    exp->which = LIT_EXPR_FLT;
    exp->fval = val;
    exp->len = 8;
    exp->ts = float64_type_spec();
    return (Expr *)exp;
}

Expr *expr_from_lit_bool(int val)
{
    LitExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_LITERAL_KIND;
    exp->which = LIT_EXPR_BOOL;
    exp->bval = val;
    exp->ts = bool_type_spec();
    return (Expr *)exp;
}

static char esc_char(char ch)
{
    char val;
    switch (ch) {
        case 'a':
            val = 7;
            break;
        case 'b':
            val = 8;
            break;
        case 'f':
            val = 12;
            break;
        case 'n':
            val = 10;
            break;
        case 'r':
            val = 13;
            break;
        case 't':
            val = 9;
            break;
        case 'v':
            val = 11;
            break;
        default:
            val = ch;
            break;
    }
    return val;
}

static void do_escape(Buffer *buf)
{
    char *s = buf->buf;
    int len = buf->len;
    if (len == 0) return;

    int j = 0;
    char ch;
    for (int i = 0; i < len; ++i) {
        ch = s[i];
        if (ch == '\\') {
            if (i + 1 < len) {
                ch = s[++i];
                s[j++] = esc_char(ch);
            }
        } else {
            s[j++] = ch;
        }
    }
    ASSERT(j <= len);
    buf->len = j;
    s[j] = '\0';
}

Expr *expr_from_lit_str(Buffer *buf)
{
    LitExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_LITERAL_KIND;
    exp->which = LIT_EXPR_STR;
    exp->sval = mm_alloc_fast(buf->len + 1);
    memcpy(exp->sval, buf->buf, buf->len);
    exp->sval[buf->len] = '\0';
    exp->len = buf->len;
    exp->ts = str_type_spec();
    return (Expr *)exp;
}

Expr *expr_from_lit_char(Buffer *buf)
{
    LitExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_LITERAL_KIND;
    exp->which = LIT_EXPR_CHAR;
    exp->sval = mm_alloc_fast(buf->len + 1);
    memcpy(exp->sval, buf->buf, buf->len);
    exp->sval[buf->len] = '\0';
    exp->len = buf->len;
    exp->ts = uint8_type_spec();
    return (Expr *)exp;
}

Expr *expr_from_lit_bytes(Buffer *buf)
{
    LitExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_LITERAL_KIND;
    exp->which = LIT_EXPR_BYTES;
    exp->sval = mm_alloc_fast(buf->len + 1);
    memcpy(exp->sval, buf->buf, buf->len);
    exp->sval[buf->len] = '\0';
    exp->len = buf->len;
    exp->ts = bytes_type_spec();
    return (Expr *)exp;
}

Expr *expr_from_lit_none(void)
{
    LitExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_LITERAL_KIND;
    exp->which = LIT_EXPR_NONE;
    exp->ts = optional_type_spec_intern(NULL);
    return (Expr *)exp;
}

Literal *expr_to_literal(Expr *e)
{
    ASSERT(e->kind == EXPR_LITERAL_KIND);

    LitExpr *exp = (LitExpr *)e;
    Literal *lit = mm_alloc_obj(lit);
    if (exp->which == LIT_EXPR_INT) {
        lit->which = LIT_INT;
        lit->sign = exp->sign;
        lit->len = exp->len;
        lit->ival = exp->ival;
    } else if (exp->which == LIT_EXPR_FLT) {
        lit->which = LIT_FLT;
        lit->len = exp->len;
        lit->fval = exp->fval;
    } else if (exp->which == LIT_EXPR_BOOL) {
        lit->which = LIT_BOOL;
        lit->bval = exp->bval;
    } else if (exp->which == LIT_EXPR_STR) {
        lit->which = LIT_STR;
        lit->len = exp->len;
        lit->sval = exp->sval;
    } else if (exp->which == LIT_EXPR_CHAR) {
        lit->which = LIT_INT;
        lit->sign = exp->sign;
        lit->len = exp->len;
        lit->ival = exp->ival;
    } else if (exp->which == LIT_EXPR_BYTES) {
        lit->which = LIT_BYTES;
        lit->len = exp->len;
        lit->sval = exp->sval;
    } else if (exp->which == LIT_EXPR_NONE) {
        lit->which = LIT_NONE;
    } else {
        UNREACHABLE();
    }
    return lit;
}

Expr *expr_from_literal(Literal *lit)
{
    LitExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_LITERAL_KIND;
    if (lit->which == LIT_INT) {
        exp->which = LIT_EXPR_INT;
        exp->sign = lit->sign;
        exp->len = lit->len;
        exp->ival = 0;
        /* sign-extend a signed value, e.g. -3 must not become 2^64 - 3 */
        exp->ival_128 = lit->sign ? (__int128)(int64_t)lit->ival : (__int128)lit->ival;
        exp->ts = lit->sign ? int64_type_spec() : uint64_type_spec();
    } else if (lit->which == LIT_FLT) {
        exp->which = LIT_EXPR_FLT;
        exp->len = lit->len;
        exp->fval = lit->fval;
        exp->ts = float64_type_spec();
    } else if (lit->which == LIT_BOOL) {
        exp->which = LIT_EXPR_BOOL;
        exp->bval = lit->bval;
        exp->ts = bool_type_spec();
    } else if (lit->which == LIT_STR) {
        exp->which = LIT_EXPR_STR;
        exp->len = lit->len;
        exp->sval = str_ndup(lit->sval, lit->len);
        exp->ts = str_type_spec();
    } else if (lit->which == LIT_BYTES) {
        exp->which = LIT_EXPR_BYTES;
        exp->len = lit->len;
        exp->sval = str_ndup(lit->sval, lit->len);
        exp->ts = bytes_type_spec();
    } else if (lit->which == LIT_NONE) {
        exp->which = LIT_EXPR_NONE;
        exp->ts = optional_type_spec_intern(NULL);
    } else {
        UNREACHABLE();
    }
    return (Expr *)exp;
}

Expr *expr_from_const_placeholder(Buffer *buf)
{
    ConstPlaceholderExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_CONST_PLACEHOLDER;
    exp->name = mm_alloc_fast(buf->len + 1);
    memcpy(exp->name, buf->buf, buf->len);
    exp->name[buf->len] = '\0';
    return (Expr *)exp;
}

Expr *expr_from_ident(Ident *id)
{
    IdentExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_ID_KIND;
    exp->id = *id;
    return (Expr *)exp;
}

Expr *expr_from_under(void)
{
    Expr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_UNDER_KIND;
    return exp;
}

Expr *expr_from_self(void)
{
    Expr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_SELF_KIND;
    return exp;
}

Expr *expr_from_lambda(Vector *params, Expr *body)
{
    LambdaExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_ANONY_KIND;
    exp->params = params;
    exp->body = body;
    return (Expr *)exp;
}

Expr *expr_from_is_expr(Expr *exp, Loc op_loc, TypeSpec *type)
{
    IsExpr *e = mm_alloc_obj(e);
    e->kind = EXPR_IS_KIND;
    e->exp = exp;
    e->op_loc = op_loc;
    e->type = type;
    e->result = RUNTIME_CHECK;
    return (Expr *)e;
}

Expr *expr_from_as_expr(Expr *exp, Loc op_loc, TypeSpec *type)
{
    AsExpr *e = mm_alloc_obj(e);
    e->kind = EXPR_AS_KIND;
    e->exp = exp;
    e->op_loc = op_loc;
    e->type = type;
    return (Expr *)e;
}

Expr *expr_from_in_expr(Expr *lhs, Loc op_loc, Expr *rhs)
{
    InExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_IN_KIND;
    exp->lhs = lhs;
    exp->op_loc = op_loc;
    exp->rhs = rhs;
    return (Expr *)exp;
}

Expr *expr_from_unary(UnOpKind kind, Loc op_loc, Expr *e)
{
    UnaryExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_UNARY_KIND;
    exp->op = kind;
    exp->exp = e;
    exp->op_loc = op_loc;
    return (Expr *)exp;
}

Expr *expr_from_binary(BiOpKind op, Loc op_loc, Expr *lhs, Expr *rhs)
{
    BinaryExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_BINARY_KIND;
    exp->op = op;
    exp->op_loc = op_loc;
    exp->lhs = lhs;
    exp->rhs = rhs;
    return (Expr *)exp;
}

Expr *expr_from_keyword(Loc loc, Ident key, Expr *value)
{
    KeyWordExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_KW_KIND;
    exp->loc = loc;
    exp->key = key;
    exp->value = value;
    return (Expr *)exp;
}

Expr *expr_from_type(TypeSpec *type)
{
    TypeExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_TYPE_KIND;
    exp->ts = type;
    return (Expr *)exp;
}

Expr *expr_from_optional_type(Expr *src)
{
    OptionalTypeExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_OPTIONAL_TYPE_KIND;
    exp->src = src;
    return (Expr *)exp;
}

Expr *expr_from_list(Vector *vec)
{
    ListExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_LIST_KIND;
    exp->vec = vec;
    return (Expr *)exp;
}

Expr *expr_from_map(Vector *vec)
{
    MapExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_MAP_KIND;
    exp->vec = vec;
    return (Expr *)exp;
}

Expr *expr_from_map_entry(Expr *key, Expr *val)
{
    MapEntryExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_MAP_ENTRY_KIND;
    exp->key = key;
    exp->val = val;
    return (Expr *)exp;
}

Expr *expr_from_tuple(Vector *vec)
{
    TupleExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_TUPLE_KIND;
    exp->vec = vec;
    return (Expr *)exp;
}

Expr *expr_from_call(Expr *lhs, Vector *args)
{
    CallExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_CALL_KIND;
    exp->lhs = lhs;
    exp->args = args;
    return (Expr *)exp;
}

Expr *expr_from_dot(Expr *lhs, Ident *id, int opt_or_bang)
{
    DotExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_DOT_KIND;
    exp->lhs = lhs;
    exp->id = *id;
    exp->opt_or_bang = opt_or_bang;
    return (Expr *)exp;
}

Expr *expr_from_index(Expr *lhs, Vector *vec)
{
    IndexExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_INDEX_KIND;
    exp->lhs = lhs;
    exp->vec = vec;
    return (Expr *)exp;
}

Expr *expr_from_slice(Expr *start, Expr *stop, Expr *step)
{
    SliceExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_SLICE_KIND;
    exp->start = start;
    exp->end = stop;
    exp->step = step;
    return (Expr *)exp;
}

Expr *expr_from_bang(Expr *exp)
{
    BangExpr *e = mm_alloc_obj(e);
    e->kind = EXPR_BANG_KIND;
    e->exp = exp;
    return (Expr *)e;
}

Expr *expr_from_if(Expr *cond, Expr *then_exp, Expr *else_exp)
{
    IfExpr *e = mm_alloc_obj(e);
    e->kind = EXPR_IF_KIND;
    e->cond = cond;
    e->then_exp = then_exp;
    e->else_exp = else_exp;
    return (Expr *)e;
}

Expr *expr_from_coalesce(Expr *lhs, Expr *rhs)
{
    CoalesceExpr *e = mm_alloc_obj(e);
    e->kind = EXPR_COALESCE_KIND;
    e->lhs = lhs;
    e->rhs = rhs;
    return (Expr *)e;
}

static void ident_expr_free(Expr *exp) { mm_free(exp); }
static void under_expr_free(Expr *exp) { mm_free(exp); }

static void lit_expr_free(Expr *exp)
{
    LitExpr *lit = (LitExpr *)exp;
    if (lit->which == LIT_EXPR_STR || lit->which == LIT_EXPR_BYTES) mm_free(lit->sval);
    mm_free(exp);
}

static void self_expr_free(Expr *exp) { mm_free(exp); }

static void list_expr_free(Expr *exp)
{
    ListExpr *list = (ListExpr *)exp;
    Expr *e;
    vector_foreach(e, list->vec) {
        if (!e) continue;
        expr_free(e);
    }
    vector_destroy(list->vec);
    mm_free(exp);
}

static void map_expr_free(Expr *exp)
{
    MapExpr *map = (MapExpr *)exp;
    Expr *e;
    vector_foreach(e, map->vec) {
        if (!e) continue;
        expr_free(e);
    }
    vector_destroy(map->vec);
    mm_free(exp);
}

static void map_entry_expr_free(Expr *exp)
{
    MapEntryExpr *entry = (MapEntryExpr *)exp;
    expr_free(entry->key);
    expr_free(entry->val);
    mm_free(exp);
}
static void tuple_expr_free(Expr *exp)
{
    TupleExpr *tuple = (TupleExpr *)exp;
    Expr *e;
    vector_foreach(e, tuple->vec) {
        if (!e) continue;
        expr_free(e);
    }
    vector_destroy(tuple->vec);
    mm_free(exp);
}

static void anony_expr_free(Expr *exp)
{
    LambdaExpr *lambda = (LambdaExpr *)exp;
    ParamDecl *param;
    vector_foreach(param, lambda->params) {
        type_spec_free(param->type);
        expr_free(param->value);
        mm_free(param);
    }
    vector_destroy(lambda->params);
    expr_free(lambda->body);
    stbl_free(lambda->scope);
    mm_free(exp);
}

static void type_expr_free(Expr *exp) { mm_free(exp); }

static void optional_type_expr_free(Expr *exp)
{
    OptionalTypeExpr *e = (OptionalTypeExpr *)exp;
    expr_free(e->src);
    mm_free(exp);
}

static void call_expr_free(Expr *exp)
{
    CallExpr *e = (CallExpr *)exp;
    expr_free(e->lhs);

    Expr *arg;
    vector_foreach(arg, e->args) {
        if (!arg) continue;
        expr_free(arg);
    }
    vector_destroy(e->args);
    vector_destroy(e->tp_args);

    mm_free(exp);
}

static void dot_expr_free(Expr *exp)
{
    DotExpr *e = (DotExpr *)exp;
    expr_free(e->lhs);
    mm_free(exp);
}

static void index_expr_free(Expr *exp)
{
    IndexExpr *e = (IndexExpr *)exp;
    expr_free(e->lhs);
    Expr *idx;
    vector_foreach(idx, e->vec) {
        if (!idx) continue;
        expr_free(idx);
    }
    vector_destroy(e->vec);
    vector_destroy(e->tp_args);
    mm_free(exp);
}

static void slice_expr_free(Expr *exp)
{
    SliceExpr *slice = (SliceExpr *)exp;
    expr_free(slice->start);
    expr_free(slice->end);
    expr_free(slice->step);
    mm_free(exp);
}

static void unary_expr_free(Expr *exp)
{
    UnaryExpr *e = (UnaryExpr *)exp;
    expr_free(e->exp);
    mm_free(exp);
}

static void binary_expr_free(Expr *exp)
{
    BinaryExpr *e = (BinaryExpr *)exp;
    expr_free(e->lhs);
    expr_free(e->rhs);
    mm_free(exp);
}

static void keyword_expr_free(Expr *exp)
{
    KeyWordExpr *e = (KeyWordExpr *)exp;
    expr_free(e->value);
    mm_free(exp);
}

static void is_expr_free(Expr *exp) { mm_free(exp); }
static void as_expr_free(Expr *exp) { mm_free(exp); }
static void in_expr_free(Expr *exp) { mm_free(exp); }

static void bang_expr_free(Expr *exp)
{
    BangExpr *e = (BangExpr *)exp;
    expr_free(e->exp);
    mm_free(exp);
}

static void coalesce_expr_free(Expr *exp)
{
    CoalesceExpr *e = (CoalesceExpr *)exp;
    expr_free(e->lhs);
    expr_free(e->rhs);
    mm_free(exp);
}

static void if_expr_free(Expr *exp)
{
    IfExpr *e = (IfExpr *)exp;
    expr_free(e->cond);
    expr_free(e->then_exp);
    expr_free(e->else_exp);
    mm_free(exp);
}

static void panic_expr_free(Expr *exp) { mm_free(exp); }

static void const_placeholder_expr_free(Expr *exp) { mm_free(exp); }

void expr_free(Expr *exp)
{
    if (!exp) return;

    static void (*free_handlers[EXPR_MAX_KIND])(Expr *) = {
        [EXPR_UNK_KIND] = NULL,
        [EXPR_ID_KIND] = ident_expr_free,
        [EXPR_UNDER_KIND] = under_expr_free,
        [EXPR_LITERAL_KIND] = lit_expr_free,
        [EXPR_SELF_KIND] = self_expr_free,
        [EXPR_LIST_KIND] = list_expr_free,
        [EXPR_MAP_KIND] = map_expr_free,
        [EXPR_MAP_ENTRY_KIND] = map_entry_expr_free,
        [EXPR_TUPLE_KIND] = tuple_expr_free,
        [EXPR_ANONY_KIND] = anony_expr_free,
        [EXPR_TYPE_KIND] = type_expr_free,
        [EXPR_OPTIONAL_TYPE_KIND] = optional_type_expr_free,
        [EXPR_CALL_KIND] = call_expr_free,
        [EXPR_DOT_KIND] = dot_expr_free,
        [EXPR_INDEX_KIND] = index_expr_free,
        [EXPR_SLICE_KIND] = slice_expr_free,
        [EXPR_UNARY_KIND] = unary_expr_free,
        [EXPR_BINARY_KIND] = binary_expr_free,
        [EXPR_KW_KIND] = keyword_expr_free,
        [EXPR_IS_KIND] = is_expr_free,
        [EXPR_AS_KIND] = as_expr_free,
        [EXPR_IN_KIND] = in_expr_free,
        [EXPR_BANG_KIND] = bang_expr_free,
        [EXPR_COALESCE_KIND] = coalesce_expr_free,
        [EXPR_IF_KIND] = if_expr_free,
        [EXPR_CONST_PLACEHOLDER] = const_placeholder_expr_free,
    };

    free_handlers[exp->kind](exp);
}

Stmt *stmt_from_import(Buffer *buf, char *alias, Vector *names)
{
    ImportStmt *s = mm_alloc_obj(s);
    s->kind = STMT_IMPORT_KIND;
    s->path = atom_nstr(buf->buf, buf->len);
    s->alias = alias ? atom(alias) : NULL;
    s->names = names;
    return (Stmt *)s;
}

Stmt *stmt_from_link(Buffer *buf)
{
    LinkStmt *s = mm_alloc_obj(s);
    s->kind = STMT_LINK_KIND;
    s->path = atom_nstr(buf->buf, buf->len);
    return (Stmt *)s;
}

Stmt *stmt_from_var_decl(Ident id, TypeSpec *ty, int which, Expr *e)
{
    VarDeclStmt *s = mm_alloc_obj(s);
    s->kind = STMT_VAR_KIND;
    s->id = id;
    s->which = which;
    s->type = ty;
    s->exp = e;
    return (Stmt *)s;
}

TypeParamDecl *type_param_new(Loc loc, Ident id, Vector *bound, TypeSpec *dfl)
{
    TypeParamDecl *tp = mm_alloc_obj(tp);
    tp->loc = loc;
    tp->id = id;
    tp->which = TP_DECL_NORMAL;
    tp->bound = bound;
    tp->dfl_ts = dfl;
    return tp;
}

TypeParamDecl *infer_type_param_new(Loc loc, Ident id)
{
    TypeParamDecl *tp = mm_alloc_obj(tp);
    tp->loc = loc;
    tp->id = id;
    tp->which = TP_DECL_INFER;
    return tp;
}

TypeParamDecl *const_type_param_new(Loc loc, Ident id, TypeSpec *type)
{
    TypeParamDecl *tp = mm_alloc_obj(tp);
    tp->loc = loc;
    tp->id = id;
    tp->which = TP_DECL_CONST;
    tp->const_type = type;
    return tp;
}

ParamDecl *param_new(Loc loc, Ident id, TypeSpec *type, Expr *value)
{
    ParamDecl *p = mm_alloc_obj(p);
    p->loc = loc;
    p->id = id;
    p->type = type;
    p->value = value;
    return p;
}

Stmt *stmt_from_func_decl(Ident id, Vector *args, TypeSpec *ret, Vector *tps)
{
    FuncDeclStmt *s = mm_alloc_obj(s);
    s->kind = STMT_FUNC_KIND;
    s->id = id;
    s->args = args;
    s->ret = ret;
    s->tps = tps;
    return (Stmt *)s;
}

Stmt *stmt_from_assignment(AssignOpKind op, Expr *lhs, Expr *rhs)
{
    AssignStmt *s = mm_alloc_obj(s);
    s->kind = STMT_ASSIGN_KIND;
    s->op = op;
    s->lhs = lhs;
    s->rhs = rhs;
    return (Stmt *)s;
}

Stmt *stmt_from_block(Vector *stmts)
{
    BlockStmt *s = mm_alloc_obj(s);
    s->kind = STMT_BLOCK_KIND;
    s->stmts = stmts;
    return (Stmt *)s;
}

Stmt *stmt_from_if(Expr *cond, Vector *block, Stmt *_else)
{
    IfStmt *s = mm_alloc_obj(s);
    s->kind = STMT_IF_KIND;
    s->cond = cond;
    s->block = block;
    s->_else = _else;
    return (Stmt *)s;
}

Stmt *stmt_from_if_let(Ident *id, Expr *exp, Vector *block, Stmt *_else)
{
    IfLetStmt *s = mm_alloc_obj(s);
    s->kind = STMT_IF_LET_KIND;
    s->id = *id;
    s->cond = exp;
    s->block = block;
    s->_else = _else;
    return (Stmt *)s;
}

Stmt *stmt_from_while(Expr *cond, Vector *block)
{
    WhileStmt *s = mm_alloc_obj(s);
    s->kind = STMT_WHILE_KIND;
    s->cond = cond;
    s->block = block;
    return (Stmt *)s;
}

Stmt *stmt_from_while_let(Ident *id, Expr *exp, Vector *block)
{
    WhileLetStmt *s = mm_alloc_obj(s);
    s->kind = STMT_WHILE_LET_KIND;
    s->id = *id;
    s->cond = exp;
    s->block = block;
    return (Stmt *)s;
}

/*
 * with expr { ... }: 'id' is NULL, 'exp' is the resource, a plain name 'r' binds 'r' in the block.
 * with v = expr { ... }: 'id' is the new variable 'v' bound to the resource 'exp'.
 */
Stmt *stmt_from_with(Ident *id, Expr *exp, Vector *block)
{
    WithStmt *s = mm_alloc_obj(s);
    s->kind = STMT_WITH_KIND;
    s->exp = exp;
    s->block = block;
    if (id) {
        s->id = *id;
    } else {
        s->is_ref = 1;
        if (exp->kind == EXPR_ID_KIND) s->id = ((IdentExpr *)exp)->id;
    }
    return (Stmt *)s;
}

/* with let v = expr { ... } else { ... }: 'else_block' may be NULL */
Stmt *stmt_from_with_let(Ident *id, Expr *exp, Vector *block, Vector *else_block)
{
    Stmt *s = stmt_from_with(id, exp, block);
    WithStmt *ws = (WithStmt *)s;
    ws->is_let = 1;
    ws->else_block = else_block;
    return s;
}

Stmt *stmt_from_for(Vector *ids, Expr *iterable, Vector *block)
{
    ForStmt *s = mm_alloc_obj(s);
    s->kind = STMT_FOR_KIND;
    s->ids = ids;
    s->iterable = iterable;
    s->block = block;
    vector_init(&s->sym_ids, sizeof(int));
    return (Stmt *)s;
}

CasePattern *case_pattern_new(CasePatternKind kind, Expr *exp, TypeSpec *type, Loc loc)
{
    CasePattern *pat = mm_alloc_obj(pat);
    pat->kind = kind;
    pat->loc = loc;
    pat->exp = exp;
    pat->type = type;
    return pat;
}

void case_pattern_free(CasePattern *pat)
{
    if (!pat) return;
    expr_free(pat->exp);
    /* 'type' is the raw grammar type, consumed (freed) by the semantic pass;
       'target_ts' is interned and owned by the type system */
    type_spec_free(pat->type);
    mm_free(pat);
}

CaseClause *case_clause_new(Vector *patterns, Vector *block, Loc loc)
{
    CaseClause *clause = mm_alloc_obj(clause);
    clause->loc = loc;
    clause->patterns = patterns;
    clause->block = block;
    return clause;
}

void case_clause_free(CaseClause *clause)
{
    if (!clause) return;
    CasePattern *pat;
    vector_foreach(pat, clause->patterns) {
        case_pattern_free(pat);
    }
    vector_destroy(clause->patterns);
    Stmt *st;
    vector_foreach(st, clause->block) {
        stmt_free(st);
    }
    vector_destroy(clause->block);
    mm_free(clause);
}

Stmt *stmt_from_switch(Expr *exp, Vector *clauses)
{
    SwitchStmt *s = mm_alloc_obj(s);
    s->kind = STMT_SWITCH_KIND;
    s->exp = exp;
    s->clauses = clauses;
    return (Stmt *)s;
}

Stmt *stmt_from_tuple_decl(Vector *ids, int which, Expr *exp)
{
    TupleDeclStmt *s = mm_alloc_obj(s);
    s->kind = STMT_TUPLE_DECL_KIND;
    s->ids = ids;
    s->which = which;
    s->exp = exp;
    vector_init(&s->sym_ids, sizeof(int));
    return (Stmt *)s;
}

Stmt *stmt_from_tuple_assign(AssignOpKind op, Loc op_loc, Expr *lhs, Expr *rhs)
{
    TupleAssignStmt *s = mm_alloc_obj(s);
    s->kind = STMT_TUPLE_ASSIGN_KIND;
    s->op = op;
    s->op_loc = op_loc;
    s->lhs = lhs;
    s->rhs = rhs;
    return (Stmt *)s;
}

Stmt *stmt_from_expr(Expr *exp)
{
    ExprStmt *s = mm_alloc_obj(s);
    s->kind = STMT_EXPR_KIND;
    s->exp = exp;
    return (Stmt *)s;
}

Stmt *stmt_from_type(StmtKind kind, IdentType name, Vector *tps, Vector *bases, Vector *stmts)
{
    KlassDeclStmt *s = mm_alloc_obj(s);
    s->kind = kind;
    s->id = name.id;
    s->ts = name.ts;
    s->tps = tps;
    s->bases = bases;
    s->stmts = stmts;
    return (Stmt *)s;
}

EnumMember *enum_member_new(Ident id, int has_value, int64_t value, Loc value_loc)
{
    EnumMember *m = mm_alloc_obj(m);
    m->id = id;
    m->has_value = has_value;
    m->value = value;
    m->value_loc = value_loc;
    return m;
}

/*
 * The pub name() function desugared from an enum, built at AST construction time:
 *   pub func Color$name(v int64) str {
 *       switch v {
 *           case 0: return "RED"
 *           case 5: return "GREEN"
 *           case _: panic("unreachable: invalid enum value")
 *       }
 *   }
 * It is recorded on the enum stmt and registered as an ordinary top-level function
 * before semantic analysis, so the switch lowering and the pub klc export need no
 * special code. An int literal converts to an enum only when it is a declared member
 * value, so the panic case is a fuse that never fires. Values auto-increment exactly
 * like the semantic pass computes them; validation stays in the semantic pass.
 */
static Stmt *enum_name_func(EnumDeclStmt *e)
{
    Loc loc = e->id.loc;

    char name[512];
    snprintf(name, sizeof(name), "%s$name", e->id.name);
    IDENT(fn_id, atom(name), loc);

    IDENT(v_id, atom("v"), loc);
    ParamDecl *param = param_new(loc, v_id, int64_type_spec(), NULL);
    Vector *params = vector_create_ptr();
    vector_push_back(params, &param);

    Vector *clauses = vector_create_ptr();
    int64_t next = 0;
    EnumMember *m;
    vector_foreach(m, e->members) {
        int64_t val = m->has_value ? m->value : next;
        next = (val == INT64_MAX) ? 0 : val + 1;

        char text[32];
        snprintf(text, sizeof(text), "%lld", (long long)val);
        Expr *lit = expr_from_lit_int(atom(text), 1, 0, (__int128)val);
        expr_set_loc(lit, m->value_loc);
        CasePattern *pat = case_pattern_new(CASE_PAT_VALUE, lit, NULL, m->value_loc);
        Vector *pats = vector_create_ptr();
        vector_push_back(pats, &pat);

        Buffer strbuf = { 0, (int)strlen(m->id.name), m->id.name };
        Expr *str = expr_from_lit_str(&strbuf);
        expr_set_loc(str, m->id.loc);
        Stmt *ret = stmt_from_return(str);
        stmt_set_loc(ret, m->id.loc);
        Vector *body = vector_create_ptr();
        vector_push_back(body, &ret);

        CaseClause *clause = case_clause_new(pats, body, m->id.loc);
        vector_push_back(clauses, &clause);
    }

    IDENT(panic_id, atom("panic"), loc);
    Expr *callee = expr_from_ident(&panic_id);
    expr_set_loc(callee, loc);
    char *msg = "unreachable: invalid enum value";
    Buffer msgbuf = { 0, (int)strlen(msg), msg };
    Expr *arg = expr_from_lit_str(&msgbuf);
    expr_set_loc(arg, loc);
    Vector *args = vector_create_ptr();
    vector_push_back(args, &arg);
    Expr *panic_call = expr_from_call(callee, args);
    expr_set_loc(panic_call, loc);
    Stmt *panic_stmt = stmt_from_expr(panic_call);
    stmt_set_loc(panic_stmt, loc);
    Vector *panic_body = vector_create_ptr();
    vector_push_back(panic_body, &panic_stmt);
    CasePattern *wild = case_pattern_new(CASE_PAT_WILD, NULL, NULL, loc);
    Vector *wild_pats = vector_create_ptr();
    vector_push_back(wild_pats, &wild);
    CaseClause *dfl = case_clause_new(wild_pats, panic_body, loc);
    vector_push_back(clauses, &dfl);

    IDENT(subject_id, atom("v"), loc);
    Expr *subject = expr_from_ident(&subject_id);
    expr_set_loc(subject, loc);
    Stmt *sw = stmt_from_switch(subject, clauses);
    stmt_set_loc(sw, loc);
    Vector *fn_body = vector_create_ptr();
    vector_push_back(fn_body, &sw);

    Stmt *fn = stmt_from_func_decl(fn_id, params, str_type_spec(), NULL);
    stmt_set_loc(fn, loc);
    ((FuncDeclStmt *)fn)->body = fn_body;
    PrefixFlags prefix;
    memset(&prefix, 0, sizeof(prefix));
    prefix.pub.flag = 1;
    prefix.pub.loc = loc;
    stmt_set_prefix(fn, prefix);
    return fn;
}

Stmt *stmt_from_enum(Ident id, Vector *members)
{
    EnumDeclStmt *s = mm_alloc_obj(s);
    s->kind = STMT_ENUM_KIND;
    s->id = id;
    s->members = members;
    s->name_func = enum_name_func(s);
    return (Stmt *)s;
}

Stmt *stmt_from_type_alias(Ident id, TypeSpec *ts)
{
    TypeAliasDeclStmt *s = mm_alloc_obj(s);
    s->kind = STMT_TYPE_ALIAS_KIND;
    s->id = id;
    s->ts = ts;
    return (Stmt *)s;
}

Stmt *stmt_from_return(Expr *exp)
{
    RetStmt *s = mm_alloc_obj(s);
    s->kind = STMT_RETURN_KIND;
    s->exp = exp;
    return (Stmt *)s;
}

Stmt *stmt_from_continue(void)
{
    Stmt *s = mm_alloc_obj(s);
    s->kind = STMT_CONTINUE_KIND;
    return s;
}

Stmt *stmt_from_break(void)
{
    Stmt *s = mm_alloc_obj(s);
    s->kind = STMT_BREAK_KIND;
    return s;
}

static void import_stmt_free(Stmt *stmt)
{
    ImportStmt *s = (ImportStmt *)stmt;
    vector_destroy(s->names);
    mm_free(stmt);
}

static void link_stmt_free(Stmt *stmt)
{
    LinkStmt *s = (LinkStmt *)stmt;
    mm_free(stmt);
}

static void var_decl_stmt_free(Stmt *stmt)
{
    VarDeclStmt *s = (VarDeclStmt *)stmt;
    expr_free(s->exp);
    mm_free(s);
}

/* types/params are owned by the annotation; ident/value are atoms.
   params may have been transferred to FuncSymbol.ann_params (bench). */
static void annotation_free(Annotation *ann)
{
    Vector *tp_args;
    vector_foreach(tp_args, ann->types) {
        vector_destroy(tp_args);
    }
    vector_destroy(ann->types);

    AnnotParam *param;
    vector_foreach(param, ann->params) {
        mm_free(param);
    }
    vector_destroy(ann->params);
}

static void func_decl_stmt_free(Stmt *stmt)
{
    FuncDeclStmt *s = (FuncDeclStmt *)stmt;

    annotation_free(&s->flags.ann);

    Vector *tps = s->tps;
    TypeParamDecl *tp;
    vector_foreach(tp, tps) {
        if (!tp) continue;
        vector_destroy(tp->bound);
        mm_free(tp);
    }
    vector_destroy(tps);

    Vector *args = s->args;
    ParamDecl *p;
    vector_foreach(p, args) {
        if (!p) continue;
        expr_free(p->value);
        mm_free(p);
    }
    vector_destroy(args);

    Stmt *_s;
    vector_foreach(_s, s->body) {
        if (!_s) continue;
        stmt_free(_s);
    }
    vector_destroy(s->body);

    mm_free(s);
}

static void enum_decl_stmt_free(Stmt *stmt)
{
    EnumDeclStmt *s = (EnumDeclStmt *)stmt;
    EnumMember *m;
    vector_foreach(m, s->members) {
        mm_free(m);
    }
    vector_destroy(s->members);
    mm_free(s);
}

static void type_alias_decl_stmt_free(Stmt *stmt)
{
    // ts is owned by the symbol registered for this alias
    mm_free(stmt);
}

static void klass_decl_stmt_free(Stmt *stmt)
{
    KlassDeclStmt *s = (KlassDeclStmt *)stmt;

    annotation_free(&s->flags.ann);

    Vector *tps = s->tps;
    TypeParamDecl *tp;
    vector_foreach(tp, tps) {
        if (!tp) continue;
        vector_destroy(tp->bound);
        mm_free(tp);
    }
    vector_destroy(tps);

    vector_destroy(s->bases);

    Stmt *_s;
    vector_foreach(_s, s->stmts) {
        if (!_s) continue;
        stmt_free(_s);
    }
    vector_destroy(s->stmts);

    mm_free(s);
}

static void ret_stmt_free(Stmt *stmt)
{
    RetStmt *s = (RetStmt *)stmt;
    expr_free(s->exp);
    mm_free(s);
}

static void assign_stmt_free(Stmt *stmt)
{
    AssignStmt *s = (AssignStmt *)stmt;
    expr_free(s->lhs);
    expr_free(s->rhs);
    mm_free(s);
}

static void break_stmt_free(Stmt *stmt) { mm_free(stmt); }
static void continue_stmt_free(Stmt *stmt) { mm_free(stmt); }

static void expr_stmt_free(Stmt *stmt)
{
    ExprStmt *s = (ExprStmt *)stmt;
    expr_free(s->exp);
    mm_free(s);
}

static void block_stmt_free(Stmt *stmt)
{
    BlockStmt *s = (BlockStmt *)stmt;
    Stmt *st;
    vector_foreach(st, s->stmts) {
        if (!st) continue;
        stmt_free(st);
    }
    vector_destroy(s->stmts);
    mm_free(s);
}

static void if_stmt_free(Stmt *stmt)
{
    IfStmt *s = (IfStmt *)stmt;
    expr_free(s->cond);
    Stmt *st;
    vector_foreach(st, s->block) {
        if (!st) continue;
        stmt_free(st);
    }
    vector_destroy(s->block);
    stmt_free(s->_else);
    mm_free(s);
}

static void for_stmt_free(Stmt *stmt)
{
    ForStmt *s = (ForStmt *)stmt;

    vector_destroy(s->ids);

    expr_free(s->iterable);

    Stmt *st;
    vector_foreach(st, s->block) {
        if (!st) continue;
        stmt_free(st);
    }
    vector_destroy(s->block);

    mm_free(s);
}

static void tuple_decl_stmt_free(Stmt *stmt)
{
    TupleDeclStmt *s = (TupleDeclStmt *)stmt;
    vector_destroy(s->ids);
    expr_free(s->exp);
    vector_fini(&s->sym_ids);
    Stmt *_s;
    vector_foreach(_s, s->else_block) {
        if (!_s) continue;
        stmt_free(_s);
    }
    vector_destroy(s->else_block);
    mm_free(s);
}

static void tuple_assign_stmt_free(Stmt *stmt)
{
    TupleAssignStmt *s = (TupleAssignStmt *)stmt;
    expr_free(s->lhs);
    expr_free(s->rhs);
    mm_free(s);
}

static void if_let_stmt_free(Stmt *stmt)
{
    IfLetStmt *s = (IfLetStmt *)stmt;
    expr_free(s->cond);
    Stmt *st;
    vector_foreach(st, s->block) {
        if (!st) continue;
        stmt_free(st);
    }
    vector_destroy(s->block);
    stmt_free(s->_else);
    mm_free(s);
}

static void while_stmt_free(Stmt *stmt)
{
    WhileStmt *s = (WhileStmt *)stmt;
    expr_free(s->cond);
    Stmt *st;
    vector_foreach(st, s->block) {
        if (!st) continue;
        stmt_free(st);
    }
    vector_destroy(s->block);
    mm_free(s);
}

static void with_stmt_free(Stmt *stmt)
{
    WithStmt *s = (WithStmt *)stmt;
    expr_free(s->exp);
    Stmt *st;
    vector_foreach(st, s->block) {
        if (!st) continue;
        stmt_free(st);
    }
    vector_destroy(s->block);
    if (s->else_block) {
        vector_foreach(st, s->else_block) {
            if (!st) continue;
            stmt_free(st);
        }
        vector_destroy(s->else_block);
    }
    mm_free(s);
}

static void while_let_stmt_free(Stmt *stmt)
{
    WhileLetStmt *s = (WhileLetStmt *)stmt;
    expr_free(s->cond);
    Stmt *st;
    vector_foreach(st, s->block) {
        if (!st) continue;
        stmt_free(st);
    }
    vector_destroy(s->block);
    mm_free(s);
}

static void switch_stmt_free(Stmt *stmt)
{
    SwitchStmt *s = (SwitchStmt *)stmt;
    expr_free(s->exp);
    CaseClause *clause;
    vector_foreach(clause, s->clauses) {
        case_clause_free(clause);
    }
    vector_destroy(s->clauses);
    mm_free(s);
}

void stmt_free(Stmt *stmt)
{
    if (!stmt) return;

    static void (*free_handlers[STMT_MAX_KIND])(Stmt *) = {
        [STMT_IMPORT_KIND] = import_stmt_free,
        [STMT_LINK_KIND] = link_stmt_free,
        [STMT_VAR_KIND] = var_decl_stmt_free,
        [STMT_FUNC_KIND] = func_decl_stmt_free,
        [STMT_CLASS_KIND] = klass_decl_stmt_free,
        [STMT_TRAIT_KIND] = klass_decl_stmt_free,
        [STMT_ENUM_KIND] = enum_decl_stmt_free,
        [STMT_TYPE_ALIAS_KIND] = type_alias_decl_stmt_free,
        [STMT_RETURN_KIND] = ret_stmt_free,
        [STMT_ASSIGN_KIND] = assign_stmt_free,
        [STMT_BREAK_KIND] = break_stmt_free,
        [STMT_CONTINUE_KIND] = continue_stmt_free,
        [STMT_EXPR_KIND] = expr_stmt_free,
        [STMT_BLOCK_KIND] = block_stmt_free,
        [STMT_IF_KIND] = if_stmt_free,
        [STMT_FOR_KIND] = for_stmt_free,
        [STMT_WHILE_KIND] = while_stmt_free,
        [STMT_SWITCH_KIND] = switch_stmt_free,
        [STMT_IF_LET_KIND] = if_let_stmt_free,
        [STMT_WHILE_LET_KIND] = while_let_stmt_free,
        [STMT_WITH_KIND] = with_stmt_free,
        [STMT_TUPLE_DECL_KIND] = tuple_decl_stmt_free,
        [STMT_TUPLE_ASSIGN_KIND] = tuple_assign_stmt_free,
    };

    free_handlers[stmt->kind](stmt);
}

#ifdef __cplusplus
}
#endif

/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2023 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "ast.h"

#ifdef __cplusplus
extern "C" {
#endif

Expr *expr_from_lit_int(char *orginal, __int128_t val, int sign, int bit_mode)
{
    LitExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_LITERAL_KIND;
    exp->which = LIT_EXPR_INT;
    exp->orginal = orginal; // atom str
    exp->bit_mode = bit_mode;
    exp->sign = sign;
    exp->ival_128 = val;
    exp->ival = 0;
    exp->ts = sign ? int64_type_spec() : uint64_type_spec();
    return (Expr *)exp;
}

Expr *expr_from_lit_float(double val)
{
    LitExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_LITERAL_KIND;
    exp->which = LIT_EXPR_FLT;
    exp->fval = val;
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
    buf->len = 0;
    return (Expr *)exp;
}

Expr *expr_from_lit_none(void)
{
    LitExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_LITERAL_KIND;
    exp->which = LIT_EXPR_NONE;
    exp->ts = optional_type_spec(NULL);
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

Expr *expr_from_is_expr(Expr *exp, Loc op_loc, TypeSpec *type)
{
    IsExpr *e = mm_alloc_obj(e);
    e->kind = EXPR_IS_KIND;
    e->exp = exp;
    e->op_loc = op_loc;
    e->type = type;
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

Expr *expr_from_array(Vector *vec)
{
    ArrayExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_ARRAY_KIND;
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

Expr *expr_from_set(Vector *vec)
{
    SetExpr *exp = mm_alloc_obj(exp);
    exp->kind = EXPR_SET_KIND;
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
    exp->stop = stop;
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

Expr *expr_from_panic(Expr *exp)
{
    PanicExpr *e = mm_alloc_obj(e);
    e->kind = EXPR_PANIC_KIND;
    e->exp = exp;
    e->ts = no_type_spec();
    return (Expr *)e;
}

void expr_free(Expr *exp) { mm_free(exp); }

Stmt *stmt_from_var_decl(Ident id, TypeSpec *ty, int ro, Expr *e)
{
    VarDeclStmt *s = mm_alloc_obj(s);
    s->kind = STMT_VAR_KIND;
    s->id = id;
    s->ro = ro;
    s->type = ty;
    s->exp = e;
    return (Stmt *)s;
}

TypeParamDecl *type_param_new(Loc loc, Ident id, Vector *bound)
{
    TypeParamDecl *tp = mm_alloc_obj(tp);
    tp->loc = loc;
    tp->id = id;
    tp->bound = bound;
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

Stmt *stmt_from_expr(Expr *exp)
{
    ExprStmt *s = mm_alloc_obj(s);
    s->kind = STMT_EXPR_KIND;
    s->exp = exp;
    return (Stmt *)s;
}

Stmt *stmt_from_type(StmtKind kind, Ident id, Vector *tps, Vector *bases, Vector *stmts)
{
    KlassDeclStmt *s = mm_alloc_obj(s);
    s->kind = kind;
    s->id = id;
    s->tps = tps;
    s->bases = bases;
    s->stmts = stmts;
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

static void var_decl_stmt_free(Stmt *stmt)
{
    VarDeclStmt *s = (VarDeclStmt *)stmt;
    expr_free(s->exp);
    mm_free(s);
}

static void func_decl_stmt_free(Stmt *stmt)
{
    FuncDeclStmt *s = (FuncDeclStmt *)stmt;

    Vector *tps = s->tps;
    TypeParamDecl *tp;
    vector_foreach(tp, tps) {
        if (!tp) continue;
        vector_destroy(tp->bound);
    }
    vector_destroy(tps);

    Vector *args = s->args;
    ParamDecl *p;
    vector_foreach(p, args) {
        if (!p) continue;
        expr_free(p->value);
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

static void klass_decl_stmt_free(Stmt *stmt)
{
    KlassDeclStmt *s = (KlassDeclStmt *)stmt;

    Vector *tps = s->tps;
    TypeParamDecl *tp;
    vector_foreach(tp, tps) {
        if (!tp) continue;
        vector_destroy(tp->bound);
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

void stmt_free(Stmt *stmt)
{
    if (!stmt) return;

    static void (*free_handlers[STMT_MAX_KIND])(Stmt *) = {
        [STMT_VAR_KIND] = var_decl_stmt_free,
        [STMT_FUNC_KIND] = func_decl_stmt_free,
        [STMT_CLASS_KIND] = klass_decl_stmt_free,
        [STMT_TRAIT_KIND] = klass_decl_stmt_free,
        [STMT_RETURN_KIND] = ret_stmt_free,
        [STMT_ASSIGN_KIND] = assign_stmt_free,
        [STMT_BREAK_KIND] = break_stmt_free,
        [STMT_CONTINUE_KIND] = continue_stmt_free,
        [STMT_EXPR_KIND] = expr_stmt_free,
        [STMT_BLOCK_KIND] = block_stmt_free,
        [STMT_IF_KIND] = if_stmt_free,
        [STMT_IF_LET_KIND] = if_let_stmt_free,
        [STMT_WHILE_KIND] = while_stmt_free,
        [STMT_WHILE_LET_KIND] = while_let_stmt_free,
    };

    free_handlers[stmt->kind](stmt);
}

#ifdef __cplusplus
}
#endif

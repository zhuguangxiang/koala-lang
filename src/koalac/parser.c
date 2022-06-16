/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2018-2022 James <zhuguangxiang@gmail.com>
 */

#include "parser.h"

#ifdef __cplusplus
extern "C" {
#endif

void init_parser(void)
{
}

void fini_parser(void)
{
}

void ident_has_param_type(ParserState *ps, char *name)
{
    if (!strcmp(name, "Person") || !strcmp(name, "Comparable")) {
        ps->in_angle = 1;
    } else {
        ps->in_angle = 0;
    }
}

void parser_error_detail(ParserState *ps, int row, int col)
{
    FILE *in = ps->in;
    if (!in) {
        in = fopen(ps->filename, "r");
        ps->in = in;
    }
    fseek(in, ps->offset, SEEK_SET);

    char buf[1024];
    fread(buf, sizeof(buf), in);

    printf("%5d | %s\n", row, buf);
    if (col - 1 == 0) {
        printf("%5c | " RED_COLOR("^") "\n", ' ');
    } else {
        printf("%5c | %*c" RED_COLOR("^") "\n", ' ', col - 1, ' ');
    }
}

static ParserScope *new_scope(ScopeKind kind, int block)
{
    ParserScope *scope = mm_alloc_obj(scope);
    scope->kind = kind;
    scope->blocktype = block;
    scope->stbl = stbl_new();
    return scope;
}

static void free_scope(ParserScope *scope)
{
    mm_free(scope);
}

#if !defined(NLOG)
static const char *scopes[] = {
    "NULL", "TOP", "TYPE", "FUNC", "BLOCK", "ANONY",
};

static const char *blocks[] = {
    "NULL",      "BLOCK",       "IF-BLOCK",   "WHILE-BLOCK",
    "FOR-BLOCK", "MATCH-BLOCK", "MATCH-CASE", "MATCH-CLAUSE",
};
#endif

void parser_enter_scope(ParserState *ps, ScopeKind kind, int block)
{
    ParserScope *scope = new_scope(kind, block);

    /* push old scope into stack */
    if (ps->scope) vector_push_back(&ps->scope_stack, &ps->scope);
    ps->scope = scope;
    ps->depth++;

    /* log_debug show */
#if !defined(NLOG)
    const char *scopestr;
    if (kind != SCOPE_BLOCK)
        scopestr = scopes[kind];
    else
        scopestr = blocks[block];
    log_debug("Enter scope-%d(%s)", ps->depth, scopestr);
#endif
}

void parser_exit_scope(ParserState *ps)
{
    /* log_debug show */
#if !defined(NLOG)
    const char *scopestr;
    if (ps->scope->kind != SCOPE_BLOCK)
        scopestr = scopes[ps->scope->kind];
    else
        scopestr = blocks[ps->scope->blocktype];
    log_debug("Exit scope-%d(%s)", ps->depth, scopestr);
#endif

    free_scope(ps->scope);
    ps->scope = null;
    ps->depth--;

    /* restore ps->scope to top of ps->scope_stack */
    vector_pop_back(&ps->scope_stack, &ps->scope);
}

static Symbol *find_ident_symbol(ParserState *ps, IdExpr *exp)
{
    Ident *id = &exp->id;
    if (id->sym) return id->sym;

    ParserScope *scope = ps->scope;

    /* find id symbol from current scope */
    Symbol *sym = stbl_get(scope->stbl, id->str);
    if (sym) {
        log_debug("find symbol '%s' in scope-%d(%s: %s)", id->str, ps->depth,
                  scopes[scope->kind], scope->sym->name);
        id->sym = sym;
        exp->type = expr_type_ref(sym->type);
        exp->where = CURRENT_SCOPE;
        exp->scope = scope;
        return sym;
    }

    /* find ident from up scope */
    int depth = ps->depth;
    ParserScope **up;
    vector_foreach_reverse(up, &ps->scope_stack, {
        depth--;
        sym = stbl_get((*up)->stbl, id->str);
        if (sym) {
            log_debug("find symbol '%s' in up scope-%d(%s) %s", id->str, depth,
                      scopes[(*up)->kind], (*up)->sym->name);
            id->sym = sym;
            exp->type = expr_type_ref(sym->type);
            exp->where = UP_SCOPE;
            exp->scope = up;
            return sym;
        }
    });

    return null;
}

static void parser_visit_expr(ParserState *ps, Expr *exp);

static void parse_nil(ParserState *ps, Expr *exp)
{
    log_debug("parse nil");
    exp->eop = EXPR_OP_RO;
}

static void parse_self(ParserState *ps, Expr *exp)
{
}

static void parse_super(ParserState *ps, Expr *exp)
{
}

static void parse_int(ParserState *ps, Expr *exp)
{
    ConstExpr *kexp = (ConstExpr *)exp;
    log_debug("parse integer '%ld'", kexp->ival);
    exp->eop = EXPR_OP_RO;
}

static void parse_float(ParserState *ps, Expr *exp)
{
    ConstExpr *kexp = (ConstExpr *)exp;
    log_debug("parse float '%lf'", kexp->fval);
    exp->eop = EXPR_OP_RO;
}

static void parse_bool(ParserState *ps, Expr *exp)
{
    ConstExpr *kexp = (ConstExpr *)exp;
    log_debug("parse bool '%s'", kexp->bval ? "true" : "false");
    exp->eop = EXPR_OP_RO;
}

static void parse_char(ParserState *ps, Expr *exp)
{
    ConstExpr *kexp = (ConstExpr *)exp;
    char *s = (char *)&kexp->cval;
    log_debug("parse char '%s'", s);
    exp->eop = EXPR_OP_RO;
}

static void parse_str(ParserState *ps, Expr *exp)
{
    ConstExpr *kexp = (ConstExpr *)exp;
    log_debug("parse str '%s'", kexp->sval);
    exp->eop = EXPR_OP_RO;
}

static void parse_ident(ParserState *ps, Expr *exp)
{
    IdExpr *idexp = (IdExpr *)exp;
    Ident *id = &idexp->id;
    log_debug("parse ident '%s'", id->str);
    Symbol *sym = find_ident_symbol(ps, idexp);
    if (!sym) {
        yy_errmsg(id->loc, "'%s' is not found", id->str);
        return;
    }

    if (sym->kind == SYM_VAR) {
        log_debug("'%s' is variable", id->str);
        idexp->id.sym = sym;
        idexp->type = expr_type_ref(sym->type);
        idexp->eop = EXPR_OP_RW;
    } else if (sym->kind == SYM_LET) {
        log_debug("'%s' is immutable", id->str);
        idexp->id.sym = sym;
        idexp->type = expr_type_ref(sym->type);
        idexp->eop = EXPR_OP_RO;
    }
}

static void parse_under(ParserState *ps, Expr *exp)
{
}

static void parse_unary(ParserState *ps, Expr *exp)
{
    log_debug("parse unary");
    UnaryExpr *uexp = (UnaryExpr *)exp;

    // parse right expr
    Expr *rexp = uexp->exp;
    parser_visit_expr(ps, rexp);
    if (!rexp || !rexp->type) return;

    switch (uexp->uop) {
        case UNARY_PLUS:
            if (!desc_is_number(rexp->type->ty)) {
                yy_errmsg(uexp->oploc, "wrong type argument to unary plus");
                return;
            }
            break;
        case UNARY_NEG:
            if (!desc_is_number(rexp->type->ty)) {
                yy_errmsg(uexp->oploc, "wrong type argument to unary minus");
                return;
            }
            break;
        case UNARY_NOT:
            if (!desc_is_bool(rexp->type->ty)) {
                yy_errmsg(uexp->oploc, "wrong type argument to unary not");
                return;
            }
            break;
        case UNARY_BIT_NOT:
            if (!desc_is_integer(rexp->type->ty)) {
                yy_errmsg(uexp->oploc, "wrong type argument to bit-complement");
                return;
            }
            break;
        default:
            assert(0);
            break;
    }

    // set expr's type as right expr's type
    exp->type = expr_type_ref(rexp->type->ty);
}

static void show_binary_error(ParserState *ps, BinaryExpr *bexp, char *opstr)
{
    Expr *lexp = bexp->lexp;
    Expr *rexp = bexp->rexp;

    BUF(s1);
    BUF(s2);
    desc_to_str(lexp->type->ty, &s1);
    desc_to_str(rexp->type->ty, &s2);
    yy_errmsg(bexp->oploc, "invalid operands to binary %s ('%s' and '%s')",
              opstr, BUF_STR(s1), BUF_STR(s2));
    FINI_BUF(s1);
    FINI_BUF(s2);
}

static void parse_binary(ParserState *ps, Expr *exp)
{
    log_debug("parse binary");
    BinaryExpr *bexp = (BinaryExpr *)exp;

    // parse left expr
    Expr *lexp = bexp->lexp;
    parser_visit_expr(ps, lexp);
    if (!lexp || !lexp->type) return;

    // parse right expr
    Expr *rexp = bexp->rexp;
    parser_visit_expr(ps, rexp);
    if (!rexp || !rexp->type) return;

    TypeDesc *ty = null;
    switch (bexp->bop) {
        case BINARY_ADD:
            if (desc_is_str(lexp->type->ty) && !desc_is_str(rexp->type->ty)) {
                show_binary_error(ps, bexp, "+");
                return;
            }

            if (!desc_is_str(lexp->type->ty) && desc_is_str(rexp->type->ty)) {
                show_binary_error(ps, bexp, "+");
                return;
            }

            if (!desc_is_str(lexp->type->ty) && !desc_is_str(rexp->type->ty)) {
                if (!desc_is_number(lexp->type->ty) ||
                    !desc_is_number(rexp->type->ty)) {
                    show_binary_error(ps, bexp, "+");
                    return;
                }
            }
            ty = lexp->type->ty;
            break;
        case BINARY_SUB:
            if (!desc_is_number(lexp->type->ty) ||
                !desc_is_number(rexp->type->ty)) {
                show_binary_error(ps, bexp, "-");
                return;
            }
            ty = lexp->type->ty;
            break;
        case BINARY_MULT:
            if (!desc_is_number(lexp->type->ty) ||
                !desc_is_number(rexp->type->ty)) {
                show_binary_error(ps, bexp, "*");
                return;
            }
            ty = lexp->type->ty;
            break;
        case BINARY_DIV:
            if (!desc_is_number(lexp->type->ty) ||
                !desc_is_number(rexp->type->ty)) {
                show_binary_error(ps, bexp, "/");
                return;
            }
            ty = lexp->type->ty;
            break;
        case BINARY_MOD:
            if (!desc_is_number(lexp->type->ty) ||
                !desc_is_number(rexp->type->ty)) {
                show_binary_error(ps, bexp, "%%");
                return;
            }
            ty = lexp->type->ty;
            break;
        case BINARY_SHL:
            if (!desc_is_integer(lexp->type->ty) ||
                !desc_is_integer(rexp->type->ty)) {
                show_binary_error(ps, bexp, "<<");
                return;
            }
            ty = lexp->type->ty;
            break;
        case BINARY_SHR:
            if (!desc_is_integer(lexp->type->ty) ||
                !desc_is_integer(rexp->type->ty)) {
                show_binary_error(ps, bexp, ">>");
                return;
            }
            ty = lexp->type->ty;
            break;
        case BINARY_BIT_AND:
            if (!desc_is_integer(lexp->type->ty) ||
                !desc_is_integer(rexp->type->ty)) {
                show_binary_error(ps, bexp, "&");
                return;
            }
            ty = lexp->type->ty;
            break;
        case BINARY_BIT_OR:
            if (!desc_is_integer(lexp->type->ty) ||
                !desc_is_integer(rexp->type->ty)) {
                show_binary_error(ps, bexp, "|");
                return;
            }
            ty = lexp->type->ty;
            break;
        case BINARY_BIT_XOR:
            if (!desc_is_integer(lexp->type->ty) ||
                !desc_is_integer(rexp->type->ty)) {
                show_binary_error(ps, bexp, "^");
                return;
            }
            ty = lexp->type->ty;
            break;
        case BINARY_GT:
            if (!desc_is_number(lexp->type->ty) ||
                !desc_is_number(rexp->type->ty)) {
                show_binary_error(ps, bexp, ">");
                return;
            }
            ty = desc_from_bool();
            break;
        case BINARY_GE:
            if (!desc_is_number(lexp->type->ty) ||
                !desc_is_number(rexp->type->ty)) {
                show_binary_error(ps, bexp, ">=");
                return;
            }
            ty = desc_from_bool();
            break;
        case BINARY_LT:
            if (!desc_is_number(lexp->type->ty) ||
                !desc_is_number(rexp->type->ty)) {
                show_binary_error(ps, bexp, "<");
                return;
            }
            ty = desc_from_bool();
            break;
        case BINARY_LE:
            if (!desc_is_number(lexp->type->ty) ||
                !desc_is_number(rexp->type->ty)) {
                show_binary_error(ps, bexp, "<");
                return;
            }
            ty = desc_from_bool();
            break;
        case BINARY_EQ:
            if (!desc_is_number(lexp->type->ty) ||
                !desc_is_number(rexp->type->ty)) {
                show_binary_error(ps, bexp, "==");
                return;
            }
            ty = desc_from_bool();
            break;
        case BINARY_NE:
            if (!desc_is_number(lexp->type->ty) ||
                !desc_is_number(rexp->type->ty)) {
                show_binary_error(ps, bexp, "!=");
                return;
            }
            ty = desc_from_bool();
            break;
        case BINARY_AND:
            if (!desc_is_bool(lexp->type->ty) ||
                !desc_is_bool(rexp->type->ty)) {
                show_binary_error(ps, bexp, "and");
                return;
            }
            ty = desc_from_bool();
            break;
        case BINARY_OR:
            if (!desc_is_bool(lexp->type->ty) ||
                !desc_is_bool(rexp->type->ty)) {
                show_binary_error(ps, bexp, "or");
                return;
            }
            ty = desc_from_bool();
            break;
        default:
            assert(0);
            break;
    }

    if (!desc_equal(lexp->type->ty, rexp->type->ty)) {
        BUF(s1);
        BUF(s2);
        desc_to_str(lexp->type->ty, &s1);
        desc_to_str(rexp->type->ty, &s2);
        yy_errmsg(bexp->oploc, "invalid operands to binary ('%s' and '%s')",
                  BUF_STR(s1), BUF_STR(s2));
        FINI_BUF(s1);
        FINI_BUF(s2);
        return;
    }

    // FIXME: check number's width?

    // set expr's type as left expr's type
    exp->type = expr_type_ref(ty);
}

static void parse_attr(ParserState *ps, Expr *exp)
{
}

static void parse_subscr(ParserState *ps, Expr *exp)
{
}

static void parse_call(ParserState *ps, Expr *exp)
{
}

static void parse_slice(ParserState *ps, Expr *exp)
{
}

static void parse_dottuple(ParserState *ps, Expr *exp)
{
}

static void parse_tuple(ParserState *ps, Expr *exp)
{
}

static void parse_array(ParserState *ps, Expr *exp)
{
}

static void parse_map(ParserState *ps, Expr *exp)
{
}

static void parse_anony(ParserState *ps, Expr *exp)
{
}

static void parse_is(ParserState *ps, Expr *exp)
{
    log_debug("parse is");
}

static void parse_as(ParserState *ps, Expr *exp)
{
}

static void parse_range(ParserState *ps, Expr *exp)
{
}

static void parse_expr_type(ParserState *ps, Expr *exp)
{
}

static void parser_visit_expr(ParserState *ps, Expr *exp)
{
    if (!exp) return;

    /* if errors is greater than MAX_ERRORS, stop parsing */
    if (ps->errors >= MAX_ERRORS) return;

    /* clang-format off */
    static void (*handlers[])(ParserState *, Expr *) = {
        null,                   /* INVALID              */
        parse_nil,              /* NIL_KIND             */
        parse_self,             /* SELF_KIND            */
        parse_super,            /* SUPER_KIND           */
        parse_int,              /* INT_KIND             */
        parse_float,            /* FLOAT_KIND           */
        parse_bool,             /* BOOL_KIND            */
        parse_char,             /* CHAR_KIND            */
        parse_str,              /* STR_KIND             */
        parse_ident,            /* ID_KIND              */
        parse_under,            /* UNDER_KIND           */
        parse_unary,            /* UNARY_KIND           */
        parse_binary,           /* BINARY_KIND          */
        parse_attr,             /* ATTRIBUTE_KIND       */
        parse_subscr,           /* SUBSCRIPT_KIND       */
        parse_call,             /* CALL_KIND            */
        parse_slice,            /* SLICE_KIND           */
        parse_dottuple,         /* DOT_TUPLE_KIND       */
        parse_tuple,            /* TUPLE_KIND           */
        parse_array,            /* ARRAY_KIND           */
        parse_map,              /* MAP_KIND             */
        parse_anony,            /* ANONY_KIND           */
        parse_is,               /* IS_KIND              */
        parse_as,               /* AS_KIND              */
        parse_range,            /* RANGE_KIND           */
        parse_expr_type,        /* TYPE_KIND            */
    };
    /* clang-format on */

    handlers[exp->kind](ps, exp);
}

static void parse_import(ParserState *ps, Stmt *stmt)
{
}

static void parse_initial_expr(ParserState *ps, Ident *id, Expr *exp)
{
    Symbol *sym = id->sym;

    parser_visit_expr(ps, exp);
    if (!exp->type) {
        free_symbol(sym);
        id->sym = null;
        return;
    }

    if (sym->type) {
        // check types are compatiable
        if (!desc_equal(sym->type, exp->type->ty)) {
            BUF(s1);
            BUF(s2);
            desc_to_str(sym->type, &s1);
            desc_to_str(exp->type->ty, &s2);
            yy_errmsg(exp->loc,
                      "incompatible types when initializing type '%s' using "
                      "type '%s'",
                      BUF_STR(s1), BUF_STR(s2));
            FINI_BUF(s1);
            FINI_BUF(s2);
            // remove symbol
            free_symbol(sym);
            id->sym = null;
        }
    } else {
        // update var's symbol type
        // IdExpr's type is useless and no need update it.
        sym->type = exp->type->ty;
    }
}

static void parse_letdecl_global(ParserState *ps, VarDeclStmt *s)
{
    Ident *id = &s->id;

    // add var failed in parser_new_var
    if (!id->sym) return;

    parse_initial_expr(ps, id, s->exp);
}

static void parse_letdecl_local(ParserState *ps, VarDeclStmt *s)
{
    log_debug("local let");

    ParserScope *scope = ps->scope;
    HashMap *stbl = scope->stbl;

    Ident *id = &s->id;
    TypeDesc *type = s->type ? s->type->ty : null;

    Symbol *sym = stbl_add_let(stbl, id->str, type);
    if (!sym) {
        yy_errmsg(id->loc, "redefinition of '%s'", id->str);
    } else {
        id->sym = sym;
    }

    parse_initial_expr(ps, id, s->exp);
}

static void parse_letdecl_field(ParserState *ps, VarDeclStmt *s)
{
}

static void parse_letdecl(ParserState *ps, Stmt *stmt)
{
    VarDeclStmt *s = (VarDeclStmt *)stmt;
    if (s->where == VAR_DECL_GLOBAL) {
        parse_letdecl_global(ps, s);
    } else if (s->where == VAR_DECL_LOCAL) {
        parse_letdecl_local(ps, s);
    } else {
        assert(s->where == VAR_DECL_FIELD);
        parse_letdecl_field(ps, s);
    }
}

static void parse_vardecl_global(ParserState *ps, VarDeclStmt *s)
{
    Ident *id = &s->id;

    // add var failed in parser_new_var
    if (!id->sym) return;

    // if exp is null, var's type must be given(sured by yacc)
    if (!s->exp) return;

    parse_initial_expr(ps, id, s->exp);
}

static void parse_vardecl_local(ParserState *ps, VarDeclStmt *s)
{
    log_debug("local var");

    ParserScope *scope = ps->scope;
    HashMap *stbl = scope->stbl;

    Ident *id = &s->id;
    TypeDesc *type = s->type ? s->type->ty : null;

    Symbol *sym = stbl_add_var(stbl, id->str, type);
    if (!sym) {
        yy_errmsg(id->loc, "redefinition of '%s'", id->str);
    } else {
        id->sym = sym;
    }

    parse_initial_expr(ps, id, s->exp);
}

static void parse_vardecl_field(ParserState *ps, VarDeclStmt *s)
{
}

static void parse_vardecl(ParserState *ps, Stmt *stmt)
{
    VarDeclStmt *s = (VarDeclStmt *)stmt;
    if (s->where == VAR_DECL_GLOBAL) {
        parse_vardecl_global(ps, s);
    } else if (s->where == VAR_DECL_LOCAL) {
        parse_vardecl_local(ps, s);
    } else {
        assert(s->where == VAR_DECL_FIELD);
        parse_vardecl_field(ps, s);
    }
}

static void parse_assign(ParserState *ps, Stmt *stmt)
{
    log_debug("parse assignment");
    AssignStmt *s = (AssignStmt *)stmt;

    // parse left expr
    Expr *lexp = s->lhs;
    parser_visit_expr(ps, lexp);
    if (!lexp || !lexp->type) return;

    if (lexp->eop != EXPR_OP_RW) {
        yy_errmsg(s->oploc, "left expr is not writable");
        return;
    }

    // parse right expr
    Expr *rexp = s->rhs;
    parser_visit_expr(ps, rexp);
    if (!rexp || !rexp->type) return;

    if (!desc_equal(lexp->type->ty, rexp->type->ty)) {
        BUF(s1);
        BUF(s2);
        desc_to_str(lexp->type->ty, &s1);
        desc_to_str(rexp->type->ty, &s2);
        yy_errmsg(rexp->loc,
                  "incompatible types when assigning to type '%s' from type "
                  "type '%s'",
                  BUF_STR(s1), BUF_STR(s2));
        FINI_BUF(s1);
        FINI_BUF(s2);
    }
}

static void parse_param_type_bound(ParserState *ps, Symbol *sym, Vector *bound)
{
    log_debug("parse param type '%s'", sym->name);
    ExprType **item;
    vector_foreach(item, bound, {
#if !defined(NLOG)
        BUF(buf);
        desc_to_str((*item)->ty, &buf);
        log_debug("bound[%d]: '%s'", i, BUF_STR(buf));
        FINI_BUF(buf);
#endif
    });
    log_debug("end");
}

static void parse_param_types(ParserState *ps, Vector *param_types)
{
    ParserScope *scope = ps->scope;
    HashMap *stbl = scope->stbl;

    IdentArgs *item;
    Ident *id;
    Vector *bound;
    Symbol *sym;
    vector_foreach(item, param_types, {
        id = &item->id;
        sym = stbl_add_param_type(stbl, id->str);
        if (!sym) {
            yy_errmsg(id->loc, "redefinition of '%s'", id->str);
            continue;
        }
        bound = item->args;
        parse_param_type_bound(ps, sym, bound);
    });
}

static void parse_func_proto(ParserState *ps, Vector *args, ExprType *ret_type)
{
    ParserScope *scope = ps->scope;
    FuncSymbol *sym = (FuncSymbol *)scope->sym;

    Vector *params = null;
    if (args) {
        params = vector_create_ptr();
        IdentType *idtype;
        Ident *id;
        ExprType *type;
        vector_foreach(idtype, args, {
            id = &idtype->id;
            type = idtype->type;
            log_debug("parse argument '%s'", id->str);
            Symbol *varsym = stbl_add_var(sym->stbl, id->str, type->ty);
            if (!varsym) {
                yy_errmsg(id->loc, "redefinition of '%s'", id->str);
            } else {
                id->sym = varsym;
            }
            vector_push_back(params, &type->ty);
        });
    }

    TypeDesc *type = ret_type ? ret_type->ty : null;
    sym->type = desc_from_proto(type, params);

#if !defined(NLOG)
    BUF(buf);
    desc_to_str(sym->type, &buf);
    log_debug("type of func '%s':", sym->name);
    log_debug("%s", BUF_STR(buf));
    FINI_BUF(buf);
#endif
}

static void parse_funcdecl(ParserState *ps, Stmt *stmt)
{
    FuncDeclStmt *func = (FuncDeclStmt *)stmt;
    Ident *id = &func->id;
    FuncSymbol *sym = (FuncSymbol *)id->sym;
    // add func failed in parser_new_func
    if (!sym) return;

    parser_enter_scope(ps, SCOPE_FUNC, 0);
    ParserScope *scope = ps->scope;
    scope->sym = (Symbol *)sym;
    scope->stbl = sym->stbl;

    // parse param_types
    parse_param_types(ps, func->param_types);

    // parse func proto
    parse_func_proto(ps, func->args, func->ret);

    // parse func body
    int topret = 0;
    Stmt **item;
    vector_foreach(item, func->body, {
        if ((*item)->kind == STMT_RETURN_KIND) topret = 1;
        parse_stmt(ps, *item);
    });

    // top branches need return value
    ProtoType *proto = (ProtoType *)sym->type;
    if (proto && proto->ret && !topret) {
        yy_errmsg(func->loc, "control reaches end of non-return function");
    }

    parser_exit_scope(ps);
}

static ParserScope *find_func_anony_scope(ParserState *ps)
{
    ParserScope *scope = ps->scope;
    ScopeKind kind = scope->kind;
    if (kind == SCOPE_FUNC || kind == SCOPE_ANONY) return scope;

    ParserScope **item;
    vector_foreach_reverse(item, &ps->scope_stack, {
        kind = (*item)->kind;
        if (kind == SCOPE_FUNC || kind == SCOPE_ANONY) return scope;
    });

    return null;
}

static void parse_return(ParserState *ps, Stmt *stmt)
{
    log_debug("parse return");

    ReturnStmt *ret = (ReturnStmt *)stmt;
    if (ret->exp) {
        parser_visit_expr(ps, ret->exp);
        if (!ret->exp->type) return;
    }

    ParserScope *up = find_func_anony_scope(ps);
    if (!up) {
        yy_errmsg(stmt->loc, "'return' outside of function or anonymous");
        return;
    }

    Symbol *func = up->sym;
    assert(func->kind == SYM_FUNC || func->kind == SYM_ANONY);
    ProtoType *proto = (ProtoType *)func->type;

    if (!ret->exp) {
        if (proto->ret) {
            yy_errmsg(stmt->loc,
                      "'return' with no value, but function returning value");
        }
        return;
    }

    if (!proto->ret) {
        yy_errmsg(stmt->loc,
                  "'return' with a value, but function returning no value");
        return;
    }

    if (!desc_equal(proto->ret, ret->exp->type->ty)) {
        BUF(s1);
        BUF(s2);
        desc_to_str(ret->exp->type->ty, &s1);
        desc_to_str(proto->ret, &s2);
        yy_errmsg(ret->exp->loc,
                  "incompatible types when returning to type '%s' but type "
                  "type '%s' expected",
                  BUF_STR(s1), BUF_STR(s2));
        FINI_BUF(s1);
        FINI_BUF(s2);
    }
}

static void parse_expr(ParserState *ps, Stmt *stmt)
{
    ExprStmt *s = (ExprStmt *)stmt;
    Expr *exp = s->exp;
    parser_visit_expr(ps, exp);
}

static void parse_block(ParserState *ps, Stmt *stmt)
{
}

static void parse_ifunc(ParserState *ps, Stmt *stmt)
{
}

static void parse_class(ParserState *ps, Stmt *stmt)
{
}

static void parse_trait(ParserState *ps, Stmt *stmt)
{
}

static void parse_enum(ParserState *ps, Stmt *stmt)
{
}

static void parse_break(ParserState *ps, Stmt *stmt)
{
}

static void parse_continue(ParserState *ps, Stmt *stmt)
{
}

static void parse_if(ParserState *ps, Stmt *stmt)
{
}

static void parse_while(ParserState *ps, Stmt *stmt)
{
}

static void parse_for(ParserState *ps, Stmt *stmt)
{
}

static void parse_match(ParserState *ps, Stmt *stmt)
{
}

void parse_stmt(ParserState *ps, Stmt *stmt)
{
    if (!stmt) return;

    /* if errors is greater than MAX_ERRORS, stop parsing */
    if (ps->errors >= MAX_ERRORS) return;

    /* clang-format off */
    static void (*handlers[])(ParserState *, Stmt *) = {
        null,               /* INVALID          */
        parse_import,       /* IMPORT_KIND      */
        parse_letdecl,      /* LET_KIND         */
        parse_vardecl,      /* VAR_KIND         */
        parse_assign,       /* ASSIGN_KIND      */
        parse_funcdecl,     /* FUNC_KIND        */
        parse_return,       /* RETURN_KIND      */
        parse_expr,         /* EXPR_KIND        */
        parse_block,        /* BLOCK_KIND       */
        parse_ifunc,        /* IFUNC_KIND       */
        parse_class,        /* CLASS_KIND       */
        parse_trait,        /* TRAIT_KIND       */
        parse_enum,         /* ENUM_KIND        */
        parse_break,        /* BREAK_KIND       */
        parse_continue,     /* CONTINUE_KIND    */
        parse_if,           /* IF_KIND          */
        parse_while,        /* WHILE_KIND       */
        parse_for,          /* FOR_KIND         */
        parse_match,        /* MATCH_KIND       */
    };
    /* clang-format on */

    handlers[stmt->kind](ps, stmt);
}

void parser_eval_stmts(ParserState *ps)
{
    Stmt **item;
    vector_foreach(item, &ps->stmts, { parse_stmt(ps, *item); });
}

void parser_free_stmts(ParserState *ps)
{
    Stmt **item;
    vector_foreach(item, &ps->stmts, { free_stmt(*item); });
    vector_clear(&ps->stmts);
}

void parser_append_stmt(ParserState *ps, Stmt *stmt)
{
    if (!stmt) return;
    vector_push_back(&ps->stmts, &stmt);
}

void parser_new_var(ParserState *ps, Stmt *stmt)
{
    if (!stmt) return;
    ParserGroup *grp = ps->grp;
    VarDeclStmt *vardecl = (VarDeclStmt *)stmt;
    Ident *id = &vardecl->id;
    TypeDesc *type = vardecl->type ? vardecl->type->ty : null;

    Symbol *sym = null;
    if (stmt->kind == STMT_LET_KIND) {
        sym = stbl_add_let(grp->stbl, id->str, type);
    } else {
        assert(stmt->kind == STMT_VAR_KIND);
        sym = stbl_add_var(grp->stbl, id->str, type);
    }

    if (!sym) {
        yy_errmsg(id->loc, "redefinition of '%s'", id->str);
    } else {
        id->sym = sym;
    }
}

void parser_new_func(ParserState *ps, Stmt *stmt)
{
    if (!stmt) return;
    ParserGroup *grp = ps->grp;
    FuncDeclStmt *funcdecl = (FuncDeclStmt *)stmt;
    Ident *id = &funcdecl->id;
    // func's type and args are handled in parse_funcdecl.
    // some classes/traits may be defined after used.
    Symbol *sym = stbl_add_func(grp->stbl, id->str);
    if (!sym) {
        yy_errmsg(id->loc, "redefinition of '%s'", id->str);
    } else {
        id->sym = sym;
    }
}

#ifdef __cplusplus
}
#endif

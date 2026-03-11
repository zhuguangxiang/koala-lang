/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "parser.h"
#include "passes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BUILDER(ps) \
    ParserScope *sc = ps->scope; \
    KlrBuilder bldr; \
    klr_builder_end(&bldr, sc->bb);

static void emit_ir_visit_expr(ParserState *ps, Expr *exp);

static void emit_ir_ident(ParserState *ps, Expr *exp)
{
    IdentExpr *ident = (IdentExpr *)exp;
    Symbol *sym = ident->sym;
    if (!sym) {
        kl_error(ident->id.loc, "undefined symbol '%s'", ident->id.name);
        return;
    }

    if (!sym->ir_val) {
        ASSERT(sym->flags & SYM_FLAGS_EXT);
        KlrValue *val = NULL;
        if (sym->kind == SYM_FUNC) {
            FuncSymbol *func_sym = (FuncSymbol *)sym;
            val = klr_add_ext_func(ps->mod, func_sym->ret, sym->path, sym->name);
        } else if (sym->kind == SYM_VAR) {
            val = klr_add_ext_global(ps->mod, sym->ts, sym->path, sym->name);
        } else {
            // UNREACHABLE();
        }
        sym->ir_val = val;
    }

    BUILDER(ps);

    switch (sym->kind) {
        case SYM_VAR: {
            VarSymbol *var_sym = (VarSymbol *)sym;
            if (var_sym->scope == VAR_SCOPE_GLOBAL) {
                exp->ir_val = klr_build_get_global(&bldr, sym->ir_val);
            } else {
                exp->ir_val = sym->ir_val;
            }
            break;
        }
        case SYM_FUNC: {
            exp->ir_val = sym->ir_val;
            break;
        }
        default: {
            UNREACHABLE();
            break;
        }
    }
}

static void emit_ir_literal(ParserState *ps, Expr *exp)
{
    LitExpr *lit = (LitExpr *)exp;
    KlrModule *m = ps->mod;

    switch (lit->which) {
        case LIT_EXPR_INT: {
            exp->ir_val = klr_const_int(lit->ival, lit->ts, m);
            break;
        }
        case LIT_EXPR_FLT: {
            exp->ir_val = klr_const_float(lit->fval, lit->ts, m);
            break;
        }
        case LIT_EXPR_BOOL: {
            exp->ir_val = klr_const_bool(lit->bval, m);
            break;
        }
        case LIT_EXPR_STR: {
            exp->ir_val = klr_const_str(lit->sval, lit->len, m);
            break;
        }
        case LIT_EXPR_NONE: {
            UNREACHABLE();
            break;
        }
        default: {
            UNREACHABLE();
            break;
        }
    }
}

static void emit_ir_type(ParserState *ps, Expr *exp)
{
    Symbol *sym = exp->sym;
    if (sym->kind == SYM_CLASS) {
        KlassSymbol *kls_sym = (KlassSymbol *)sym;
        exp->ir_val = klr_add_klass(ps->mod, kls_sym->instance_ts, sym->name);
    } else if (sym->kind == SYM_INSTANCE) {
        InstanceSymbol *inst_sym = (InstanceSymbol *)sym;
        exp->ir_val = klr_add_klass(ps->mod, inst_sym->instance_ts, sym->name);
    }
}

static void emit_ir_call(ParserState *ps, Expr *exp)
{
    CallExpr *call = (CallExpr *)exp;
    Expr *lhs = call->lhs;
    Vector *args = call->args;
    int size = vector_size(args);

    // gen ir for lhs
    lhs->ctx = EXPR_CTX_LOAD;
    emit_ir_visit_expr(ps, lhs);
    if (!lhs->ir_val) return;

    KlrValue *ir_args[size];

    // gen ir for arguments
    Expr *e;
    vector_foreach(e, args) {
        e->ctx = EXPR_CTX_LOAD;
        emit_ir_visit_expr(ps, e);
        if (!e->ir_val) return;
        ir_args[i__] = e->ir_val;
    }

    // codegen

    BUILDER(ps);
    KlrValue *ret = klr_build_call(&bldr, lhs->ir_val, ir_args, size, "");
    exp->ir_val = ret;
}

static OpCode get_binary_op_code(BiOpKind op)
{
    switch (op) {
        case BINARY_ADD:
            return OP_BINARY_ADD;
        case BINARY_SUB:
            return OP_BINARY_SUB;
        case BINARY_MUL:
            return OP_BINARY_MUL;
        case BINARY_DIV:
            return OP_BINARY_DIV;
        case BINARY_MOD:
            return OP_BINARY_MOD;
        default:
            UNREACHABLE();
            return OP_NOP;
    }
}

static char *get_binary_op_name(BiOpKind op)
{
    switch (op) {
        case BINARY_ADD:
            return "add";
        case BINARY_SUB:
            return "sub";
        case BINARY_MUL:
            return "mul";
        case BINARY_DIV:
            return "div";
        case BINARY_MOD:
            return "mod";
        default:
            UNREACHABLE();
            return "";
    }
}

static void emit_ir_binary(ParserState *ps, Expr *exp)
{
    BinaryExpr *bin = (BinaryExpr *)exp;
    BiOpKind op = bin->op;
    Expr *lhs = bin->lhs;
    Expr *rhs = bin->rhs;

    lhs->ctx = EXPR_CTX_LOAD;
    emit_ir_visit_expr(ps, lhs);
    if (!lhs->ir_val) return;

    rhs->ctx = EXPR_CTX_LOAD;
    emit_ir_visit_expr(ps, rhs);
    if (!rhs->ir_val) return;

    BUILDER(ps);

    KlrValue *res = klr_build_binary(&bldr, lhs->ir_val, rhs->ir_val,
                                     get_binary_op_code(op), "", get_binary_op_name(op));
    exp->ir_val = res;
}

static void emit_ir_list(ParserState *ps, Expr *exp)
{
    ListExpr *list = (ListExpr *)exp;

    int size = vector_size(list->vec);
    KlrValue *items[size];
    int konst = 1;

    Expr *e;
    vector_foreach(e, list->vec) {
        e->ctx = EXPR_CTX_LOAD;
        emit_ir_visit_expr(ps, e);
        if (!e->ir_val) return;
        items[i__] = e->ir_val;
        if (!klr_is_const(e->ir_val)) konst = 0;
    }

    emit_ir_type(ps, exp);

    if (konst) {
        KlrValue *lit = klr_const_list(items, size, exp->ts, ps->mod);
        exp->ir_val = lit;
    } else {
        BUILDER(ps);
        KlrValue *ret = klr_build_call(&bldr, exp->ir_val, items, size, "");
        exp->ir_val = ret;
    }
}

static void emit_ir_tuple(ParserState *ps, Expr *exp)
{
    ListExpr *list = (ListExpr *)exp;

    int size = vector_size(list->vec);
    KlrValue *items[size];
    int konst = 1;

    Expr *e;
    vector_foreach(e, list->vec) {
        e->ctx = EXPR_CTX_LOAD;
        emit_ir_visit_expr(ps, e);
        if (!e->ir_val) return;
        items[i__] = e->ir_val;
        if (!klr_is_const(e->ir_val)) konst = 0;
    }

    emit_ir_type(ps, exp);

    if (konst) {
        KlrValue *lit = klr_const_tuple(items, size, exp->ts, ps->mod);
        exp->ir_val = lit;
    } else {
        BUILDER(ps);
        KlrValue *ret = klr_build_call(&bldr, exp->ir_val, items, size, "");
        exp->ir_val = ret;
    }
}

static void emit_ir_visit_expr(ParserState *ps, Expr *exp)
{
    if (!exp) return;

    /* if errors is greater than MAX_ERRORS, stop parsing */
    if (ps->errors >= MAX_ERRORS) return;

    /* clang-format off */
    static void (*handlers[])(ParserState *, Expr *) = {
        [EXPR_ID_KIND]      = emit_ir_ident,
        [EXPR_LITERAL_KIND] = emit_ir_literal,
        [EXPR_TYPE_KIND]    = emit_ir_type,
        [EXPR_CALL_KIND]    = emit_ir_call,
        [EXPR_BINARY_KIND]  = emit_ir_binary,
        [EXPR_LIST_KIND]    = emit_ir_list,
        [EXPR_TUPLE_KIND]   = emit_ir_tuple,
    };
    /* clang-format on */

    handlers[exp->kind](ps, exp);
}

static void emit_ir_var_decl(ParserState *ps, Stmt *stmt)
{
    VarDeclStmt *var = (VarDeclStmt *)stmt;
    Expr *exp = var->exp;

    if (exp) {
        exp->ctx = EXPR_CTX_LOAD;
        emit_ir_visit_expr(ps, exp);
        if (!exp->ir_val) return;
    }

    BUILDER(ps);
    VarSymbol *sym = (VarSymbol *)var->sym;
    if (sym->scope == VAR_SCOPE_GLOBAL) {
        if (exp) {
            klr_build_set_global(&bldr, sym->ir_val, exp->ir_val);
        }
    } else if (sym->scope == VAR_SCOPE_LOCAL) {
        if (sym->flags & SYM_FLAGS_MUTABLE) {
            sym->ir_val = klr_build_local_var(&bldr, sym->ts, var->id.name);
        } else {
            sym->ir_val = klr_build_local(&bldr, sym->ts, var->id.name);
        }
        if (exp) {
            klr_build_move(&bldr, sym->ir_val, exp->ir_val);
        }
    } else if (sym->scope == VAR_SCOPE_FIELD) {
        NYI();
    } else {
        UNREACHABLE();
    }
}

static void emit_ir_stmt(ParserState *ps, Stmt *stmt);

static void emit_ir_func_decl(ParserState *ps, Stmt *stmt)
{
    FuncDeclStmt *fn = (FuncDeclStmt *)stmt;
    Symbol *sym = fn->sym;
    ParserScope *scope = enter_scope(ps, SCOPE_FUNC, 0, fn->id.name);
    KlrBasicBlock *entry = klr_append_block(sym->ir_val, "entry");
    scope->bb = entry;

    Stmt *s;
    vector_foreach(s, fn->body) {
        if (!s) continue;
        emit_ir_stmt(ps, s);
    }

    exit_scope(ps);

    klr_print_func((KlrFunc *)sym->ir_val, stdout);

    klr_run_default_passes((KlrFunc *)sym->ir_val);
}

static void emit_ir_class(ParserState *ps, Stmt *stmt) {}

static void emit_ir_trait(ParserState *ps, Stmt *stmt) {}

static void emit_ir_return(ParserState *ps, Stmt *stmt)
{
    RetStmt *ret = (RetStmt *)stmt;
    Expr *exp = ret->exp;
    if (!exp) {
        BUILDER(ps);
        klr_build_ret_void(&bldr);
        return;
    }

    exp->ctx = EXPR_CTX_LOAD;
    emit_ir_visit_expr(ps, exp);
    if (!exp->ir_val) return;

    BUILDER(ps);
    klr_build_ret(&bldr, exp->ir_val);
}

static void emit_ir_expr(ParserState *ps, Stmt *stmt)
{
    ExprStmt *s = (ExprStmt *)stmt;
    Expr *exp = s->exp;
    exp->ctx = EXPR_CTX_LOAD;
    emit_ir_visit_expr(ps, exp);
}

static void emit_ir_stmt(ParserState *ps, Stmt *stmt)
{
    if (!stmt) return;

    /* if errors is greater than MAX_ERRORS, stop parsing */
    if (ps->errors >= MAX_ERRORS) return;

    /* clang-format off */
    static void (*handlers[STMT_MAX_KIND])(ParserState *, Stmt *) = {
        [STMT_VAR_KIND]     = emit_ir_var_decl,
        [STMT_FUNC_KIND]    = emit_ir_func_decl,
        [STMT_CLASS_KIND]   = emit_ir_class,
        [STMT_TRAIT_KIND]   = emit_ir_trait,
        [STMT_RETURN_KIND]  = emit_ir_return,
        [STMT_EXPR_KIND]    = emit_ir_expr,
    };
    /* clang-format on */

    handlers[stmt->kind](ps, stmt);
}

static void _add_global(KlrModule *m, VarDeclStmt *var)
{
    VarSymbol *sym = (VarSymbol *)var->sym;
    int mut = var->which == VAR_DECL_VAR ? 1 : 0;
    KlrValue *gvar = klr_add_global(m, sym->ts, var->id.name, mut);
    sym->ir_val = gvar;
}

static void _add_func(KlrModule *m, FuncDeclStmt *fn)
{
    Ident *id = &fn->id;
    FuncSymbol *sym = (FuncSymbol *)fn->sym;

    KlrValue *fval = klr_add_func(m, sym->ret, id->name);

    ArgInfo *arg;
    vector_foreach(arg, sym->params) {
        klr_func_add_param(fval, arg->ts, arg->name);
    }

    sym->ir_val = fval;
}

static void _add_klass(KlrModule *m, KlassDeclStmt *kls)
{
    Ident *id = &kls->id;
    KlassSymbol *sym = (KlassSymbol *)kls->sym;
    KlrValue *kval = klr_add_klass(m, sym->ts, id->name);

    VarSymbol *field;
    vector_foreach(field, sym->fields) {
        klr_klass_add_field(kval, field->name, field->ts);
    }

    sym->ir_val = kval;
}

void klr_let_lit_prop_pass(KlrFunc *func, void *ctx);
void klr_dce_pass(KlrFunc *fn, void *ctx);

void ast_emit_ir(ParserState *ps)
{
    KlrModule *m = klr_create_module(ps->filename);
    ps->mod = m;

    // visit all global variables and add them to ir module
    Stmt *s;
    vector_foreach(s, &ps->stmts) {
        if (!s) continue;
        if (s->kind == STMT_VAR_KIND) {
            VarDeclStmt *var = (VarDeclStmt *)s;
            _add_global(m, var);
        } else if (s->kind == STMT_FUNC_KIND) {
            FuncDeclStmt *fn = (FuncDeclStmt *)s;
            _add_func(m, fn);
        } else if (s->kind == STMT_CLASS_KIND || s->kind == STMT_TRAIT_KIND) {
            KlassDeclStmt *kls = (KlassDeclStmt *)s;
            _add_klass(m, kls);
        } else {
            // do nothing
        }
    }

    KlrValue *fn = klr_add_func(m, no_type_spec(), "__init__");
    m->init = (KlrFunc *)fn;

    ParserScope *scope = enter_scope(ps, SCOPE_TOP, 0, "top");
    KlrBasicBlock *entry = klr_append_block(fn, "entry");
    scope->bb = entry;

    // emit ir for all statements
    vector_foreach(s, &ps->stmts) {
        if (!s) continue;
        emit_ir_stmt(ps, s);
    }

    // klr_print_module(m, stdout);

    klr_run_default_passes((KlrFunc *)fn);

    klr_print_module(m, stdout);

    exit_scope(ps);
}

#ifdef __cplusplus
}
#endif

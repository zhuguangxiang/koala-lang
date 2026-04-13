/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "cmd.h"
#include "parser.h"
#include "pass.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MOD ps->module->module

#define CURRENT_FUNC ((KlrValue *)ps->scope->bb->func)

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
            val = klr_add_ext_func(MOD, func_sym->ret, sym->path, sym->name);
        } else if (sym->kind == SYM_VAR) {
            val = klr_add_ext_global(MOD, sym->ts, sym->path, sym->name);
        } else {
            // UNREACHABLE();
        }
        sym->ir_val = val;
    }

    KlrBuilder bldr;
    klr_builder_end(&bldr, ps->scope->bb);

    switch (sym->kind) {
        case SYM_VAR: {
            VarSymbol *var_sym = (VarSymbol *)sym;
            if (var_sym->scope == VAR_SCOPE_GLOBAL) {
                if (exp->ctx == EXPR_CTX_LOAD) {
                    exp->ir_val = klr_build_get_global(&bldr, sym->ir_val);
                } else {
                    ASSERT(exp->ctx == EXPR_CTX_STORE);
                    exp->ir_val = sym->ir_val;
                }
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
    KlrModule *m = MOD;

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
            exp->ir_val = klr_const_none(m);
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
        exp->ir_val = klr_add_klass(MOD, kls_sym->instance_ts, sym->name);
    } else if (sym->kind == SYM_INSTANCE) {
        InstanceSymbol *inst_sym = (InstanceSymbol *)sym;
        exp->ir_val = klr_add_klass(MOD, inst_sym->instance_ts, sym->name);
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

    KlrBuilder bldr;
    klr_builder_end(&bldr, ps->scope->bb);
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
        case BINARY_SHL:
            return OP_BINARY_SHL;
        case BINARY_SHR:
            return OP_BINARY_SHR;
        case BINARY_BIT_AND:
            return OP_BINARY_AND;
        case BINARY_BIT_OR:
            return OP_BINARY_OR;
        case BINARY_BIT_XOR:
            return OP_BINARY_XOR;
        case BINARY_GT:
            return OP_BINARY_CMPGT;
        case BINARY_GE:
            return OP_BINARY_CMPGE;
        case BINARY_LT:
            return OP_BINARY_CMPLT;
        case BINARY_LE:
            return OP_BINARY_CMPLE;
        case BINARY_EQ:
            return OP_BINARY_CMPEQ;
        case BINARY_NEQ:
            return OP_BINARY_CMPNE;
        case BINARY_AND:
            return OP_LAND;
        case BINARY_OR:
            return OP_LOR;
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
        case BINARY_GT:
            return "gt";
        case BINARY_GE:
            return "ge";
        case BINARY_LT:
            return "lt";
        case BINARY_LE:
            return "le";
        case BINARY_EQ:
            return "eq";
        case BINARY_NEQ:
            return "neq";
        case BINARY_AND:
            return "and";
        case BINARY_OR:
            return "or";
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

    KlrBuilder bldr;
    klr_builder_end(&bldr, ps->scope->bb);

    if (op >= BINARY_GT && op <= BINARY_NEQ) {
        KlrValue *res =
            klr_build_cmp(&bldr, lhs->ir_val, rhs->ir_val, get_binary_op_code(op), "");
        exp->ir_val = res;
    } else {
        KlrValue *res =
            klr_build_binary(&bldr, lhs->ir_val, rhs->ir_val, get_binary_op_code(op), "",
                             get_binary_op_name(op));
        exp->ir_val = res;
    }
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
        KlrValue *lit = klr_const_list(items, size, exp->ts, MOD);
        exp->ir_val = lit;
    } else {
        KlrBuilder bldr;
        klr_builder_end(&bldr, ps->scope->bb);
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
        KlrValue *lit = klr_const_tuple(items, size, exp->ts, MOD);
        exp->ir_val = lit;
    } else {
        KlrBuilder bldr;
        klr_builder_end(&bldr, ps->scope->bb);
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

    KlrBuilder bldr;
    klr_builder_end(&bldr, ps->scope->bb);
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

    KlrBasicBlock *last = scope->bb;
    klr_add_last_return(last);

    if (dump_no_opt_ir_enabled()) {
        fprintf(stdout, "--- IR Dump After ir-gen(no-opt) ---\n");
        klr_print_func((KlrFunc *)sym->ir_val, stdout);
    }

    exit_scope(ps);
}

static void emit_ir_class(ParserState *ps, Stmt *stmt) {}

static void emit_ir_trait(ParserState *ps, Stmt *stmt) {}

static void emit_ir_return(ParserState *ps, Stmt *stmt)
{
    RetStmt *ret = (RetStmt *)stmt;
    Expr *exp = ret->exp;
    if (!exp) {
        KlrBuilder bldr;
        klr_builder_end(&bldr, ps->scope->bb);
        klr_build_ret_void(&bldr);
        return;
    }

    exp->ctx = EXPR_CTX_LOAD;
    emit_ir_visit_expr(ps, exp);
    if (!exp->ir_val) return;

    KlrBuilder bldr;
    klr_builder_end(&bldr, ps->scope->bb);
    klr_build_ret(&bldr, exp->ir_val);

    // add a dead block after return to avoid generating code after return
    // ps->scope->bb = klr_append_block(CURRENT_FUNC, "dead.code");

    // The front-end will guarantee that there is no code after return
    // statement, so we don't need to append a new block here, just set current
    // block to NULL to avoid generating ir for unreachable code If the
    // front-end allows code after return statement in the future, we can
    // uncomment the above line to append a new block for unreachable code
    // ps->scope->bb = NULL;
}

static void emit_ir_expr(ParserState *ps, Stmt *stmt)
{
    ExprStmt *s = (ExprStmt *)stmt;
    Expr *exp = s->exp;
    exp->ctx = EXPR_CTX_LOAD;
    emit_ir_visit_expr(ps, exp);
}

static void emit_ir_visit_block(ParserState *ps, Vector *block)
{
    Stmt *s;
    vector_foreach(s, block) {
        if (!s) continue;
        emit_ir_stmt(ps, s);
    }
}

static void emit_ir_if_stmt(ParserState *ps, Stmt *stmt)
{
    KlrValue *fn = CURRENT_FUNC;

    IfStmt *s = (IfStmt *)stmt;
    Expr *cond = s->cond;

    cond->ctx = EXPR_CTX_LOAD;
    emit_ir_visit_expr(ps, cond);
    if (!cond->ir_val) return;

    KlrBasicBlock *if_then = klr_append_block(fn, "if-then");
    KlrBasicBlock *if_else = klr_append_block(fn, "if-else");
    KlrBasicBlock *if_end = klr_append_block(fn, "if-end");

    KlrBuilder bldr;
    klr_builder_end(&bldr, ps->scope->bb);
    klr_build_jmp_cond(&bldr, cond->ir_val, if_then, if_else);

    ParserScope *sc = enter_scope(ps, SCOPE_BLOCK, IF_BLOCK, "if-block");
    sc->bb = if_then;
    emit_ir_visit_block(ps, s->block);

    if (!block_has_terminator(sc->bb)) {
        KlrBuilder _bldr;
        klr_builder_end(&_bldr, sc->bb);
        klr_build_jmp(&_bldr, if_end);
    }

    exit_scope(ps);

    if (s->_else) {
        ParserScope *_sc = enter_scope(ps, SCOPE_BLOCK, ELSE_BLOCK, "else-block");
        _sc->bb = if_else;

        if (s->_else->kind == STMT_BLOCK_KIND) {
            emit_ir_visit_block(ps, ((BlockStmt *)s->_else)->stmts);
        } else {
            emit_ir_if_stmt(ps, s->_else);
        }

        if (!block_has_terminator(_sc->bb)) {
            KlrBuilder _bldr;
            klr_builder_end(&_bldr, _sc->bb);
            klr_build_jmp(&_bldr, if_end);
        }

        exit_scope(ps);
    } else {
        KlrBuilder _bldr;
        klr_builder_end(&_bldr, if_else);
        klr_build_jmp(&_bldr, if_end);
    }

    ps->scope->bb = if_end;
}

static void emit_ir_while_stmt(ParserState *ps, Stmt *stmt)
{
    KlrValue *fn = CURRENT_FUNC;

    WhileStmt *s = (WhileStmt *)stmt;
    Expr *cond = s->cond;

    KlrBasicBlock *while_cond = klr_append_block(fn, "while-cond");
    KlrBasicBlock *while_body = klr_append_block(fn, "while-body");
    KlrBasicBlock *while_end = klr_append_block(fn, "while-end");

    // 1. current block jmp to cond block
    KlrBuilder bldr;
    klr_builder_end(&bldr, ps->scope->bb);
    klr_build_jmp(&bldr, while_cond);

    ParserScope *sc = enter_scope(ps, SCOPE_BLOCK, ONLY_BLOCK, "while-cond");
    sc->bb = while_cond;

    KlrValue *cond_val = NULL;
    if (cond != NULL) {
        cond->ctx = EXPR_CTX_LOAD;
        emit_ir_visit_expr(ps, cond);
        if (!cond->ir_val) return;
        cond_val = cond->ir_val;
    } else {
        // while true
        cond_val = klr_const_bool(1, MOD);
    }

    // 2. build conditonal jmp
    KlrBuilder cond_bldr;
    klr_builder_end(&cond_bldr, while_cond);
    klr_build_jmp_cond(&cond_bldr, cond_val, while_body, while_end);

    exit_scope(ps);

    sc = enter_scope(ps, SCOPE_BLOCK, WHILE_BLOCK, "while-block");
    sc->bb = while_body;

    // save continue_bb and break_bb for `break` and `continue`
    sc->continue_bb = while_cond;
    sc->break_bb = while_end;

    emit_ir_visit_block(ps, s->block);

    // add jmp to cond block
    if (!block_has_terminator(sc->bb)) {
        KlrBuilder _bldr;
        klr_builder_end(&_bldr, sc->bb);
        klr_build_jmp(&_bldr, while_cond);
    }

    exit_scope(ps);

    ps->scope->bb = while_end;
}

struct RangeInfo {
    KlrValue *start;
    KlrValue *end;
    KlrValue *step;
};

static int is_new_range(KlrInsn *insn, struct RangeInfo *out, ParserState *ps)
{
    if (insn->code != OP_IR_CALL) return 0;
    KlrValue *val = insn_oper_value(insn, 0);
    if (val->kind != KLR_VALUE_KLASS) return 0;
    if (!str_eq(val->name, "range")) return 0;
    int num_opers = insn->num_opers;
    ASSERT(num_opers >= 3 && num_opers <= 4);
    out->start = insn_oper_value(insn, 1);
    out->end = insn_oper_value(insn, 2);
    KlrValue *one = klr_const_int(1, int64_type_spec(), MOD);
    out->step = (num_opers == 4 ? insn_oper_value(insn, 3) : one);

    return 1;
}

static Symbol *get_loop_range_symbol(ForStmt *s)
{
    ASSERT(vector_size(&s->sym_ids) == 1);
    int *sym_id = vector_get_ptr(&s->sym_ids, 0);
    Symbol *sym = get_symbol_by_id(*sym_id);
    ASSERT(sym->kind == SYM_VAR);
    VarSymbol *var_sym = (VarSymbol *)sym;
    ASSERT(var_sym->scope == VAR_SCOPE_LOCAL);
    return sym;
}

static void build_loop_range_cond(KlrBuilder *bldr, KlrValue *range_cur,
                                  struct RangeInfo *range_info, KlrBasicBlock *true_bb,
                                  KlrBasicBlock *false_bb, ParserState *ps)
{
    KlrValue *zero = klr_const_int(0, int64_type_spec(), MOD);
    KlrValue *cond = klr_build_cmpgt(bldr, range_info->step, zero, "");
    KlrValue *fwd = klr_build_cmplt(bldr, range_cur, range_info->end, "");
    KlrValue *back = klr_build_cmpgt(bldr, range_cur, range_info->end, "");
    KlrValue *loop_cond = klr_build_select(bldr, cond, fwd, back, "");
    klr_build_jmp_cond(bldr, loop_cond, true_bb, false_bb);
}

/*
header:                    ;; initialize（range or iterator）
    %state = new(...)
    jmp cond

cond:                      ;; don't generate variables
    %has = call has_next(%state)
    jmp_if %has, body, end

body:                      ;; generate tmp variables
    %0 = local xxx
    %i = call next(%state)
    move %0 ,%i
    call print, %0
    jmp cond

end:
*/
static void emit_ir_for_stmt(ParserState *ps, Stmt *stmt)
{
    KlrValue *fn = CURRENT_FUNC;
    ForStmt *s = (ForStmt *)stmt;

    KlrBasicBlock *loop_header = klr_append_block(fn, "loop-header");
    KlrBasicBlock *loop_cond = klr_append_block(fn, "loop-cond");
    KlrBasicBlock *loop_body = klr_append_block(fn, "loop-body");
    KlrBasicBlock *loop_end = klr_append_block(fn, "loop-end");
    KlrValue *range_cur = NULL;
    struct RangeInfo range_info = { 0 };
    Symbol *range_sym = NULL;
    int which = 0;
#define GEN_RANGE    1
#define GEN_TUPLE    2
#define GEN_ITERATOR 3

    // 1. current block jmp to loop header
    KlrBuilder bldr;
    klr_builder_end(&bldr, ps->scope->bb);
    klr_build_jmp(&bldr, loop_header);

    // loop-header
    ParserScope *sc = enter_scope(ps, SCOPE_BLOCK, ONLY_BLOCK, "loop-header");
    sc->bb = loop_header;

    Expr *it = s->iterable;
    it->ctx = EXPR_CTX_LOAD;
    emit_ir_visit_expr(ps, it);
    if (!it->ir_val) return;

    // check last insn is call @range or call @tuple
    KlrInsn *last = insn_last(sc->bb);
    if (is_new_range(last, &range_info, ps)) {
        which = GEN_RANGE;
        klr_erase_insn(last);
        klr_builder_end(&bldr, sc->bb);

        Symbol *sym = get_loop_range_symbol(s);
        if (sym->flags & SYM_FLAGS_MUTABLE) {
            sym->ir_val = klr_build_local_var(&bldr, sym->ts, sym->name);
        } else {
            sym->ir_val = klr_build_local(&bldr, sym->ts, sym->name);
        }
        klr_build_move(&bldr, sym->ir_val, range_info.start);
        range_cur = sym->ir_val;
    } else {
        NYI();
    }

    klr_builder_end(&bldr, sc->bb);
    klr_build_jmp(&bldr, loop_cond);

    exit_scope(ps);

    // loop-cond
    sc = enter_scope(ps, SCOPE_BLOCK, ONLY_BLOCK, "loop-cond");
    sc->bb = loop_cond;

    if (which == GEN_RANGE) {
        klr_builder_end(&bldr, sc->bb);
        build_loop_range_cond(&bldr, range_cur, &range_info, loop_body, loop_end, ps);
    } else {
        NYI();
    }

    exit_scope(ps);

    // loop-body
    sc = enter_scope(ps, SCOPE_BLOCK, FOR_BLOCK, "for-block");
    sc->bb = loop_body;

    // int sym_id;
    // vector_foreach(sym_id, &s->sym_ids) {
    //     Symbol *sym = get_symbol_by_id(sym_id);
    //     ASSERT(sym->kind == SYM_VAR);
    //     VarSymbol *var_sym = (VarSymbol *)sym;
    //     ASSERT(var_sym->scope == VAR_SCOPE_LOCAL);
    //     if (sym->flags & SYM_FLAGS_MUTABLE) {
    //         sym->ir_val = klr_build_local_var(&bldr, sym->ts, sym->name);
    //     } else {
    //         sym->ir_val = klr_build_local(&bldr, sym->ts, sym->name);
    //     }
    // }

    // save continue_bb and break_bb for `break` and `continue`
    sc->continue_bb = loop_cond;
    sc->break_bb = loop_end;

    emit_ir_visit_block(ps, s->block);

    if (which == GEN_RANGE) {
        klr_builder_end(&bldr, sc->bb);
        KlrValue *tmp = klr_build_add(&bldr, range_cur, range_info.step, "");
        klr_build_move(&bldr, range_cur, tmp);
    } else {
        NYI();
    }

    // add jmp to loop_body block
    if (!block_has_terminator(sc->bb)) {
        KlrBuilder _bldr;
        klr_builder_end(&_bldr, sc->bb);
        build_loop_range_cond(&bldr, range_cur, &range_info, loop_body, loop_end, ps);
    }

    exit_scope(ps);

    ps->scope->bb = loop_end;
}

static void emit_ir_block(ParserState *ps, Stmt *stmt)
{
    KlrValue *fn = (KlrValue *)ps->scope->bb->func;
    KlrBasicBlock *block = klr_append_block(fn, "block");
    ParserScope *sc = enter_scope(ps, SCOPE_BLOCK, ONLY_BLOCK, "block");
    BlockStmt *s = (BlockStmt *)stmt;
    sc->bb = block;
    emit_ir_visit_block(ps, s->stmts);
    exit_scope(ps);
}

static void emit_ir_simple_assignment(ParserState *ps, Expr *lhs, Expr *rhs)
{
    KlrBuilder bldr;
    klr_builder_end(&bldr, ps->scope->bb);
    KlrValue *var = lhs->ir_val;
    if (var->kind == KLR_VALUE_GLOBAL) {
        klr_build_set_global(&bldr, var, rhs->ir_val);
    } else {
        klr_build_move(&bldr, var, rhs->ir_val);
    }
}

static void emit_ir_assignment(ParserState *ps, Stmt *stmt)
{
    AssignStmt *s = (AssignStmt *)stmt;
    AssignOpKind op = s->op;
    Expr *lhs = s->lhs;
    Expr *rhs = s->rhs;

    rhs->ctx = EXPR_CTX_LOAD;
    emit_ir_visit_expr(ps, rhs);

    if (op == OP_ASSIGN) {
        lhs->ctx = EXPR_CTX_STORE;
        lhs->arg = rhs;
        emit_ir_visit_expr(ps, lhs);
        if (!lhs->ir_val || !rhs->ir_val) return;
        emit_ir_simple_assignment(ps, lhs, rhs);
    } else {
        NYI();
    }
}

static void emit_ir_break(ParserState *ps, Stmt *stmt)
{
    ParserScope *sc = find_loop_scope(ps);
    // The front-end should guarantee that break statement is always inside a
    // loop, so sc should never be NULL here.
    ASSERT(sc);
    ASSERT(sc->break_bb);
    KlrBuilder bldr;
    klr_builder_end(&bldr, ps->scope->bb);
    klr_build_jmp(&bldr, sc->break_bb);

    // after jmp to break_bb, the code is unreachable, we can append a new block
    // to avoid generating ir for unreachable code ps->scope->bb =
    // klr_append_block(CURRENT_FUNC, "dead.code");

    // The front-end will guarantee that there is no code after break statement,
    // so we don't need to append a new block here, just set current block to
    // NULL to avoid generating ir for unreachable code If the front-end allows
    // code after break statement in the future, we can uncomment the above line
    // to append a new block for unreachable code ps->scope->bb = NULL;
}

static void emit_ir_continue(ParserState *ps, Stmt *stmt)
{
    ParserScope *sc = find_loop_scope(ps);
    // The front-end should guarantee that continue statement is always inside a
    // loop, so sc should never be NULL here.
    ASSERT(sc);
    ASSERT(sc->continue_bb);
    KlrBuilder bldr;
    klr_builder_end(&bldr, ps->scope->bb);
    klr_build_jmp(&bldr, sc->continue_bb);

    // after jmp to continue_bb, the code is unreachable, we can append a new
    // block to avoid generating ir for unreachable code ps->scope->bb =
    // klr_append_block(CURRENT_FUNC, "dead.code");

    // The front-end will guarantee that there is no code after continue
    // statement, so we don't need to append a new block here, just set current
    // block to NULL to avoid generating ir for unreachable code If the
    // front-end allows code after continue statement in the future, we can
    // uncomment the above line to append a new block for unreachable code
    // ps->scope->bb = NULL;
}

static void emit_ir_stmt(ParserState *ps, Stmt *stmt)
{
    if (!stmt) return;

    /* if errors is greater than MAX_ERRORS, stop parsing */
    if (ps->errors >= MAX_ERRORS) return;

    /* clang-format off */
    static void (*handlers[STMT_MAX_KIND])(ParserState *, Stmt *) = {
        [STMT_VAR_KIND]      = emit_ir_var_decl,
        [STMT_FUNC_KIND]     = emit_ir_func_decl,
        [STMT_CLASS_KIND]    = emit_ir_class,
        [STMT_TRAIT_KIND]    = emit_ir_trait,
        [STMT_RETURN_KIND]   = emit_ir_return,
        [STMT_ASSIGN_KIND]   = emit_ir_assignment,
        [STMT_BREAK_KIND]    = emit_ir_break,
        [STMT_CONTINUE_KIND] = emit_ir_continue,
        [STMT_EXPR_KIND]     = emit_ir_expr,
        [STMT_BLOCK_KIND]    = emit_ir_block,
        [STMT_IF_KIND]       = emit_ir_if_stmt,
        [STMT_WHILE_KIND]    = emit_ir_while_stmt,
        [STMT_FOR_KIND]      = emit_ir_for_stmt,
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

    KlrValue *param;
    ArgInfo *arg;
    vector_foreach(arg, sym->params) {
        param = klr_func_add_param(fval, arg->ts, arg->name);
        arg->sym->ir_val = param;
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

void kl_gen_ir(ParserModule *pm)
{
    KlrModule *m = klr_create_module(pm->path);
    pm->module = m;

    // add __init__ function firstly
    KlrValue *fn = klr_add_func(m, no_type_spec(), "__init__");
    klr_append_block(fn, "entry");
    m->init = (KlrFunc *)fn;

    ParserState *ps;
    vector_foreach(ps, &pm->pss) {
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
    }

    vector_foreach(ps, &pm->pss) {
        KlrBasicBlock *last = last_basic_block((KlrFunc *)fn);
        ParserScope *scope = enter_scope(ps, SCOPE_TOP, 0, "top");
        scope->bb = last;
        // emit ir for all statements
        Stmt *s;
        vector_foreach(s, &ps->stmts) {
            if (!s) continue;
            emit_ir_stmt(ps, s);
        }
        exit_scope(ps);
    }

    if (dump_no_opt_ir_enabled()) {
        fprintf(stdout, "--- IR Dump After ir-gen(no-opt) ---\n");
        klr_print_func((KlrFunc *)fn, stdout);
    }

    if (klr_func_empty((KlrFunc *)fn)) {
        klr_delete_func(m, (KlrFunc *)fn);
        m->init = NULL;
    }
}

#ifdef __cplusplus
}
#endif

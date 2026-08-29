/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "cmd.h"
#include "parser.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MOD ps->pm->m

#define CURRENT_FUNC ((KlrValue *)ps->scope->bb->func)
#define METHOD_SELF  (((KlrFunc *)CURRENT_FUNC)->self)

static void _add_all_intf_to_trait(KlrExtTrait *trait, KlassSymbol *kls_sym)
{
    ASSERT(vector_size(&trait->intfs) == 0);

    Vector *lro = &kls_sym->lro;
    int index = 0;

    TypeSpec *ts;
    vector_foreach(ts, lro) {
        if (!ts) continue;

        Symbol *sym = get_symbol_by_id(ts->sym_id);
        if (!sym) continue;
        if (sym->kind != SYM_TRAIT) continue;

        KlassSymbol *trait_kls = (KlassSymbol *)sym;
        Symbol *fn;
        vector_foreach(fn, trait_kls->funcs) {
            ASSERT(fn->kind == SYM_FUNC);
            FuncSymbol *func_sym = (FuncSymbol *)fn;
            klr_add_ext_intf(trait, func_sym->ret, func_sym->name);
        }
    }
}

// on-demand(used) intfs not all intfs in instance symbol.
// see: get_instance_method in parser_expr.c
// so here has some functions or inherited functions not in instance symbol, but in origin klass
// symbol.
static void _add_instance_used_intf_to_trait(KlrExtTrait *trait, InstanceSymbol *inst_sym)
{
    ASSERT(vector_size(&trait->intfs) == 0);

    KlassSymbol *kls_sym = (KlassSymbol *)inst_sym->origin;

    Vector *lro = &kls_sym->lro;
    int index = 0;

    TypeSpec *ts;
    vector_foreach(ts, lro) {
        if (!ts) continue;

        Symbol *sym = get_symbol_by_id(ts->sym_id);
        if (!sym) continue;
        if (sym->kind != SYM_TRAIT) continue;

        KlassSymbol *trait_kls = (KlassSymbol *)sym;
        Symbol *fn;
        vector_foreach(fn, trait_kls->funcs) {
            ASSERT(fn->kind == SYM_FUNC || fn->kind == SYM_INHERITED);
            if (fn->kind == SYM_INHERITED) {
                // iterate by lro, so ignore inherited functions, they are already in origin
                // klass's funcs.
                continue;
            }
            Symbol *_fn_inst_sym = stbl_get(inst_sym->stbl, fn->name);
            if (_fn_inst_sym) {
                ASSERT(_fn_inst_sym->kind == SYM_FUNC);
                klr_add_ext_intf(trait, ((FuncSymbol *)_fn_inst_sym)->ret, fn->name);
            } else {
                // reserve a empty slot for unsed intf, so the index-slots are correctly matched
                // with origin trait's intfs.
                void *empty = NULL;
                vector_push_back(&trait->intfs, &empty);
            }
        }
    }
}

// obj: class or trait type
// ts: target trait type
static KlrValue *_build_obj_intf_upcast(ParserState *ps, KlrValue *obj, TypeSpec *ts, char *name)
{
    KlrBuilder bldr;

    // 1. the same type, no cast needed
    if (ts == obj->ts) return NULL;

    // 2. if target is any, no cast needed
    if (type_is_any(ts)) return NULL;

    Symbol *ts_sym = get_symbol_by_id(ts->sym_id);
    Symbol *obj_sym = get_symbol_by_id(obj->ts->sym_id);

    // 3. no symbol found, optional type, no cast needed
    if (!ts_sym || !obj_sym) return NULL;

    // 4. if ts is an instance, use its origin for upcast
    if (ts_sym->kind == SYM_INSTANCE) {
        ts_sym = ((InstanceSymbol *)ts_sym)->origin;
        ts = ((KlassSymbol *)ts_sym)->instance_ts;
    }

    // 5. only support class/trait -> trait upcast for now
    if (ts_sym->kind != SYM_TRAIT) return NULL;

    if (obj_sym->kind == SYM_INSTANCE) {
        obj_sym = ((InstanceSymbol *)obj_sym)->origin;
    }

    if (obj_sym->kind == SYM_CLASS) {
        klr_builder_end(&bldr, ps->scope->bb);
        int intf_index = get_intf_index(obj_sym, ts);
        return klr_build_make_intf(&bldr, obj, ts, intf_index, name);
    } else if (obj_sym->kind == SYM_TRAIT) {
        klr_builder_end(&bldr, ps->scope->bb);
        int intf_index = get_intf_index(obj_sym, ts);
        return klr_build_upcast_intf(&bldr, obj, ts, intf_index, name);
    } else {
        return NULL;
    }
}

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
        if (sym->kind == SYM_SHADOW_VAR) {
            // copy ir_val from origin symbol
            ShadowVarSymbol *shadow_sym = (ShadowVarSymbol *)sym;
            sym->ir_val = shadow_sym->origin->ir_val;
        } else if (sym->kind == SYM_PACKAGE) {
            KlrExtModule *val = klr_add_ext_module(MOD, sym->name);
            sym->ir_val = (KlrValue *)val;
        } else {
            ASSERT(sym->flags & SYM_FLAGS_EXT);
            KlrValue *val = NULL;
            if (sym->kind == SYM_FUNC) {
                FuncSymbol *func_sym = (FuncSymbol *)sym;
                Symbol *parent = sym->parent;
                ASSERT(parent && parent->kind == SYM_PACKAGE);
                val = klr_add_ext_func(MOD, parent->name, func_sym->ret, sym->name);
                if (is_magic_func(func_sym)) {
                    val->magic = 1;
                }
            } else if (sym->kind == SYM_VAR) {
                Symbol *parent = sym->parent;
                ASSERT(parent && parent->kind == SYM_PACKAGE);
                val = klr_add_ext_global(MOD, parent->name, sym->ts, sym->name);
            } else if (sym->kind == SYM_CLASS) {
                Symbol *parent = sym->parent;
                ASSERT(parent && parent->kind == SYM_PACKAGE);
                val = klr_add_ext_klass(MOD, parent->name, ((KlassSymbol *)sym)->instance_ts,
                                        sym->name);
            } else {
                UNREACHABLE();
            }
            sym->ir_val = val;
        }
    }

    KlrBuilder bldr;
    klr_builder_end(&bldr, ps->scope->bb);

    switch (sym->kind) {
        case SYM_VAR: {
            VarSymbol *var_sym = (VarSymbol *)sym;
            if (exp->ctx == EXPR_CTX_LOAD) {
                if (var_sym->scope == VAR_SCOPE_GLOBAL) {
                    exp->ir_val = klr_build_get_global(&bldr, sym->ir_val);
                } else if (var_sym->scope == VAR_SCOPE_FIELD) {
                    KlrValue *self = METHOD_SELF;
                    exp->ir_val = klr_build_get_field(&bldr, self, sym->ir_val, exp->ts, "");
                } else {
                    exp->ir_val = sym->ir_val;
                }
            } else if (exp->ctx == EXPR_CTX_STORE || exp->ctx == EXPR_CTX_LOAD_STORE) {
                exp->ir_val = sym->ir_val;
            } else if (exp->ctx == EXPR_CTX_CALL) {
                // __call__()
                exp->ir_val = sym->ir_val;
            } else {
                UNREACHABLE();
            }
            break;
        }

        case SYM_SHADOW_VAR: {
            Symbol *origin = ((ShadowVarSymbol *)sym)->origin;
            ASSERT(type_is_optional(origin->ts));
            VarSymbol *var_sym = (VarSymbol *)origin;

            int readonly = origin->flags & SYM_FLAGS_MUTABLE ? 0 : 1;
            TypeSpec *ts = sym->ts;

            if (exp->ctx == EXPR_CTX_LOAD) {
                if (!type_is_optional(ts)) {
                    // non-optional shadow var → unbox

                    KlrValue *val;
                    if (var_sym->scope == VAR_SCOPE_FIELD) {
                        KlrValue *self = METHOD_SELF;
                        val = klr_build_get_field(&bldr, self, origin->ir_val, exp->ts, "");
                    } else {
                        val = origin->ir_val;
                    }

                    if (val->ts != ts) {
                        KlrValue *unbox = klr_build_cast(&bldr, val, ts, "");
                        klr_set_loc(unbox, ps->filename, exp->loc);
                        exp->ir_val = unbox;
                    } else {
                        klr_set_loc(val, ps->filename, exp->loc);
                        exp->ir_val = val;
                    }
                } else {
                    // optional shadow var → no unbox
                    exp->ir_val = origin->ir_val;
                }
            } else {
                ASSERT(exp->ctx == EXPR_CTX_STORE || exp->ctx == EXPR_CTX_LOAD_STORE);
                exp->ir_val = sym->ir_val;
            }

            break;
        }

        case SYM_FUNC: {
            ASSERT(sym->ir_val);
            exp->ir_val = sym->ir_val;
            break;
        }

        case SYM_CLASS: {
            ASSERT(sym->ir_val);
            exp->ir_val = sym->ir_val;
            break;
        }

        case SYM_PACKAGE: {
            ASSERT(sym->ir_val);
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
            if (lit->sign) {
                exp->ir_val = klr_const_int(lit->ival, lit->ts, m);
            } else {
                exp->ir_val = klr_const_uint(lit->ival, lit->ts, m);
            }
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

    klr_set_loc(exp->ir_val, ps->filename, exp->loc);
}

static void emit_ir_self(ParserState *ps, Expr *exp)
{
    FuncSymbol *fn_sym = get_current_function(ps);
    ASSERT(fn_sym && fn_sym->kind == SYM_FUNC);
    ASSERT(fn_sym->ir_val);
    KlrFunc *fn_ir_val = (KlrFunc *)fn_sym->ir_val;
    ASSERT(fn_ir_val->kind == KLR_VALUE_FUNC);
    exp->ir_val = fn_ir_val->self;
    ASSERT(exp->ir_val);
}

static void emit_ir_type(ParserState *ps, Expr *exp)
{
    Symbol *sym = exp->sym;
    if (sym->kind == SYM_CLASS) {
        KlassSymbol *kls_sym = (KlassSymbol *)sym;
        ASSERT(sym->flags & SYM_FLAGS_EXT);
        Symbol *parent = sym->parent;
        ASSERT(parent && parent->kind == SYM_PACKAGE);
        exp->ir_val = klr_add_ext_klass(MOD, parent->name, kls_sym->instance_ts, sym->name);
    } else if (sym->kind == SYM_INSTANCE) {
        InstanceSymbol *inst_sym = (InstanceSymbol *)sym;
        Symbol *origin = inst_sym->origin;
        if (!origin->ir_val) {
            // TODO: why is null?
            ASSERT(str_equal(origin->name, "tuple") || str_equal(origin->name, "list"));
        }
        exp->ir_val = origin->ir_val;
    }
    sym->ir_val = exp->ir_val;
}

static KlrValue *emit_int_call(KlrBuilder *bldr, KlrValue *callee, KlrValue **args, int nargs)
{
    // TODO: base = 10
    // if (nargs == 1) {
    KlrValue *arg = args[0];
    TypeSpec *ts = callee->ts;
    TypeSpec *arg_ts = arg->ts;
    if (ts->kind == TYPE_INT && arg_ts->kind == TYPE_INT) {
        if (arg_ts == ts) return arg;
        KlrValue *ret = klr_build_cast(bldr, arg, ts, "");
        return ret;
    }
    // }

    NYI();
}

static KlrValue *emit_float_call(KlrBuilder *bldr, KlrValue *callee, KlrValue **args, int nargs)
{
    if (nargs == 1) {
        KlrValue *arg = args[0];
        TypeSpec *ts = callee->ts;
        TypeSpec *arg_ts = arg->ts;
        if (ts->kind == TYPE_FLOAT && arg_ts->kind == TYPE_FLOAT) {
            if (arg_ts == ts) return arg;
            KlrValue *ret = klr_build_cast(bldr, arg, ts, "");
            return ret;
        }
    }

    NYI();
}

static KlrValue *emit_str_call(KlrValue **args, int nargs)
{
    ASSERT(nargs == 1);
    KlrValue *arg = args[0];
    return arg;
}

static KlrValue *emit_range_call(ParserState *ps, KlrBuilder *bldr, KlrValue *callee,
                                 KlrValue **args, int nargs)
{
    ASSERT(nargs == 3);

    int konst = 1;

    for (int i = 0; i < nargs; i++) {
        KlrValue *arg = args[i];
        if (!klr_is_const(arg)) {
            konst = 0;
            break;
        }
    }

    KlrValue *step = args[2];
    if (klr_is_const(step)) {
        KlrConst *kc = (KlrConst *)step;
        if (kc->which == CONST_INT && kc->ival == 0) {
            KlrModule *m = ps->pm->m;
            KlrLocInfo *loc = &step->loc;
            klr_error(loc, "range step cannot be zero");
        }
    }

    KlrValue *ret;

    if (konst) {
        ret = klr_const_range(args, callee->ts, MOD);
    } else {
        ret = klr_build_intern(bldr, args, nargs, callee->ts, INTERN_RANGE, "");
    }
    return ret;
}

static KlrValue *emit_tuple_call(ParserState *ps, KlrBuilder *bldr, KlrValue *callee,
                                 KlrValue **args, int nargs)
{
    ASSERT(nargs == 1);
    KlrValue *arg = args[0];
    ASSERT(klr_is_const(arg) || arg->kind == KLR_VALUE_INSN);
    return arg;
}

static KlrValue *emit_list_call(ParserState *ps, KlrBuilder *bldr, KlrValue *callee,
                                KlrValue **args, int nargs)
{
    if (nargs == 0) {
        // list() → empty list constant
        return klr_const_list(NULL, 0, callee->ts, MOD);
    }

    ASSERT(nargs == 1);
    KlrValue *arg = args[0];

    if (klr_is_const(arg)) {
        KlrConst *kc = (KlrConst *)arg;
        ASSERT(kc->which == CONST_TUPLE);
        // list[tuple(...)]
        Vector *vec = kc->list;
        KlrValue **_args = VECTOR_RAW(vec, KlrValue *);
        int _nargs = vector_size(vec);
        return klr_const_list(_args, _nargs, callee->ts, MOD);
    } else {
        // build_intern @tuple(...) → build_intern @list(...)
        ASSERT(arg->kind == KLR_VALUE_INSN);
        KlrInsn *insn = (KlrInsn *)arg;
        ASSERT(insn->code == OP_BUILD_INTERN && insn->intern_tag == INTERN_TUPLE);
        insn->intern_tag = INTERN_LIST;
        insn->ts = callee->ts;
        return arg;
    }
}

static void update_call_args(ParserState *ps, KlrValue **args, int nargs, TypeSpec *proto_ts)
{
    if (!proto_ts) {
        ASSERT(nargs == 0);
        return;
    }

    ASSERT(proto_ts->kind == TYPE_PROTO);
    Vector *proto_args = proto_ts->proto_type.args;
    int proto_nargs = vector_size(proto_args);
    ASSERT(proto_nargs == nargs);

    for (int i = 0; i < nargs; i++) {
        KlrValue *arg = args[i];
        TypeSpec *param_ts = vector_at(proto_args, i);
        KlrValue *casted_arg = _build_obj_intf_upcast(ps, arg, param_ts, "");
        if (casted_arg) args[i] = casted_arg;
    }
}

static KlrValue *emit_type_call(ParserState *ps, KlrValue *callee, KlrValue *init_fn,
                                KlrValue **args, int nargs, TypeSpec *ret_ts)
{
    KlrValue *ret = NULL;

    KlrBuilder bldr;
    klr_builder_end(&bldr, ps->scope->bb);

    TypeSpec *ts = callee->ts;
    if (ts->kind == TYPE_INT) {
        ret = emit_int_call(&bldr, callee, args, nargs);
    } else if (ts->kind == TYPE_FLOAT) {
        ret = emit_float_call(&bldr, callee, args, nargs);
    } else if (type_is_str(ts)) {
        ret = emit_str_call(args, nargs);
    } else if (type_is_range(ts)) {
        ret = emit_range_call(ps, &bldr, callee, args, nargs);
    } else if (type_is_tuple(ts)) {
        ret = emit_tuple_call(ps, &bldr, callee, args, nargs);
    } else if (type_is_list(ts)) {
        ret = emit_list_call(ps, &bldr, callee, args, nargs);
    } else if (ts->kind == TYPE_KLASS) {
        ASSERT(init_fn);
        log_info("emit type call for klass(klr_build_new), ts:");
        log_type_spec(ret_ts);
        ret = klr_build_new(&bldr, callee, ret_ts, "");
        KlrValue *_args[nargs + 1];
        _args[0] = ret;
        for (int i = 0; i < nargs; i++) {
            _args[i + 1] = args[i];
        }
        TypeSpec *init_fn_ts;
        if (init_fn->kind == KLR_VALUE_EXT_FUNC) {
            init_fn_ts = ((KlrExtFunc *)init_fn)->proto;
        } else {
            KlrFunc *func = (KlrFunc *)init_fn;
            Vector *params = vector_create_ptr();
            KlrValue *arg;
            vector_foreach(arg, &func->params) {
                if (arg == func->self) continue;
                TypeSpec *ts = arg->ts;
                vector_push_back(params, &ts);
            }
            init_fn_ts = func_type_spec(params, func->ts);
        }
        update_call_args(ps, _args + 1, nargs, init_fn_ts);
        klr_builder_end(&bldr, ps->scope->bb);
        klr_build_call(&bldr, init_fn, no_type_spec(), _args, nargs + 1, "");
    } else {
        NYI();
    }

    return ret;
}

static void emit_ir_call(ParserState *ps, Expr *exp)
{
    CallExpr *call = (CallExpr *)exp;
    Expr *lhs = call->lhs;
    Vector *args = call->args;
    int size = vector_size(args);

    // gen ir for lhs
    lhs->ctx = EXPR_CTX_CALL;
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

    KlrValue *callee = lhs->ir_val;
    KlrValue *ret;

    if (callee->kind == KLR_VALUE_KLASS) {
        Symbol *lhs_sym = lhs->sym;
        ASSERT(lhs_sym->kind == SYM_CLASS || lhs_sym->kind == SYM_INSTANCE);
        KlassSymbol *kls_sym = (KlassSymbol *)lhs_sym;
        if (lhs_sym->kind == SYM_INSTANCE) {
            InstanceSymbol *inst_sym = (InstanceSymbol *)lhs_sym;
            Symbol *origin = inst_sym->origin;
            kls_sym = (KlassSymbol *)origin;
        }
        Symbol *_sym = kls_sym->__init__;
        ASSERT(_sym && _sym->ir_val);
        KlrValue *init_fn = _sym->ir_val;
        ret = emit_type_call(ps, callee, init_fn, ir_args, size, exp->ts);
    } else if (callee->kind == KLR_VALUE_EXT_KLASS) {
        Symbol *lhs_sym = lhs->sym;
        KlassSymbol *kls_sym;

        if (lhs_sym->kind == SYM_INSTANCE) {
            InstanceSymbol *inst_sym = (InstanceSymbol *)lhs_sym;
            Symbol *origin = inst_sym->origin;
            kls_sym = (KlassSymbol *)origin;
        } else {
            ASSERT(lhs_sym->kind == SYM_CLASS);
            kls_sym = (KlassSymbol *)lhs_sym;
        }

        if (!kls_sym->ir_val) {
            ASSERT(kls_sym->flags & SYM_FLAGS_EXT);
            Symbol *parent = kls_sym->parent;
            ASSERT(parent && parent->kind == SYM_PACKAGE);
            kls_sym->ir_val =
                klr_add_ext_klass(MOD, parent->name, kls_sym->instance_ts, kls_sym->name);
        }

        Symbol *_sym = kls_sym->__init__;
        ASSERT(_sym);
        if (!_sym->ir_val) {
            ASSERT(_sym->flags & SYM_FLAGS_EXT);
            ASSERT(_sym->kind == SYM_FUNC);
            FuncSymbol *func_sym = (FuncSymbol *)_sym;
            KlrExtKlass *kls_ir_val = (KlrExtKlass *)(kls_sym->ir_val);
            KlrValue *_val = klr_add_ext_method(kls_ir_val, func_sym->ret, _sym->name);
            _sym->ir_val = _val;
            ((KlrExtFunc *)_val)->proto = func_sym->ts;
        }

        KlrValue *init_fn = _sym->ir_val;
        ret = emit_type_call(ps, callee, init_fn, ir_args, size, exp->ts);
    } else {
        // normal call, try to build interface cast
        update_call_args(ps, ir_args, size, lhs->ts);

        KlrValue *self = lhs->arg;
        if (self) {
            if (self->kind == KLR_VALUE_EXT_MODULE) {
                // module function call, e.g. math.sin()
                KlrBuilder bldr;
                klr_builder_end(&bldr, ps->scope->bb);
                ret = klr_build_call(&bldr, callee, exp->ts, ir_args, size, "");
            } else if (self->kind == KLR_VALUE_KLASS || self->kind == KLR_VALUE_EXT_KLASS) {
                // Foo.hello(), static method call
                KlrBuilder bldr;
                klr_builder_end(&bldr, ps->scope->bb);
                ret = klr_build_call(&bldr, callee, exp->ts, ir_args, size, "");
            } else {
                // method call
                KlrValue *_args[size + 1];
                _args[0] = self;
                for (int i = 0; i < size; i++) {
                    _args[i + 1] = ir_args[i];
                }
                size += 1;
                KlrBuilder bldr;
                klr_builder_end(&bldr, ps->scope->bb);
                ret = klr_build_call(&bldr, callee, exp->ts, _args, size, "");
            }
        } else {
            Symbol *parent = lhs->sym->parent;
            if (parent && parent->kind == SYM_CLASS) {
                FuncSymbol *cur_fn = get_current_function(ps);
                ASSERT(cur_fn->parent == parent);
                // method call
                KlrValue *_args[size + 1];
                _args[0] = METHOD_SELF;
                for (int i = 0; i < size; i++) {
                    _args[i + 1] = ir_args[i];
                }
                size += 1;
                KlrBuilder bldr;
                klr_builder_end(&bldr, ps->scope->bb);
                ret = klr_build_call(&bldr, callee, exp->ts, _args, size, "");
            } else if (!parent || parent->kind == SYM_PACKAGE) {
                KlrBuilder bldr;
                klr_builder_end(&bldr, ps->scope->bb);
                ret = klr_build_call(&bldr, callee, exp->ts, ir_args, size, "");
            } else {
                UNREACHABLE();
            }
        }
    }

    klr_set_loc(ret, ps->filename, exp->loc);
    exp->ir_val = ret;
}

static Symbol *_get_field(Vector *fields, const char *name)
{
    Symbol *sym;
    vector_foreach(sym, fields) {
        if (str_equal(sym->name, name)) {
            return sym;
        }
    }

    UNREACHABLE();
}

static KlrValue *build_get_field(TypeSpec *ts, char *name, KlrBuilder *bldr, KlrValue *val,
                                 TypeSpec *field_ts, ParserState *ps)
{
    Symbol *sym = get_symbol_by_id(ts->sym_id);
    int field_index = -1;

    if (sym->kind == SYM_CLASS) {
        KlassSymbol *kls_sym = (KlassSymbol *)sym;
        Symbol *fld_sym = stbl_get(kls_sym->stbl, name);
        ASSERT(fld_sym && fld_sym->kind == SYM_VAR);

        if (kls_sym->flags & SYM_FLAGS_EXT) {
            KlrValue *kls_ir_val = kls_sym->ir_val;
            if (!kls_ir_val) {
                Symbol *parent = sym->parent;
                ASSERT(parent && parent->kind == SYM_PACKAGE);
                kls_ir_val = klr_add_ext_klass(MOD, parent->name, kls_sym->instance_ts, sym->name);
                kls_sym->ir_val = kls_ir_val;
            }
            ASSERT(kls_ir_val->kind == KLR_VALUE_EXT_KLASS);
            KlrValue *fld_ir_val = fld_sym->ir_val;
            if (!fld_ir_val) {
                ASSERT(fld_sym->flags & SYM_FLAGS_EXT);
                FuncSymbol *func_sym = (FuncSymbol *)fld_sym;
                fld_ir_val = klr_add_ext_field((KlrExtKlass *)kls_ir_val, fld_sym->ts, name);
                fld_sym->ir_val = fld_ir_val;
            }
            ASSERT(fld_ir_val->kind == KLR_VALUE_EXT_FIELD);
            return klr_build_get_field_ext(bldr, val, fld_ir_val, "");
        } else {
            ASSERT(fld_sym->ir_val);
            return klr_build_get_field(bldr, val, fld_sym->ir_val, field_ts, "");
        }
    } else if (sym->kind == SYM_INSTANCE) {
        InstanceSymbol *inst_sym = (InstanceSymbol *)sym;
        Symbol *origin = inst_sym->origin;
        ASSERT(origin->kind == SYM_CLASS);

        Symbol *fld_sym = stbl_get(origin->stbl, name);
        ASSERT(fld_sym && fld_sym->kind == SYM_VAR);

        if (origin->flags & SYM_FLAGS_EXT) {
            NYI();
        } else {
            ASSERT(fld_sym->ir_val);
            return klr_build_get_field(bldr, val, fld_sym->ir_val, field_ts, "");
        }
    } else {
        UNREACHABLE();
    }

    // fi->index = field_index;
    // fi->name = name;

    return NULL;
}

static KlrValue *get_ext_global_value(char *name, Symbol *sym, ParserState *ps)
{
    ASSERT(sym->kind == SYM_PACKAGE);
    PkgSymbol *pkg = (PkgSymbol *)sym;
    Symbol *var_sym = stbl_get(pkg->stbl, name);
    ASSERT(var_sym->kind == SYM_VAR);
    VarSymbol *var = (VarSymbol *)var_sym;
    if (!var->ir_val) {
        KlrValue *val = klr_add_ext_global(MOD, pkg->name, var->ts, var->name);
        var->ir_val = val;
    }
    return var->ir_val;
}

static void emit_ir_dot(ParserState *ps, Expr *exp)
{
    DotExpr *dot = (DotExpr *)exp;
    int opt_or_bang = dot->opt_or_bang;

    Expr *lhs = dot->lhs;
    lhs->ctx = EXPR_CTX_LOAD;
    emit_ir_visit_expr(ps, lhs);
    if (!lhs->ir_val) return;

    Symbol *sym = exp->sym;
    if (sym->kind == SYM_FUNC) {
        if (exp->ctx == EXPR_CTX_CALL) {
            if (!sym->ir_val) {
                // ASSERT(sym->flags & SYM_FLAGS_EXT);
                Symbol *_sym = sym->parent;
                if (_sym->kind == SYM_INSTANCE) {
                    InstanceSymbol *inst_sym = (InstanceSymbol *)_sym;
                    Symbol *origin = inst_sym->origin;
                    if (origin->flags & SYM_FLAGS_EXT) {
                        if (origin->kind == SYM_CLASS) {
                            KlrValue *_val = _sym->ir_val;
                            if (!_val) {
                                Symbol *parent = origin->parent;
                                ASSERT(parent && parent->kind == SYM_PACKAGE);
                                _val = klr_add_ext_klass(MOD, parent->name, inst_sym->instance_ts,
                                                         origin->name);
                                _sym->ir_val = _val;
                            }
                            ASSERT(_val && _val->kind == KLR_VALUE_EXT_KLASS);
                            KlrExtKlass *ext_kls = (KlrExtKlass *)_val;
                            exp->ir_val =
                                klr_add_ext_method(ext_kls, ((FuncSymbol *)sym)->ret, sym->name);
                        } else if (origin->kind == SYM_TRAIT) {
                            KlrValue *_val = _sym->ir_val;
                            if (!_val) {
                                Symbol *parent = origin->parent;
                                ASSERT(parent && parent->kind == SYM_PACKAGE);
                                _val = klr_add_ext_trait(MOD, parent->name, inst_sym->instance_ts,
                                                         _sym->name);
                                _sym->ir_val = _val;
                                _add_instance_used_intf_to_trait((KlrExtTrait *)_val, inst_sym);
                            }
                            ASSERT(_val && _val->kind == KLR_VALUE_EXT_TRAIT);
                            exp->ir_val = klr_get_ext_intf((KlrExtTrait *)_val, sym->name);
                            ASSERT(exp->ir_val);
                        } else {
                            UNREACHABLE();
                        }
                    } else {
                        Symbol *_fn_sym = stbl_get(origin->stbl, sym->name);
                        ASSERT(_fn_sym &&
                               (_fn_sym->kind == SYM_FUNC || _fn_sym->kind == SYM_INHERITED));
                        ASSERT(_fn_sym->ir_val);
                        exp->ir_val = _fn_sym->ir_val;
                    }
                } else if (_sym->kind == SYM_CLASS) {
                    KlassSymbol *kls_sym = (KlassSymbol *)_sym;
                    KlrValue *_val = kls_sym->ir_val;
                    if (!_val) {
                        _val = klr_add_ext_klass(MOD, kls_sym->path, kls_sym->instance_ts,
                                                 kls_sym->name);
                        kls_sym->ir_val = _val;
                    }
                    ASSERT(_val && _val->kind == KLR_VALUE_EXT_KLASS);
                    KlrExtKlass *ext_kls = (KlrExtKlass *)_val;
                    exp->ir_val = klr_add_ext_method(ext_kls, ((FuncSymbol *)sym)->ret, sym->name);
                } else if (_sym->kind == SYM_PACKAGE) {
                    KlrValue *_val = _sym->ir_val;
                    ASSERT(_val && _val->kind == KLR_VALUE_EXT_MODULE);
                    KlrExtModule *ext_mod = (KlrExtModule *)_val;
                    exp->ir_val =
                        klr_add_ext_func(MOD, _sym->name, ((FuncSymbol *)sym)->ret, sym->name);
                } else {
                    ASSERT(_sym->kind == SYM_TRAIT);
                    ASSERT(_sym->flags & SYM_FLAGS_EXT);
                    KlrValue *_val = _sym->ir_val;
                    if (!_val) {
                        Symbol *parent = _sym->parent;
                        ASSERT(parent && parent->kind == SYM_PACKAGE);
                        _val = klr_add_ext_trait(MOD, parent->name,
                                                 ((KlassSymbol *)_sym)->instance_ts, _sym->name);
                        _sym->ir_val = _val;
                        _add_all_intf_to_trait((KlrExtTrait *)_val, (KlassSymbol *)_sym);
                    }
                    ASSERT(_val && _val->kind == KLR_VALUE_EXT_TRAIT);
                    exp->ir_val = klr_get_ext_intf((KlrExtTrait *)_val, sym->name);
                    ASSERT(exp->ir_val);
                }
                sym->ir_val = exp->ir_val;
            } else {
                exp->ir_val = sym->ir_val;
            }
            // use expr's arg to save the lhs's ir_val, so that we can use it in emit_ir_call
            exp->arg = lhs->ir_val;
        } else if (exp->ctx == EXPR_CTX_LOAD) {
            KlrValue *fn_ir_val;
            if (!sym->ir_val) {
                ASSERT(sym->flags & SYM_FLAGS_EXT);
                Symbol *_sym = sym->parent;
                ASSERT(_sym->kind == SYM_CLASS);
                KlassSymbol *kls_sym = (KlassSymbol *)_sym;
                KlrValue *_val = kls_sym->ir_val;
                ASSERT(_val && _val->kind == KLR_VALUE_EXT_KLASS);
                KlrExtKlass *ext_kls = (KlrExtKlass *)_val;
                fn_ir_val = klr_add_ext_method(ext_kls, ((FuncSymbol *)sym)->ret, sym->name);
            } else {
                fn_ir_val = sym->ir_val;
            }
            NYI();
            // gen a closure for method
            // KlrBuilder bldr;
            // klr_builder_end(&bldr, ps->scope->bb);
            // KlrValue *closure = klr_build_closure(&bldr, fn_ir_val, lhs->ir_val, "");
            // klr_set_loc(closure, ps->filename, exp->loc);
            // exp->ir_val = closure;
        } else {
            UNREACHABLE();
        }
        return;
    }

    if (sym->kind == SYM_CLASS) {
        if (!sym->ir_val) {
            Symbol *parent = sym->parent;
            ASSERT(parent && parent->kind == SYM_PACKAGE);
            KlrValue *_val = parent->ir_val;
            ASSERT(_val && _val->kind == KLR_VALUE_EXT_MODULE);
            KlrExtModule *ext_mod = (KlrExtModule *)_val;
            exp->ir_val =
                klr_add_ext_klass(MOD, parent->name, ((KlassSymbol *)sym)->instance_ts, sym->name);
            sym->ir_val = exp->ir_val;
        } else {
            exp->ir_val = sym->ir_val;
        }
        return;
    }

    if (sym->kind == SYM_INHERITED) {
        if (exp->ctx == EXPR_CTX_CALL) {
            if (!sym->ir_val) {
                NYI();
            }
            exp->ir_val = sym->ir_val;
            // use expr's arg to save the lhs's ir_val, so that we can use it in emit_ir_call
            exp->arg = lhs->ir_val;
        } else {
            UNREACHABLE();
        }
        return;
    }

    ASSERT(sym->kind == SYM_VAR);

    if (exp->ctx == EXPR_CTX_LOAD) {
        KlrValue *lhs_val = lhs->ir_val;
        if (lhs_val->kind == KLR_VALUE_EXT_MODULE) {
            // load global from external module
            KlrBuilder bldr;
            klr_builder_end(&bldr, ps->scope->bb);
            KlrValue *global_val = get_ext_global_value(dot->id.name, lhs->sym, ps);
            KlrValue *val = klr_build_get_global(&bldr, global_val);
            klr_set_loc(val, ps->filename, exp->loc);
            exp->ir_val = val;
        } else {
            // load field
            KlrBuilder bldr;
            klr_builder_end(&bldr, ps->scope->bb);
            KlrValue *val = build_get_field(lhs->ts, dot->id.name, &bldr, lhs_val, exp->ts, ps);
            klr_set_loc(val, ps->filename, exp->loc);
            exp->ir_val = val;
        }
    } else {
        // store field
        ASSERT(exp->ctx == EXPR_CTX_STORE);
        Symbol *lhs_ts_sym = get_symbol_by_id(lhs->ts->sym_id);
        if (lhs_ts_sym->kind == SYM_CLASS) {
            KlassSymbol *kls_sym = (KlassSymbol *)lhs_ts_sym;
            Symbol *fld_sym = stbl_get(kls_sym->stbl, dot->id.name);
            ASSERT(fld_sym && fld_sym->kind == SYM_VAR);

            ASSERT(fld_sym->ir_val);
            exp->ir_val = fld_sym->ir_val;
        } else if (lhs_ts_sym->kind == SYM_INSTANCE) {
            InstanceSymbol *inst_sym = (InstanceSymbol *)lhs_ts_sym;
            Symbol *origin = inst_sym->origin;
            ASSERT(origin->kind == SYM_CLASS);

            Symbol *fld_sym = stbl_get(origin->stbl, dot->id.name);
            ASSERT(fld_sym && fld_sym->kind == SYM_VAR);

            ASSERT(fld_sym->ir_val);
            exp->ir_val = fld_sym->ir_val;
        } else if (lhs_ts_sym->kind == SYM_PACKAGE) {
            // store global from external module
            exp->ir_val = get_ext_global_value(dot->id.name, lhs_ts_sym, ps);
        } else {
            UNREACHABLE();
        }
        // exp->ir_val = sym->ir_val;
    }
}

static void emit_ir_index(ParserState *ps, Expr *exp)
{
    IndexExpr *index = (IndexExpr *)exp;
    Expr *lhs = index->lhs;
    Vector *vec = index->vec;

    lhs->ctx = EXPR_CTX_LOAD;
    emit_ir_visit_expr(ps, lhs);
    if (!lhs->ir_val) return;

    if (lhs->ts->kind == TYPE_TYPE) {
        // type index, e.g. list[int]
        emit_ir_type(ps, exp);
        return;
    }

    if (lhs->ts->kind == TYPE_PROTO) {
        // assign function's KlrValue to expr's ir_val
        exp->ir_val = lhs->sym->ir_val;
        ASSERT(exp->ir_val);
        return;
    }

    if (exp->ctx == EXPR_CTX_LOAD_STORE) {
        NYI();
        return;
    }

    if (vector_size(vec) == 1) {
        Expr *e = vector_at(vec, 0);
        e->ctx = EXPR_CTX_LOAD;
        emit_ir_visit_expr(ps, e);
        if (!e->ir_val) return;
        KlrValue *ir_val = e->ir_val;

        KlrValue *item;
        KlrBuilder bldr;
        klr_builder_end(&bldr, ps->scope->bb);

        if (type_is_seq(lhs->ts)) {
            // sequence index
            if (exp->ctx == EXPR_CTX_LOAD) {
                item = klr_build_seq_get(&bldr, lhs->ir_val, ir_val, exp->ts, "");
            } else {
                item = klr_new_index(&bldr, lhs->ir_val, ir_val, KLR_SEQ_SET);
            }
        } else if (type_is_map(lhs->ts)) {
            // map index
            if (exp->ctx == EXPR_CTX_LOAD) {
                item = klr_build_map_get(&bldr, lhs->ir_val, ir_val, exp->ts, "");
            } else {
                item = klr_new_index(&bldr, lhs->ir_val, ir_val, KLR_MAP_SET);
            }
        } else {
            UNREACHABLE();
        }

        klr_set_loc(item, ps->filename, exp->loc);
        exp->ir_val = item;
        return;
    }

    // multi-dimensional index or slice
    NYI();
}

static void emit_ir_slice(ParserState *ps, Expr *exp) { NYI(); }

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
        case BINARY_SHL:
            return "shl";
        case BINARY_SHR:
            return "shr";
        case BINARY_BIT_AND:
            return "and";
        case BINARY_BIT_OR:
            return "or";
        case BINARY_BIT_XOR:
            return "xor";
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

    /*
    if (type_is_str(lhs->ts)) {
        ASSERT(op == BINARY_ADD);
        KlrValue *fn = klr_add_ext_func(MOD, lhs->ts, "std/builtin", "str_add");
        KlrValue *args[] = { lhs->ir_val, rhs->ir_val };
        KlrValue *res = klr_build_call(&bldr, fn, args, 2, "");
        exp->ir_val = res;
        res->ast.filename = ps->filename;
        res->ast.loc = exp->loc;
        return;
    }
    */

    KlrValue *res;

    KlrValue *cast_lhs = lhs->ir_val;
    KlrValue *cast_rhs = rhs->ir_val;

    TypeSpec *lhs_ts = cast_lhs->ts;
    TypeSpec *rhs_ts = cast_rhs->ts;

    if (type_is_int(lhs_ts)) {
        if (lhs_ts->int_flt_info.width < 8) {
            cast_lhs = klr_build_cast(&bldr, cast_lhs, int64_type_spec(), "");
        }
    } else if (type_is_uint(lhs_ts)) {
        if (lhs_ts->int_flt_info.width < 8) {
            cast_lhs = klr_build_cast(&bldr, cast_lhs, uint64_type_spec(), "");
        }
    }

    if (type_is_int(rhs_ts)) {
        if (rhs_ts->int_flt_info.width < 8) {
            cast_rhs = klr_build_cast(&bldr, cast_rhs, int64_type_spec(), "");
        }
    } else if (type_is_uint(rhs_ts)) {
        if (rhs_ts->int_flt_info.width < 8) {
            cast_rhs = klr_build_cast(&bldr, cast_rhs, uint64_type_spec(), "");
        }
    }

    if (type_is_float(lhs_ts)) {
        if (lhs_ts->int_flt_info.width < 8) {
            cast_lhs = klr_build_cast(&bldr, cast_lhs, float64_type_spec(), "");
        }
    }

    if (type_is_float(rhs_ts)) {
        if (rhs_ts->int_flt_info.width < 8) {
            cast_rhs = klr_build_cast(&bldr, cast_rhs, float64_type_spec(), "");
        }
    }

    if (op >= BINARY_GT && op <= BINARY_NEQ) {
        res = klr_build_cmp(&bldr, cast_lhs, cast_rhs, get_binary_op_code(op), "");
    } else {
        res = klr_build_binary(&bldr, cast_lhs, cast_rhs, get_binary_op_code(op), "",
                               get_binary_op_name(op));
    }

    if (exp->ts) res->ts = exp->ts;
    ASSERT(res->ts);
    exp->ir_val = res;
    klr_set_loc(res, ps->filename, exp->loc);
}

static void emit_ir_unary(ParserState *ps, Expr *exp)
{
    UnaryExpr *unary = (UnaryExpr *)exp;
    UnOpKind op = unary->op;

    Expr *e = unary->exp;
    e->ctx = EXPR_CTX_LOAD;
    emit_ir_visit_expr(ps, e);
    if (!e->ir_val) return;

    KlrValue *res = NULL;
    KlrBuilder bldr;
    klr_builder_end(&bldr, ps->scope->bb);

    switch (op) {
        case UNARY_PLUS: {
            // unary plus is a no-op, just return the value
            res = e->ir_val;
            break;
        }
        case UNARY_NEG: {
            res = klr_build_unary(&bldr, e->ir_val, OP_UNARY_NEG, "", "neg");
            break;
        }
        case UNARY_BIT_NOT: {
            res = klr_build_unary(&bldr, e->ir_val, OP_UNARY_NOT, "", "bit_not");
            break;
        }
        case UNARY_NOT: {
            res = klr_build_unary(&bldr, e->ir_val, OP_LNOT, "", "not");
            break;
        }
        default: {
            UNREACHABLE();
            break;
        }
    }

    exp->ir_val = res;
    klr_set_loc(res, ps->filename, exp->loc);
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
        KlrValue *ret = klr_build_intern(&bldr, items, size, exp->ts, INTERN_LIST, "");
        exp->ir_val = ret;
    }
}

static void emit_ir_tuple(ParserState *ps, Expr *exp)
{
    TupleExpr *tuple = (TupleExpr *)exp;

    int size = vector_size(tuple->vec);
    KlrValue *items[size];
    int konst = 1;

    Expr *e;
    vector_foreach(e, tuple->vec) {
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
        KlrValue *ret = klr_build_intern(&bldr, items, size, exp->ts, INTERN_TUPLE, "");
        exp->ir_val = ret;
    }
}

static void emit_ir_kw(ParserState *ps, Expr *exp) { UNREACHABLE(); }

static void emit_ir_bang(ParserState *ps, Expr *exp)
{
    BangExpr *bang = (BangExpr *)exp;
    Expr *e = bang->exp;
    e->ctx = EXPR_CTX_LOAD;
    emit_ir_visit_expr(ps, e);
    if (!e->ir_val) return;
    exp->ir_val = e->ir_val;
}

static void emit_ir_const_placeholder(ParserState *ps, Expr *exp)
{
    ConstPlaceholderExpr *cp = (ConstPlaceholderExpr *)exp;
    Literal *lit = cp->lit;
    KlrModule *m = MOD;

    switch (lit->which) {
        case LIT_INT: {
            if (lit->sign) {
                exp->ir_val = klr_const_int(lit->ival, cp->ts, m);
            } else {
                exp->ir_val = klr_const_uint(lit->ival, cp->ts, m);
            }
            break;
        }
        case LIT_FLT: {
            exp->ir_val = klr_const_float(lit->fval, cp->ts, m);
            break;
        }
        case LIT_BOOL: {
            exp->ir_val = klr_const_bool(lit->bval, m);
            break;
        }
        case LIT_STR: {
            exp->ir_val = klr_const_str(lit->sval, lit->len, m);
            break;
        }
        case LIT_NONE: {
            exp->ir_val = klr_const_none(m);
            break;
        }
        default: {
            UNREACHABLE();
            break;
        }
    }

    klr_set_loc(exp->ir_val, ps->filename, exp->loc);
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
        [EXPR_SELF_KIND]    = emit_ir_self,
        [EXPR_LIST_KIND]    = emit_ir_list,
        [EXPR_TUPLE_KIND]   = emit_ir_tuple,
        [EXPR_TYPE_KIND]    = emit_ir_type,
        [EXPR_CALL_KIND]    = emit_ir_call,
        [EXPR_DOT_KIND]     = emit_ir_dot,
        [EXPR_INDEX_KIND]   = emit_ir_index,
        [EXPR_SLICE_KIND]   = emit_ir_slice,
        [EXPR_UNARY_KIND]   = emit_ir_unary,
        [EXPR_BINARY_KIND]  = emit_ir_binary,
        [EXPR_KW_KIND]      = emit_ir_kw,
        [EXPR_BANG_KIND]    = emit_ir_bang,
        [EXPR_CONST_PLACEHOLDER] = emit_ir_const_placeholder,
    };
    /* clang-format on */

    handlers[exp->kind](ps, exp);
}

static void emit_ir_import(ParserState *ps, Stmt *stmt) {}
static void emit_ir_link(ParserState *ps, Stmt *stmt) {}

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
            KlrValue *_val = _build_obj_intf_upcast(ps, exp->ir_val, sym->ts, "");
            if (!_val) _val = exp->ir_val;
            klr_builder_end(&bldr, ps->scope->bb);
            klr_build_move(&bldr, sym->ir_val, _val);
        }
    } else if (sym->scope == VAR_SCOPE_FIELD) {
        NYI();
    } else {
        UNREACHABLE();
    }
}

static void emit_ir_stmt(ParserState *ps, Stmt *stmt);

static void emit_ir_fields(ParserState *ps, ParserScope *scope, Vector *fields)
{
    KlrBuilder bldr;

    VarDeclStmt *s;
    vector_foreach(s, fields) {
        ASSERT(s->kind == STMT_VAR_KIND);
        VarSymbol *var_sym = (VarSymbol *)s->sym;
        ASSERT(var_sym->scope == VAR_SCOPE_FIELD);
        Expr *e = s->exp;
        if (e) {
            emit_ir_visit_expr(ps, e);
            ASSERT(e->ir_val);
            ASSERT(var_sym->ir_val);
            KlrValue *self = METHOD_SELF;
            ASSERT(self);
            KlrValue *_val = _build_obj_intf_upcast(ps, e->ir_val, var_sym->ts, "");
            if (!_val) _val = e->ir_val;
            klr_builder_end(&bldr, scope->bb);
            klr_build_set_field(&bldr, self, var_sym->ir_val, _val);
        }
    }
}

static void emit_ir_func_decl(ParserState *ps, Stmt *stmt)
{
    FuncDeclStmt *fn = (FuncDeclStmt *)stmt;
    Symbol *sym = fn->sym;
    ParserScope *scope = enter_scope(ps, SCOPE_FUNC, 0, fn->id.name);
    KlrBasicBlock *entry = klr_append_block(sym->ir_val, "entry");
    scope->bb = entry;
    scope->sym = sym;

    if (str_equal(sym->name, "__init__")) {
        emit_ir_fields(ps, scope, fn->data);
    }

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

    if (has_specialized_meta(stmt)) {
        Vector *tp_args = get_specialized_types(stmt);
        char *mangled_name = mangle_func_name(sym->name, tp_args);
        KlrValue *new_fn = klr_specialize_func((KlrFunc *)sym->ir_val, mangled_name, tp_args);
        Symbol *new_fn_sym = stbl_get(ps->pm->stbl, mangled_name);
        ASSERT(new_fn_sym);
        new_fn_sym->ir_val = new_fn;

        if (dump_no_opt_ir_enabled()) {
            fprintf(stdout, "--- IR Dump After ir-gen(no-opt) ---\n");
            klr_print_func((KlrFunc *)new_fn, stdout);
        }
    }
}

static void emit_ir_class(ParserState *ps, Stmt *stmt)
{
    KlassDeclStmt *kls = (KlassDeclStmt *)stmt;
    Symbol *sym = kls->sym;

    ScopeKind scope_kind = (kls->kind == STMT_CLASS_KIND) ? SCOPE_CLASS : SCOPE_TRAIT;
    ParserScope *scope = enter_scope(ps, scope_kind, 0, sym->name);
    scope->sym = sym;

    Vector fields = VECTOR_INIT_PTR;

    Stmt *s;
    vector_foreach(s, kls->stmts) {
        if (!s) continue;
        if (s->kind == STMT_VAR_KIND) {
            vector_push_back(&fields, &s);
            continue;
        }
    }

    vector_foreach(s, kls->stmts) {
        if (!s || s->kind != STMT_FUNC_KIND) continue;
        FuncDeclStmt *method = (FuncDeclStmt *)s;
        Symbol *sym = method->sym;

        if (sym->flags & SYM_FLAGS_STATIC) continue;

        if (str_equal(sym->name, "__init__")) {
            method->data = &fields;
        }
        emit_ir_stmt(ps, s);
    }

    vector_fini(&fields);

    exit_scope(ps);
}

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

    FuncSymbol *fn_sym = get_current_function(ps);
    TypeSpec *fn_ret_ts = fn_sym->ret;

    KlrBuilder bldr;
    KlrValue *ret_ir_val = _build_obj_intf_upcast(ps, exp->ir_val, fn_ret_ts, "");

    klr_builder_end(&bldr, ps->scope->bb);

    if (!ret_ir_val) {
        KlrValue *cast = exp->ir_val;
        ret_ir_val = cast;
        if (cast->ts != fn_ret_ts) {
            if (!type_is_optional(fn_ret_ts)) {
                log_info("implicit cast from");
                log_type_spec(cast->ts);
                log_info("  to");
                log_type_spec(fn_ret_ts);
                ret_ir_val = klr_build_cast(&bldr, cast, fn_ret_ts, "");
            }
        }
    }

    klr_build_ret(&bldr, ret_ir_val);

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

static void build_while_cond(ParserState *ps, Expr *cond, KlrBasicBlock *bb, KlrBasicBlock *body,
                             KlrBasicBlock *end)
{
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

    KlrBuilder cond_bldr;
    klr_builder_end(&cond_bldr, bb);
    klr_build_jmp_cond(&cond_bldr, cond_val, body, end);
}

/*
cond:
    jmp_if %cond, body, end
body:
    jmp_if %cond, body, end
end:
*/
static void emit_ir_while_stmt(ParserState *ps, Stmt *stmt)
{
    KlrValue *fn = CURRENT_FUNC;

    WhileStmt *s = (WhileStmt *)stmt;
    Expr *cond = s->cond;

    KlrBasicBlock *while_cond = klr_append_block(fn, "while-cond");
    KlrBasicBlock *while_body = klr_append_block(fn, "while-body");
    KlrBasicBlock *while_end = klr_append_block(fn, "while-end");

    // current block jmp to cond block
    KlrBuilder bldr;
    klr_builder_end(&bldr, ps->scope->bb);
    klr_build_jmp(&bldr, while_cond);

    ParserScope *sc = enter_scope(ps, SCOPE_BLOCK, ONLY_BLOCK, "while-cond");
    sc->bb = while_cond;

    build_while_cond(ps, cond, sc->bb, while_body, while_end);

    exit_scope(ps);

    sc = enter_scope(ps, SCOPE_BLOCK, WHILE_BLOCK, "while-block");
    sc->bb = while_body;

    // save continue_bb and break_bb for `break` and `continue`
    sc->continue_bb = while_cond;
    sc->break_bb = while_end;

    emit_ir_visit_block(ps, s->block);

    // add jmp to cond block
    if (!block_has_terminator(sc->bb)) {
        build_while_cond(ps, cond, sc->bb, while_body, while_end);
    }

    exit_scope(ps);

    ps->scope->bb = while_end;
}

struct RangeInfo {
    KlrValue *start;
    KlrValue *end;
    KlrValue *step;
};

static void get_range_info(KlrValue *val, KlrBuilder *bldr, ParserState *ps, struct RangeInfo *out)
{
    ASSERT(type_is_range(val->ts));
    out->start = build_get_field(val->ts, "start", bldr, val, NULL, ps);
    out->end = build_get_field(val->ts, "end", bldr, val, NULL, ps);
    out->step = build_get_field(val->ts, "step", bldr, val, NULL, ps);
}

static int is_new_range(KlrInsn *insn, struct RangeInfo *out, ParserState *ps)
{
    if (insn->code != OP_BUILD_INTERN) return 0;
    if (insn->intern_tag != INTERN_RANGE) return 0;
    int num_opers = insn->num_opers;
    ASSERT(num_opers == 3);
    out->start = insn_oper_value(insn, 0);
    out->end = insn_oper_value(insn, 1);
    out->step = insn_oper_value(insn, 2);
    return 1;
}

struct SeqInfo {
    KlrValue *seq;
    KlrValue *index;
    KlrValue *len;
};

static void get_seq_info(KlrValue *val, KlrBuilder *bldr, ParserState *ps, struct SeqInfo *out)
{
    ASSERT(type_is_seq(val->ts));
    out->seq = val;
    out->index = klr_build_local_var(bldr, int64_type_spec(), "seq.index");

    if (type_is_tuple(val->ts)) {
        Symbol *_sym = get_symbol_by_id(val->ts->sym_id);
        ASSERT(_sym->kind == SYM_INSTANCE);
        InstanceSymbol *inst_sym = (InstanceSymbol *)_sym;
        int size = vector_size(inst_sym->tp_args);
        KlrValue *_len = klr_const_int(size, int64_type_spec(), MOD);
        out->len = _len;
    } else {
        out->len = klr_build_seq_len(bldr, val, "");
    }
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
    KlrModule *m = ps->pm->m;
    KlrValue *fn = CURRENT_FUNC;
    ForStmt *s = (ForStmt *)stmt;

    KlrBasicBlock *loop_header = klr_append_block(fn, "loop-header");
    KlrBasicBlock *loop_cond = klr_append_block(fn, "loop-cond");
    KlrBasicBlock *loop_body = klr_append_block(fn, "loop-body");
    KlrBasicBlock *loop_end = klr_append_block(fn, "loop-end");
    KlrValue *range_cur = NULL;
    struct RangeInfo range_info = { 0 };
    Symbol *range_sym = NULL;
    struct SeqInfo seq_info = { 0 };
    int which = 0;
#define GEN_RANGE    1
#define GEN_SEQ      2
#define GEN_ITERATOR 3

    // current block jmp to loop header
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

    // check value is Range, Tuple, Array, List or Iterator
    KlrValue *it_val = it->ir_val;
    if (klr_is_param(it_val)) {
        if (type_is_range(it_val->ts)) {
            which = GEN_RANGE;
            klr_builder_end(&bldr, sc->bb);
            get_range_info(it_val, &bldr, ps, &range_info);
        } else if (type_is_seq(it_val->ts)) {
            which = GEN_SEQ;
            klr_builder_end(&bldr, sc->bb);
            get_seq_info(it_val, &bldr, ps, &seq_info);
        } else {
            NYI();
        }
    } else if (klr_is_local(it_val)) {
        KlrInsn *local = (KlrInsn *)it_val;
        if (type_is_range(it_val->ts)) {
            which = GEN_RANGE;
            klr_builder_end(&bldr, sc->bb);
            get_range_info(it_val, &bldr, ps, &range_info);
        } else if (type_is_seq(it_val->ts)) {
            which = GEN_SEQ;
            klr_builder_end(&bldr, sc->bb);
            get_seq_info(it_val, &bldr, ps, &seq_info);
        } else {
            NYI();
        }
    } else if (klr_is_insn(it_val)) {
        KlrInsn *insn = (KlrInsn *)it_val;
        if (is_new_range(insn, &range_info, ps)) {
            which = GEN_RANGE;
            klr_erase_insn(insn);
        } else if (type_is_range(it_val->ts)) {
            which = GEN_RANGE;
            klr_builder_end(&bldr, sc->bb);
            get_range_info(it_val, &bldr, ps, &range_info);
        } else if (type_is_seq(it_val->ts)) {
            which = GEN_SEQ;
            klr_builder_end(&bldr, sc->bb);
            get_seq_info(it_val, &bldr, ps, &seq_info);
        } else {
            NYI();
        }
    } else if (klr_is_const(it_val)) {
        if (type_is_range(it_val->ts)) {
            which = GEN_RANGE;
            KlrConst *kc = (KlrConst *)it_val;
            KlrValue **items = VECTOR_RAW(kc->list, KlrValue *);
            range_info.start = items[0];
            range_info.end = items[1];
            range_info.step = items[2];
        } else if (type_is_seq(it_val->ts)) {
            which = GEN_SEQ;
            klr_builder_end(&bldr, sc->bb);
            get_seq_info(it_val, &bldr, ps, &seq_info);
        } else {
            NYI();
        }
    } else {
        UNREACHABLE();
    }

    klr_builder_end(&bldr, sc->bb);

    if (which == GEN_RANGE) {
        Symbol *sym = get_loop_range_symbol(s);
        if (sym->flags & SYM_FLAGS_MUTABLE) {
            sym->ir_val = klr_build_local_var(&bldr, sym->ts, sym->name);
        } else {
            sym->ir_val = klr_build_local(&bldr, sym->ts, sym->name);
        }
        klr_build_move(&bldr, sym->ir_val, range_info.start);
        range_cur = sym->ir_val;
    } else if (which == GEN_SEQ) {
        // v is generated in loop body, i is generated in loop header.
        // initialize index to 0
        KlrValue *zero = klr_const_int(0, int64_type_spec(), MOD);
        klr_build_move(&bldr, seq_info.index, zero);
    } else {
        NYI();
    }

    klr_build_jmp(&bldr, loop_cond);

    exit_scope(ps);

    // loop-cond
    sc = enter_scope(ps, SCOPE_BLOCK, ONLY_BLOCK, "loop-cond");
    sc->bb = loop_cond;

    if (which == GEN_RANGE) {
        klr_builder_end(&bldr, sc->bb);
        build_loop_range_cond(&bldr, range_cur, &range_info, loop_body, loop_end, ps);
    } else if (which == GEN_SEQ) {
        klr_builder_end(&bldr, sc->bb);
        KlrValue *cond = klr_build_cmpge(&bldr, seq_info.index, seq_info.len, "");
        klr_build_jmp_cond(&bldr, cond, loop_end, loop_body);
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

    if (which == GEN_SEQ) {
        klr_builder_end(&bldr, sc->bb);
        Symbol *sym = get_loop_range_symbol(s);
        if (sym->flags & SYM_FLAGS_MUTABLE) {
            sym->ir_val = klr_build_local_var(&bldr, sym->ts, sym->name);
        } else {
            sym->ir_val = klr_build_local(&bldr, sym->ts, sym->name);
        }
        KlrValue *_val = klr_build_seq_get(&bldr, seq_info.seq, seq_info.index, sym->ts, "");
        klr_build_move(&bldr, sym->ir_val, _val);
    }

    emit_ir_visit_block(ps, s->block);

    if (which == GEN_RANGE) {
        klr_builder_end(&bldr, sc->bb);
        KlrValue *tmp = klr_build_add(&bldr, range_cur, range_info.step, "");
        klr_build_move(&bldr, range_cur, tmp);
    } else if (which == GEN_SEQ) {
        klr_builder_end(&bldr, sc->bb);
        KlrValue *one = klr_const_int(1, int64_type_spec(), MOD);
        KlrValue *tmp = klr_build_add(&bldr, seq_info.index, one, "");
        klr_build_move(&bldr, seq_info.index, tmp);
    } else {
        NYI();
    }

    // add jmp to loop_body block
    if (!block_has_terminator(sc->bb)) {
        KlrBuilder _bldr;
        klr_builder_end(&_bldr, sc->bb);
        if (which == GEN_RANGE) {
            build_loop_range_cond(&bldr, range_cur, &range_info, loop_body, loop_end, ps);
        } else if (which == GEN_SEQ) {
            KlrValue *cond = klr_build_cmpge(&bldr, seq_info.index, seq_info.len, "");
            klr_build_jmp_cond(&bldr, cond, loop_end, loop_body);
        } else {
            NYI();
        }
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
    KlrValue *val = lhs->ir_val;
    if (val->kind == KLR_VALUE_GLOBAL) {
        klr_build_set_global(&bldr, val, rhs->ir_val);
    } else if (val->kind == KLR_VALUE_EXT_GLOBAL) {
        klr_build_set_global(&bldr, val, rhs->ir_val);
    } else if (val->kind == KLR_VALUE_FIELD) {
        if (lhs->kind == EXPR_DOT_KIND) {
            Expr *obj = ((DotExpr *)lhs)->lhs;
            Symbol *sym = obj->sym;
            ASSERT(sym->kind == SYM_VAR || sym->kind == SYM_SHADOW_VAR);
            klr_build_set_field(&bldr, obj->ir_val, val, rhs->ir_val);
        } else if (lhs->kind == EXPR_ID_KIND) {
            KlrValue *self = METHOD_SELF;
            klr_build_set_field(&bldr, self, val, rhs->ir_val);
        } else {
            UNREACHABLE();
        }
    } else if (val->kind == KLR_VALUE_INDEX) {
        KlrIndexInfo *index_info = (KlrIndexInfo *)val;
        if (index_info->which == KLR_SEQ_SET) {
            klr_build_seq_set(&bldr, index_info->obj, index_info->index, rhs->ir_val);
        } else if (index_info->which == KLR_MAP_SET) {
            klr_build_map_set(&bldr, index_info->obj, index_info->index, rhs->ir_val);
        } else {
            UNREACHABLE();
        }
    } else {
        klr_build_move(&bldr, val, rhs->ir_val);
    }
}

static void emit_ir_inplace_assignment(ParserState *ps, AssignStmt *s)
{
    Expr *lhs = s->lhs;
    Expr *rhs = s->rhs;
    AssignOpKind op = s->op;

    KlrValue *val = NULL;

    if (s->bin_exp) {
        emit_ir_binary(ps, s->bin_exp);
        if (!s->bin_exp->ir_val) return;
        val = s->bin_exp->ir_val;
    } else {
        KlrBuilder bldr;
        klr_builder_end(&bldr, ps->scope->bb);
        KlrValue *args[] = { lhs->ir_val, rhs->ir_val };
        val = klr_build_call(&bldr, s->fn_sym->ir_val, s->fn_sym->ret, args, 2, "");
        klr_set_loc(val, ps->filename, s->loc);
    }

    KlrBuilder bldr;
    klr_builder_end(&bldr, ps->scope->bb);
    KlrValue *var = lhs->ir_val;
    if (var->kind == KLR_VALUE_GLOBAL) {
        klr_build_set_global(&bldr, var, val);
    } else {
        klr_build_move(&bldr, var, val);
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
        emit_ir_visit_expr(ps, lhs);
        if (!lhs->ir_val || !rhs->ir_val) return;
        emit_ir_simple_assignment(ps, lhs, rhs);
    } else {
        lhs->ctx = EXPR_CTX_LOAD_STORE;
        emit_ir_visit_expr(ps, lhs);
        if (!lhs->ir_val || !rhs->ir_val) return;
        emit_ir_inplace_assignment(ps, s);
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

/*
let v = xxx
if v != null {
    ...
} else {
    ...
}
*/
static void emit_ir_if_let_stmt(ParserState *ps, Stmt *stmt)
{
    KlrValue *fn = CURRENT_FUNC;

    IfLetStmt *s = (IfLetStmt *)stmt;
    Expr *cond = s->cond;
    Symbol *var_sym = s->sym;

    cond->ctx = EXPR_CTX_LOAD;
    emit_ir_visit_expr(ps, cond);
    if (!cond->ir_val) return;

    KlrBasicBlock *if_then = klr_append_block(fn, "if-then");
    KlrBasicBlock *if_else = klr_append_block(fn, "if-else");
    KlrBasicBlock *if_end = klr_append_block(fn, "if-end");

    KlrBuilder bldr;
    klr_builder_end(&bldr, ps->scope->bb);
    var_sym->ir_val = klr_build_local(&bldr, var_sym->ts, var_sym->name);
    klr_build_move(&bldr, var_sym->ir_val, cond->ir_val);

    KlrValue *_cond = klr_build_cmpne(&bldr, cond->ir_val, klr_const_none(MOD), "");
    klr_build_jmp_cond(&bldr, _cond, if_then, if_else);

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

static void build_while_let_cond(ParserState *ps, Symbol *var_sym, Expr *cond, KlrBasicBlock *bb,
                                 KlrBasicBlock *body, KlrBasicBlock *end)
{
    cond->ctx = EXPR_CTX_LOAD;
    emit_ir_visit_expr(ps, cond);
    if (!cond->ir_val) return;

    KlrBuilder bldr;
    klr_builder_end(&bldr, bb);
    klr_build_move(&bldr, var_sym->ir_val, cond->ir_val);

    KlrValue *_cond = klr_build_cmpne(&bldr, cond->ir_val, klr_const_none(MOD), "");
    klr_build_jmp_cond(&bldr, _cond, body, end);
}

/*
let x = xxx
while x != null {
    x = xxx
}
*/
static void emit_ir_while_let_stmt(ParserState *ps, Stmt *stmt)
{
    KlrValue *fn = CURRENT_FUNC;

    WhileLetStmt *s = (WhileLetStmt *)stmt;
    Expr *cond = s->cond;
    Symbol *var_sym = s->sym;

    KlrBasicBlock *while_cond = klr_append_block(fn, "while-cond");
    KlrBasicBlock *while_body = klr_append_block(fn, "while-body");
    KlrBasicBlock *while_end = klr_append_block(fn, "while-end");

    // current block jmp to cond block
    KlrBuilder bldr;
    klr_builder_end(&bldr, ps->scope->bb);
    klr_build_jmp(&bldr, while_cond);

    ParserScope *sc = enter_scope(ps, SCOPE_BLOCK, ONLY_BLOCK, "while-cond");
    sc->bb = while_cond;

    // create local & build condition
    klr_builder_end(&bldr, sc->bb);
    var_sym->ir_val = klr_build_local_var(&bldr, var_sym->ts, var_sym->name);
    build_while_let_cond(ps, var_sym, cond, sc->bb, while_body, while_end);

    exit_scope(ps);

    sc = enter_scope(ps, SCOPE_BLOCK, WHILE_BLOCK, "while-block");
    sc->bb = while_body;

    // save continue_bb and break_bb for `break` and `continue`
    sc->continue_bb = while_cond;
    sc->break_bb = while_end;

    emit_ir_visit_block(ps, s->block);

    // add jmp to cond block
    if (!block_has_terminator(sc->bb)) {
        build_while_let_cond(ps, var_sym, cond, sc->bb, while_body, while_end);
    }

    exit_scope(ps);

    ps->scope->bb = while_end;
}

static void emit_ir_stmt(ParserState *ps, Stmt *stmt)
{
    if (!stmt) return;

    /* if errors is greater than MAX_ERRORS, stop parsing */
    if (ps->errors >= MAX_ERRORS) return;

    /* clang-format off */
    static void (*handlers[STMT_MAX_KIND])(ParserState *, Stmt *) = {
        [STMT_IMPORT_KIND]    = emit_ir_import,
        [STMT_LINK_KIND]      = emit_ir_link,
        [STMT_VAR_KIND]       = emit_ir_var_decl,
        [STMT_FUNC_KIND]      = emit_ir_func_decl,
        [STMT_CLASS_KIND]     = emit_ir_class,
        [STMT_TRAIT_KIND]     = emit_ir_trait,
        [STMT_RETURN_KIND]    = emit_ir_return,
        [STMT_ASSIGN_KIND]    = emit_ir_assignment,
        [STMT_BREAK_KIND]     = emit_ir_break,
        [STMT_CONTINUE_KIND]  = emit_ir_continue,
        [STMT_EXPR_KIND]      = emit_ir_expr,
        [STMT_BLOCK_KIND]     = emit_ir_block,
        [STMT_IF_KIND]        = emit_ir_if_stmt,
        [STMT_WHILE_KIND]     = emit_ir_while_stmt,
        [STMT_FOR_KIND]       = emit_ir_for_stmt,
        [STMT_IF_LET_KIND]    = emit_ir_if_let_stmt,
        [STMT_WHILE_LET_KIND] = emit_ir_while_let_stmt,
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

static void _add_method(KlrKlass *kls, FuncDeclStmt *meth)
{
    Ident *id = &meth->id;
    FuncSymbol *_sym = (FuncSymbol *)meth->sym;

    KlrValue *fval = klr_add_method(kls, _sym->ret, id->name);

    if (!(_sym->flags & SYM_FLAGS_STATIC)) {
        ((KlrFunc *)fval)->self = klr_func_add_param(fval, kls->ts, "self");
    }

    KlrValue *param;
    ArgInfo *arg;
    vector_foreach(arg, _sym->params) {
        param = klr_func_add_param(fval, arg->ts, arg->name);
        arg->sym->ir_val = param;
    }

    _sym->ir_val = fval;
}

static void _add_klass(KlrModule *m, KlassDeclStmt *kls)
{
    Ident *id = &kls->id;
    KlassSymbol *sym = (KlassSymbol *)kls->sym;
    KlrValue *kval = klr_add_klass(m, sym->instance_ts, id->name);

    Stmt *s;
    vector_foreach(s, kls->stmts) {
        if (!s) continue;
        if (s->kind == STMT_VAR_KIND) {
            VarDeclStmt *var = (VarDeclStmt *)s;
            Symbol *_sym = var->sym;
            ASSERT(_sym->kind == SYM_VAR);
            _sym->ir_val = klr_add_field(kval, _sym->name, _sym->ts);
        } else if (s->kind == STMT_FUNC_KIND) {
            FuncDeclStmt *fn = (FuncDeclStmt *)s;
            _add_method((KlrKlass *)kval, fn);
        } else {
            UNREACHABLE();
        }
    }

    sym->ir_val = kval;
}

static void _add_intf(KlrTrait *trait, FuncSymbol *sym)
{
    KlrValue *fval = klr_add_intf(trait, sym->ret, sym->name);
    sym->ir_val = fval;
}

static void _add_inherited_intf(KlrTrait *trait, InheritedFunc *sym)
{
    FuncSymbol *origin = sym->origin;
    KlrValue *fval = klr_add_intf(trait, origin->ret, origin->name);
    sym->ir_val = fval;
}

static void _add_trait(KlrModule *m, KlassDeclStmt *kls)
{
    Ident *id = &kls->id;
    KlassSymbol *sym = (KlassSymbol *)kls->sym;
    KlrValue *kval = klr_add_trait(m, sym->instance_ts, id->name);

    Symbol *s;
    vector_foreach(s, sym->funcs) {
        if (!s) continue;
        if (s->kind == SYM_FUNC) {
            _add_intf((KlrTrait *)kval, (FuncSymbol *)s);
        } else if (s->kind == SYM_INHERITED) {
            InheritedFunc *inherited = (InheritedFunc *)s;
            _add_inherited_intf((KlrTrait *)kval, inherited);
        } else {
            UNREACHABLE();
        }
    }

    sym->ir_val = kval;
}

void kl_gen_ir(ParserModule *pm)
{
    KlrModule *m = klr_create_module(pm->path);
    pm->m = m;

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
            } else if (s->kind == STMT_CLASS_KIND) {
                KlassDeclStmt *kls = (KlassDeclStmt *)s;
                _add_klass(m, kls);
            } else if (s->kind == STMT_TRAIT_KIND) {
                KlassDeclStmt *kls = (KlassDeclStmt *)s;
                _add_trait(m, kls);
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

        vector_foreach(s, &ps->static_methods) {
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
    } else {
        // add symbol table entry for __init__ function
        HashMap *stbl = pm->stbl;
        Symbol *sym = stbl_add_func(stbl, "__init__", no_type_spec(), NULL, 0);
        sym->ir_val = (KlrValue *)m->init;
    }
}

#ifdef __cplusplus
}
#endif

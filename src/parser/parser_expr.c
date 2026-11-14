/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "log.h"
#include "parser.h"
#include "utf8.h"
#include "version.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ConstPlaceHolder {
    HashMapEntry hnode;
    char *name;
    TypeSpec *ts;
    Literal lit;
} ConstPlaceHolder;

static HashMap cph_map;

static int _cph_equal_(void *k1, void *k2)
{
    ConstPlaceHolder *cp1 = (ConstPlaceHolder *)k1;
    ConstPlaceHolder *cp2 = (ConstPlaceHolder *)k2;
    return !strcmp(cp1->name, cp2->name);
}

void init_const_placeholder(void)
{
    hashmap_init(&cph_map, _cph_equal_);

    // __KOALA_VERSION__
    ConstPlaceHolder *cp = mm_alloc_obj(cp);
    cp->name = "__KOALA_VERSION__";
    hashmap_entry_init(cp, str_hash(cp->name));
    cp->ts = str_type_spec();
    cp->lit.which = LIT_STR;
    cp->lit.sval = KOALA_VERSION_STRING;
    cp->lit.len = strlen(KOALA_VERSION_STRING);
    hashmap_put(&cph_map, cp);
}

static ConstPlaceHolder *get_const_placeholder(char *name)
{
    ConstPlaceHolder key = { .name = name };
    hashmap_entry_init(&key, str_hash(name));
    void *ret = hashmap_get(&cph_map, &key);
    return (ConstPlaceHolder *)ret;
}

#include "literal_as_expected.h"

static int check_literal_as_expected(Literal *lit, Loc loc, TypeSpec *expected, ParserState *ps)
{
    ASSERT(lit != NULL);
    ASSERT(expected != NULL);

    if (lit->which != LIT_INT) return 0;

    int width = expected->int_flt_info.width;
    int sign = expected->int_flt_info.sign;

    if (sign) {
        switch (width) {
            case 1:
                return expect_lit_as_int8(lit, loc, ps);
            case 2:
                return expect_lit_as_int16(lit, loc, ps);
            case 4:
                return expect_lit_as_int32(lit, loc, ps);
            case 8:
                return expect_lit_as_int64(lit, loc, ps);
            default:
                return 0;
        }
    } else {
        switch (width) {
            case 1:
                return expect_lit_as_uint8(lit, loc, ps);
            case 2:
                return expect_lit_as_uint16(lit, loc, ps);
            case 4:
                return expect_lit_as_uint32(lit, loc, ps);
            case 8:
                return expect_lit_as_uint64(lit, loc, ps);
            default:
                return 0;
        }
    }
}

static int check_lambda_capture(ParserState *ps, Ident *id, Symbol *sym)
{
    if (sym->kind == SYM_SHADOW_VAR) sym = ((ShadowVarSymbol *)sym)->origin;
    if (sym->kind != SYM_VAR) return 0;
    VarSymbol *var = (VarSymbol *)sym;
    if (var->scope != VAR_SCOPE_LOCAL && var->scope != VAR_SCOPE_PARAM) return 0;

    ParserScope *decl_scope = id->scope;
    for (ParserScope *sc = ps->scope; sc && sc != decl_scope; sc = sc->next) {
        if (sc->kind == SCOPE_ANONY) {
            kl_error(id->loc, "lambda variable captures are not implemented yet");
            return 1;
        }
    }
    return 0;
}

static void parse_ident(ParserState *ps, Expr *exp)
{
    IdentExpr *id_exp = (IdentExpr *)exp;
    Ident *id = &id_exp->id;
    Symbol *sym = find_symbol(ps, id);
    if (!sym) {
        kl_error(id->loc, "'%s' is not found", id->name);
        return;
    }

    if (sym->kind == SYM_ENUM) {
        kl_error(id->loc, "enum '%s' is not a value, use '%s.MEMBER'", id->name, id->name);
        return;
    }

    if (check_lambda_capture(ps, id, sym)) return;

    if (sym->kind == SYM_VAR && ((VarSymbol *)sym)->scope == VAR_SCOPE_FIELD) {
        for (ParserScope *sc = ps->scope; sc && sc->kind != SCOPE_FUNC; sc = sc->next) {
            if (sc->kind == SCOPE_ANONY) {
                kl_error(exp->loc, "capturing 'self' in a lambda is not implemented yet");
                return;
            }
        }
    }

    if (sym->kind == SYM_VAR) {
        if (sym->status == SYM_UNRESOLVED) {
            parse_stmt(ps, sym->arg);
            exp->ts = sym->ts;
        } else if (sym->status == SYM_RESOLVING) {
            kl_error(id->loc, "circular reference detected for '%s'", id->name);
            return;
        } else {
            // do nothing
        }
    }

    if (sym->kind == SYM_FUNC) {
        FuncSymbol *fn_sym = (FuncSymbol *)sym;
        if (!fn_sym->ts) {
            log_info("update func '%s' proto", fn_sym->name);
            fn_sym->ts = func_type_spec_from_arginfo(fn_sym->params, fn_sym->ret);
        }
    }

    if (sym->kind == SYM_TYPE_PARAM) {
        log_info("resolving type parameter '%s'", sym->name);
        if (sym->ts == NULL) {
            TypeParamSymbol *tp_sym = (TypeParamSymbol *)sym;
            if (tp_sym->which == TP_INFER) {
                log_info("type parameter '%s' is inferred", tp_sym->name);
            } else if (tp_sym->which == TP_CONST) {
                log_info("type parameter '%s' is const", tp_sym->name);
            } else {
                ASSERT(tp_sym->which == TP_NORMAL);
                log_info("type parameter '%s' is normal", tp_sym->name);
            }
            TypeSpec *ret =
                generic_var_type_spec(tp_sym->name, tp_sym->index, tp_sym->id, tp_sym->owner->name);
            sym->ts = ret;
        } else {
            log_info("type parameter '%s' is already resolved", sym->name);
            ASSERT(sym->ts->kind == TYPE_GENERIC_VAR);
        }
    }

    /* a constant int variable never converts to an enum */
    if (sym->flags & SYM_FLAGS_CONST && exp->expected && !type_is_enum(exp->expected)) {
        log_info("resolving constant variable '%s' with expected type:", sym->name);
        log_type_spec(exp->expected);
        Literal *lit = ((VarSymbol *)sym)->lit;
        ASSERT(lit != NULL);
        if (check_literal_as_expected(lit, id->loc, exp->expected, ps)) {
            exp->ts = exp->expected;
        } else {
            log_info("constant variable '%s' does not match the expected type", sym->name);
            log_type_spec(exp->expected);
            exp->ts = sym->ts;
        }
    } else {
        log_info("constant variable '%s' does not have an expected type", sym->name);
        log_type_spec(sym->ts);
        exp->ts = sym->ts;
    }

    exp->sym = sym;
    log_info("ident resolved: %s", sym->name);
    log_type_spec(sym->ts);
}

static void parse_under(ParserState *ps, Expr *exp)
{
    kl_error(exp->loc, "'_' cannot be used in this context.");
}

/* Narrow the integer literal to the integer type `ts`, updating the literal in
 * place. Return 1 when the value fits and 0 when it does not. The overflow
 * diagnostic is only emitted with `report` set, so that a union can first try
 * every integer member it has. */
static int narrow_lit_int(ParserState *ps, LitExpr *lit, TypeSpec *ts, int report)
{
    int width = ts->int_flt_info.width;
    int sign = ts->int_flt_info.sign;
    __int128 val = lit->ival_128;

    // update literal integer's type as expected type
    lit->ts = ts;
    lit->sign = sign;
    lit->len = width;

    __uint128_t phys_max =
        (width == 8) ? (__uint128_t)0xFFFFFFFFFFFFFFFFULL : ((__uint128_t)1 << (width * 8)) - 1;

    if (lit->bit_mode) {
        /* non-decimal literals */
        if (val < 0) {
            __int128 min_s = -((__int128)1 << (width * 8 - 1));
            if (val < min_s || sign == 0) {
                if (!report) return 0;
                kl_error(lit->loc, "Negative hex literal %s overflows '%s%d' range", lit->orginal,
                         (sign ? "int" : "uint"), width * 8);
                return 0;
            }
        } else {
            if ((__uint128_t)val > phys_max) {
                if (!report) return 0;
                kl_error(lit->loc, "Bit pattern exceeds the bit width of '%s%d'",
                         (sign ? "int" : "uint"), width * 8);
                return 0;
            }
            /* notes: allow 0xFF to be assigned to int8 as -1.
               The pattern is read in the target's signedness: when a signed
               target has its high bit set the value turns negative, and it is
               stored sign-extended to 64 bits, exactly as the decimal path
               stores -1. A 64-bit target needs no adjustment here because its
               slot already reinterprets the pattern. */
            if (sign && val > (__int128)(phys_max >> 1)) {
                val -= (__int128)1 << (width * 8);
            }
        }
    } else {
        __int128 min_limit, max_limit;
        if (sign) {
            max_limit = (__int128)(phys_max >> 1); // 2^(n-1) - 1
            min_limit = -(max_limit + 1);          // -2^(n-1)
        } else {
            min_limit = 0;
            max_limit = (__int128)phys_max;
        }

        if (val < min_limit || val > max_limit) {
            if (!report) return 0;
            kl_error(lit->loc, "Value %s overflows '%s%d' range", lit->orginal,
                     (sign ? "int" : "uint"), width * 8);
            return 0;
        }
    }

    lit->ival = (uint64_t)val;
    lit->ival_128 = val;
    return 1;
}

static void parse_lit_int(ParserState *ps, LitExpr *lit)
{
    /* expected type from lhs */
    TypeSpec *ts = lit->expected;
    if (!ts) {
        lit->len = 8;
        lit->ival = (uint64_t)lit->ival_128;
        return;
    }

    if (type_is_optional(ts)) {
        ts = ts->opt.src;
        if (!ts) {
            lit->len = 8;
            lit->ival = (uint64_t)lit->ival_128;
            return;
        }
    }

    lit->ival = (uint64_t)lit->ival_128;

    /* an int literal converts to an enum only when it is one of the member values */
    if (type_is_enum(ts)) {
        if (!narrow_lit_int(ps, lit, int64_type_spec(), 1)) {
            /* the overflow is reported, keep the enum type to not report it again */
            lit->ts = ts;
            return;
        }
        EnumSymbol *enum_sym = get_symbol_by_id(ts->sym_id);
        EnumMemberSym *item;
        vector_foreach_ptr(item, &enum_sym->members) {
            if (item->val == (int64_t)lit->ival_128) {
                lit->ts = ts;
                return;
            }
        }
        kl_error(lit->loc, "%s is not a valid value of enum '%s'", lit->orginal, enum_sym->name);
        /* keep the enum type: the error is reported once, not again as a type mismatch */
        lit->ts = ts;
        return;
    }

    if (type_is_union(ts)) {
        /* A literal narrows to the first integer member of the union able to
           hold it: `97 in bs` finds `uint8` inside `uint8 | bytes`, since a
           union is the sanctioned way to accept heterogeneous arguments.
           A value no integer member can hold is reported against the first
           one, which is where the reader looks for the reason. */
        TypeSpec *member;
        TypeSpec *first_int = NULL;
        vector_foreach(member, ts->union_type.args) {
            if (!member || member->kind != TYPE_INT) continue;
            if (!first_int) first_int = member;
            if (narrow_lit_int(ps, lit, member, 0)) return;
        }
        if (first_int) narrow_lit_int(ps, lit, first_int, 1);
        return;
    }

    if (ts->kind != TYPE_INT) return;

    narrow_lit_int(ps, lit, ts, 1);
}

static void parse_lit_float(ParserState *ps, LitExpr *lit)
{
    /* expected type from lhs */
    TypeSpec *ts = lit->expected;
    if (!ts) {
        lit->ts = float64_type_spec();
        return;
    }

    if (type_is_optional(ts)) {
        ts = ts->opt.src;
        if (!ts) {
            lit->ts = float64_type_spec();
            return;
        }
    }

    if (type_is_union(ts)) {
        /* The same narrowing rule as integers: the first float member able to
           hold the literal wins, so a `float64 | str` union accepts `1.5`. */
        TypeSpec *member;
        vector_foreach(member, ts->union_type.args) {
            if (!member || member->kind != TYPE_FLOAT) continue;
            lit->ts = member;
            lit->len = member->int_flt_info.width;
            return;
        }
        return;
    }

    if (ts->kind != TYPE_FLOAT) return;

    int width = ts->int_flt_info.width;
    lit->ts = ts;
    lit->len = width;
}

static void parse_none(ParserState *ps, LitExpr *lit)
{
    if (lit->ctx != EXPR_CTX_LOAD) {
        kl_error(lit->loc, "none is readonly.");
        return;
    }

    // TypeSpec *expected = lit->expected;
    // if (expected && expected->kind != TYPE_OPTIONAL) {
    //     kl_error(lit->loc, "expected an optional type.");
    // }
}

static void parse_literal(ParserState *ps, Expr *exp)
{
    if (exp->ctx != EXPR_CTX_LOAD) {
        kl_error(exp->loc, "literal is readonly.");
        return;
    }

    LitExpr *lit = (LitExpr *)exp;
    switch (lit->which) {
        case LIT_EXPR_INT: {
            log_info("literal integer:%s", lit->orginal);
            parse_lit_int(ps, lit);
            ASSERT(lit->ts);
            lit->sym = get_symbol_by_id(lit->ts->sym_id);
            ASSERT(lit->sym);
            break;
        }
        case LIT_EXPR_FLT: {
            log_info("literal float: %lf", lit->fval);
            parse_lit_float(ps, lit);
            ASSERT(lit->ts);
            lit->sym = get_symbol_by_id(lit->ts->sym_id);
            ASSERT(lit->sym);
            break;
        }
        case LIT_EXPR_BOOL: {
            log_info("literal bool: %s", lit->bval ? "true" : "false");
            ASSERT(lit->ts);
            lit->sym = get_symbol_by_id(lit->ts->sym_id);
            ASSERT(lit->sym);
            break;
        }
        case LIT_EXPR_STR: {
            BUF(buf);
            escape_str(lit->sval, &buf);
            log_info("literal string '%s'", BUF_STR(buf));
            FINI_BUF(buf);
            if (check_utf8(lit->sval, lit->len) < 0) {
                kl_error(exp->loc, "invalid utf8 string");
            }
            ASSERT(lit->ts);
            lit->sym = get_symbol_by_id(lit->ts->sym_id);
            ASSERT(lit->sym);
            break;
        }
        case LIT_EXPR_CHAR: {
            log_info("literal char '%s'", lit->sval);
            if (lit->len != 1) {
                kl_error(exp->loc, "invalid char literal, expected one char, but got %d", lit->len);
                return;
            }
            char ch = lit->sval[0];
            lit->ival_128 = (uint64_t)ch;
            lit->ival = (uint64_t)ch;
            lit->sign = 0;
            lit->len = 1;
            lit->which = LIT_EXPR_INT;
            ASSERT(lit->ts);
            lit->sym = get_symbol_by_id(lit->ts->sym_id);
            ASSERT(lit->sym);
            break;
        }
        case LIT_EXPR_BYTES: {
            log_info("literal bytes");
            ASSERT(lit->ts);
            lit->which = LIT_EXPR_BYTES;
            lit->sym = get_symbol_by_id(lit->ts->sym_id);
            ASSERT(lit->sym);
            break;
        }
        case LIT_EXPR_NONE: {
            log_info("literal nil");
            parse_none(ps, lit);
            break;
        }
        default: {
            UNREACHABLE();
            break;
        }
    }
}

static Symbol *get_current_klass(ParserState *ps)
{
    ParserScope *sc = ps->scope;
    while (sc) {
        if (sc->kind == SCOPE_CLASS) {
            return sc->sym;
        }
        sc = sc->next;
    }
    return NULL;
}

static void parse_self(ParserState *ps, Expr *exp)
{
    ASSERT(exp->ctx == EXPR_CTX_LOAD);

    FuncSymbol *fn_sym = get_current_function(ps);
    if (fn_sym && (fn_sym->flags & SYM_FLAGS_STATIC)) {
        kl_error(exp->loc, "'self' cannot be used in static method '%s'", fn_sym->name);
        return;
    }

    for (ParserScope *sc = ps->scope; sc && sc->kind != SCOPE_FUNC; sc = sc->next) {
        if (sc->kind == SCOPE_ANONY) {
            kl_error(exp->loc, "capturing 'self' in a lambda is not implemented yet");
            return;
        }
    }

    Symbol *sym = get_current_klass(ps);
    if (!sym || sym->kind != SYM_CLASS) {
        kl_error(exp->loc, "'self' can only be used in class.");
        return;
    }

    exp->ts = ((KlassSymbol *)sym)->instance_ts;
    exp->sym = sym;
    log_info("'self' resolved as class '%s'", sym->name);
    log_type_spec(exp->ts);
}

/* The type arguments of `ts` if it is an instance of `origin`, also an open one like list[T]. */
static Vector *instance_tp_args(TypeSpec *ts, Symbol *origin)
{
    if (!origin) return NULL;

    if (type_is_generic_ref(ts)) {
        if (strcmp(ts->generic_ref.name, origin->name)) return NULL;
        if (strcmp(ts->generic_ref.pkg, origin->path)) return NULL;
        return ts->generic_ref.args;
    }

    if (!type_is_klass(ts)) return NULL;

    Symbol *sym = get_symbol_by_id(ts->sym_id);
    if (!sym || sym->kind != SYM_INSTANCE) return NULL;

    InstanceSymbol *inst_sym = (InstanceSymbol *)sym;
    if (inst_sym->origin != origin) return NULL;

    return inst_sym->tp_args;
}

/*
 * The element type an empty literal takes from an expected Iterable[X], Sequence[X] or an instance
 * of the literal's own class `own_origin`, or NULL.
 */
static TypeSpec *empty_literal_elem_type(ParserState *ps, TypeSpec *expected, Symbol *own_origin)
{
    if (expected && type_is_optional(expected)) expected = expected->opt.src;
    if (!expected) return NULL;

    Vector *args = instance_tp_args(expected, own_origin);
    if (!args) args = instance_tp_args(expected, stbl_get(ps->pm->builtin, "Iterable"));
    if (!args) args = instance_tp_args(expected, stbl_get(ps->pm->builtin, "Sequence"));
    if (!args) return NULL;

    return vector_get(args, 0);
}

/*
 * The element type a non-empty list literal takes from an expected list[X], Iterable[X] or
 * Sequence[X]: X, when every element can be assigned to it, or NULL. The literal is new and
 * nothing else refers to it, so `["a", 1]` can be a list[any?] without loosening invariance.
 */
static TypeSpec *list_elem_as_expected(ParserState *ps, TypeSpec *expected, Vector *elems)
{
    Symbol *origin = stbl_get(ps->pm->builtin, "list");
    TypeSpec *elem_ts = empty_literal_elem_type(ps, expected, origin);
    if (!elem_ts) return NULL;

    // a type variable is inferred from the elements, not given to them
    TypeSpec *src = elem_ts;
    if (type_is_optional(src) && src->opt.src) src = src->opt.src;
    if (type_is_generic_var(src) || type_is_generic_ref(src)) return NULL;

    Expr *e;
    vector_foreach(e, elems) {
        if (!e->ts) return NULL;
        // a nested literal takes the element type too: `[["a", 1]]` as a list[list[any?]]
        parse_literal_as_expected(ps, e, elem_ts);
        if (!type_spec_compatible(elem_ts, e->ts)) return NULL;
    }
    return elem_ts;
}

static void parse_list(ParserState *ps, Expr *exp)
{
    ListExpr *list_exp = (ListExpr *)exp;

    Symbol *origin = stbl_get(ps->pm->builtin, "list");

    /* a lambda element takes its parameter types from the expected element type,
       so it must be known before the elements are visited */
    TypeSpec *expected_elem = empty_literal_elem_type(ps, exp->expected, origin);
    if (expected_elem && (type_is_generic_var(expected_elem) || type_is_generic_ref(expected_elem)))
        expected_elem = NULL;

    Vector *tp_args = vector_create_ptr();
    Expr *e;
    vector_foreach(e, list_exp->vec) {
        e->ctx = EXPR_CTX_LOAD;
        /* a lambda element takes its parameter types from the expected element type,
           and a nested literal takes it too so it can pass it on to its own elements */
        int nested_literal =
            e->kind == EXPR_LIST_KIND || e->kind == EXPR_MAP_KIND || e->kind == EXPR_TUPLE_KIND;
        if (expected_elem && (lambda_needs_infer(e) || nested_literal)) e->expected = expected_elem;
        parser_visit_expr(ps, e);
        if (!e->ts) return;
        vector_push_back(tp_args, &e->ts);
    }

    TypeSpec *infer_ts = NULL;
    if (vector_empty(tp_args)) {
        infer_ts = empty_literal_elem_type(ps, exp->expected, origin);
    } else {
        infer_ts = list_elem_as_expected(ps, exp->expected, list_exp->vec);
    }
    if (!infer_ts) infer_ts = find_lub(tp_args);
    log_info("infer list type:");
    log_type_spec(infer_ts);

    vector_clear(tp_args);
    vector_push_back(tp_args, &infer_ts);

    InstanceSymbol *inst_sym = find_or_add_instance(ps->pm->stbl, origin, tp_args, ps->pm);
    inst_sym->arg = infer_ts; // save infered list type for later use

    exp->ts = inst_sym->instance_ts;
    exp->sym = (Symbol *)inst_sym;
    vector_destroy(tp_args);

    log_info("list type resolved:");
    log_type_spec(exp->ts);
}

static void parse_tuple(ParserState *ps, Expr *exp)
{
    TupleExpr *tuple_exp = (TupleExpr *)exp;

    if (exp->sym) {
        log_info("tuple type already resolved:");
        log_type_spec(exp->ts);
        return;
    }

    /* expected tuple type drives the types of the elements,
       e.g. 'return (nil, 1)' for '(int?, int)' */
    TypeSpec *tuple_ts = exp->expected;
    if (tuple_ts && type_is_optional(tuple_ts)) {
        tuple_ts = tuple_ts->opt.src;
    }

    Vector *elem_types = NULL;
    if (tuple_ts && type_is_tuple(tuple_ts)) {
        Symbol *tuple_sym = get_symbol_by_id(tuple_ts->sym_id);
        if (tuple_sym && tuple_sym->kind == SYM_INSTANCE) {
            Vector *args = ((InstanceSymbol *)tuple_sym)->tp_args;
            if (vector_size(args) == vector_size(tuple_exp->vec)) elem_types = args;
        }
    }

    Vector *tp_args = vector_create_ptr();
    Expr *e;
    int i = 0;
    vector_foreach(e, tuple_exp->vec) {
        if (!e->sym) {
            /* an expected type given to the element itself is kept */
            if (elem_types && !e->expected) e->expected = vector_get(elem_types, i);
            e->ctx = EXPR_CTX_LOAD;
            parser_visit_expr(ps, e);
            if (!e->ts) return;
        }

        /* nil or T for an expected T?: the element takes the optional type,
           its value needs no conversion */
        TypeSpec *ts = e->ts;
        TypeSpec *elem_ts = elem_types ? vector_get(elem_types, i) : NULL;
        if (elem_ts && type_is_optional(elem_ts)) {
            int is_nil = type_is_optional(ts) && !ts->opt.src;
            if (is_nil || elem_ts->opt.src == ts) ts = elem_ts;
        }
        vector_push_back(tp_args, &ts);
        i++;
    }

    if (vector_empty(tp_args)) {
        // An empty tuple (also synthesized for a variadic parameter that
        // receives zero variadic arguments) is typed as tuple<any>: without a
        // type argument, the mangled instance name would be the bare "tuple",
        // which collides with the builtin tuple class in symbol lookups.
        // An expected Iterable[X] or Sequence[X] gives it the element type X.
        TypeSpec *elem_ts = empty_literal_elem_type(ps, tuple_ts, NULL);
        if (!elem_ts) elem_ts = any_type_spec();
        vector_push_back(tp_args, &elem_ts);
    }

    TypeSpec *infer_ts = find_lub(tp_args);
    log_info("infer tuple type:");
    log_type_spec(infer_ts);

    Symbol *origin = stbl_get(ps->pm->builtin, "tuple");
    InstanceSymbol *inst_sym = find_or_add_instance(ps->pm->stbl, origin, tp_args, ps->pm);
    inst_sym->arg = infer_ts; // save infered tuple type for later use

    exp->ts = inst_sym->instance_ts;
    exp->sym = (Symbol *)inst_sym;
    log_info("tuple type resolved:");
    log_type_spec(exp->ts);
}

/*
 * Parse a literal map, e.g. {1: "one", 2: "two"}.
 *
 * The map literal is always a 'dict' instance. Its key/value types come from
 * the expected type when we are in a typed context (variable initializer,
 * return statement, ...); otherwise they are inferred from the entries, just
 * like list/tuple literals do.
 */
static void parse_map(ParserState *ps, Expr *exp)
{
    MapExpr *map_exp = (MapExpr *)exp;

    if (exp->sym) {
        log_info("map type already resolved:");
        log_type_spec(exp->ts);
        return;
    }

    /* expected map type drives the key/value types of the entries */
    TypeSpec *map_ts = exp->expected;
    if (map_ts && type_is_optional(map_ts)) {
        map_ts = map_ts->opt.src;
    }

    TypeSpec *key_ts = NULL;
    TypeSpec *val_ts = NULL;

    if (map_ts && type_is_map(map_ts)) {
        Symbol *map_sym = get_symbol_by_id(map_ts->sym_id);
        if (map_sym && map_sym->kind == SYM_INSTANCE) {
            Vector *map_args = ((InstanceSymbol *)map_sym)->tp_args;
            if (vector_size(map_args) == 2) {
                key_ts = vector_get(map_args, 0);
                val_ts = vector_get(map_args, 1);
            }
        }
    }

    /* `{:}` for an open dict[K, V] in a generic body */
    if (map_ts && !key_ts && vector_empty(map_exp->vec)) {
        Vector *map_args = instance_tp_args(map_ts, stbl_get(ps->pm->builtin, "dict"));
        if (vector_size(map_args) == 2) {
            key_ts = vector_get(map_args, 0);
            val_ts = vector_get(map_args, 1);
        }
    }

    Vector *key_types = vector_create_ptr();
    Vector *val_types = vector_create_ptr();
    int has_error = 0;

    Expr *e;
    vector_foreach(e, map_exp->vec) {
        if (!e) continue;

        /* a map entry inherits the expected type of its enclosing map */
        e->expected = map_ts;
        e->ctx = EXPR_CTX_LOAD;
        parser_visit_expr(ps, e);
        if (!e->ts) {
            /* the error has been reported, keep checking the other entries */
            has_error = 1;
            continue;
        }

        MapEntryExpr *entry = (MapEntryExpr *)e;
        vector_push_back(key_types, &entry->key->ts);
        vector_push_back(val_types, &entry->val->ts);
    }

    if (has_error) {
        vector_destroy(key_types);
        vector_destroy(val_types);
        return;
    }

    /* no expected map type, infer key/value types from the entries */
    if (!key_ts) key_ts = find_lub(key_types);
    if (!val_ts) val_ts = find_lub(val_types);

    vector_destroy(key_types);
    vector_destroy(val_types);

    log_info("infer map key type:");
    log_type_spec(key_ts);
    log_info("infer map value type:");
    log_type_spec(val_ts);

    Vector *tp_args = vector_create_ptr();
    vector_push_back(tp_args, &key_ts);
    vector_push_back(tp_args, &val_ts);

    Symbol *origin = stbl_get(ps->pm->builtin, "dict");
    if (!origin) {
        kl_error(exp->loc, "builtin class 'dict' is not found.");
        vector_destroy(tp_args);
        return;
    }

    InstanceSymbol *inst_sym = find_or_add_instance(ps->pm->stbl, origin, tp_args, ps->pm);
    vector_destroy(tp_args);

    exp->ts = inst_sym->instance_ts;
    exp->sym = (Symbol *)inst_sym;

    log_info("map type resolved:");
    log_type_spec(exp->ts);
}

/*
 * Parse one key:value pair of a map literal.
 *
 * The expected type of an entry is the type of its enclosing map, set by
 * parse_map. Keys and values are typed against its type arguments so that
 * literals adapt to the declared type (e.g. map[int8, str]).
 */
static void parse_map_entry(ParserState *ps, Expr *exp)
{
    MapEntryExpr *entry = (MapEntryExpr *)exp;

    TypeSpec *map_ts = exp->expected;
    if (map_ts && type_is_optional(map_ts)) {
        map_ts = map_ts->opt.src;
    }

    TypeSpec *key_ts = NULL;
    TypeSpec *val_ts = NULL;

    if (map_ts && type_is_map(map_ts)) {
        Symbol *map_sym = get_symbol_by_id(map_ts->sym_id);
        if (map_sym && map_sym->kind == SYM_INSTANCE) {
            Vector *map_args = ((InstanceSymbol *)map_sym)->tp_args;
            if (vector_size(map_args) == 2) {
                key_ts = vector_get(map_args, 0);
                val_ts = vector_get(map_args, 1);
            }
        }
    }

    Expr *key = entry->key;
    key->ctx = EXPR_CTX_LOAD;
    key->expected = key_ts;
    parser_visit_expr(ps, key);

    Expr *val = entry->val;
    val->ctx = EXPR_CTX_LOAD;
    val->expected = val_ts;
    parser_visit_expr(ps, val);

    if (!key->ts || !val->ts) return;

    /* report both the key and the value so one entry can yield two errors */
    int compatible = 1;
    if (key_ts && !type_spec_compatible(key_ts, key->ts)) {
        kl_error_incompatible_type(key->loc, key_ts, key->ts);
        compatible = 0;
    }

    if (val_ts && !type_spec_compatible(val_ts, val->ts)) {
        kl_error_incompatible_type(val->loc, val_ts, val->ts);
        compatible = 0;
    }

    if (!compatible) return;

    /*
     * A map entry is not a standalone value, it only exists inside a map
     * literal. Take the value's type so visiting it as an expression does
     * not report a missing type.
     */
    exp->ts = val->ts;
}

/*
 * A non-empty dict literal of `entries` as an expected dict[K, V]: it becomes a dict[K, V] when
 * every key can be assigned to K and every value to V. As for a list, the literal is new.
 */
static void map_as_expected(ParserState *ps, Expr *exp, TypeSpec *expected, Vector *entries)
{
    if (expected && type_is_optional(expected)) expected = expected->opt.src;
    if (!expected || !type_is_map(expected)) return;

    Symbol *map_sym = get_symbol_by_id(expected->sym_id);
    if (!map_sym || map_sym->kind != SYM_INSTANCE) return;
    Vector *map_args = ((InstanceSymbol *)map_sym)->tp_args;
    if (vector_size(map_args) != 2) return;

    TypeSpec *key_ts = vector_get(map_args, 0);
    TypeSpec *val_ts = vector_get(map_args, 1);
    if (type_is_generic_var(key_ts) || type_is_generic_var(val_ts)) return;

    Expr *e;
    vector_foreach(e, entries) {
        if (!e) continue;
        MapEntryExpr *entry = (MapEntryExpr *)e;
        if (!entry->key->ts || !type_spec_compatible(key_ts, entry->key->ts)) return;
        if (!entry->val->ts) return;
        parse_literal_as_expected(ps, entry->val, val_ts);
        if (!type_spec_compatible(val_ts, entry->val->ts)) return;
    }

    exp->ts = expected;
    exp->sym = map_sym;
}

/*
 * An argument or the right side of an assignment is parsed before its expected type is known:
 * give a literal that type now. `()`, `[]` and `{:}` have no element to give their type, so they
 * are parsed again with it. A non-empty list or dict literal takes the expected type when its
 * elements fit it, e.g. `["a", 1]` passed as a list[any?].
 */
int check_int_to_enum(ParserState *ps, Loc loc, TypeSpec *dst, TypeSpec *src)
{
    if (!dst || !src || !type_is_enum(dst) || src->kind != TYPE_INT) return 0;
    Symbol *enum_sym = get_symbol_by_id(dst->sym_id);
    kl_error(loc, "only an int literal can be converted to enum '%s'.", enum_sym->name);
    return 1;
}

void parse_literal_as_expected(ParserState *ps, Expr *exp, TypeSpec *expected)
{
    Vector *vec = NULL;
    if (exp->kind == EXPR_TUPLE_KIND) {
        vec = ((TupleExpr *)exp)->vec;
    } else if (exp->kind == EXPR_LIST_KIND) {
        vec = ((ListExpr *)exp)->vec;
    } else if (exp->kind == EXPR_MAP_KIND) {
        vec = ((MapExpr *)exp)->vec;
    } else {
        return;
    }

    if (vector_empty(vec)) {
        exp->ctx = EXPR_CTX_LOAD;
        exp->expected = expected;
        exp->sym = NULL;
        parser_visit_expr(ps, exp);
        return;
    }

    // a literal that already fits keeps its type
    if (!exp->ts || type_spec_compatible(expected, exp->ts)) return;

    if (exp->kind == EXPR_LIST_KIND) {
        TypeSpec *elem_ts = list_elem_as_expected(ps, expected, vec);
        if (!elem_ts) return;

        Vector *tp_args = vector_create_ptr();
        vector_push_back(tp_args, &elem_ts);
        Symbol *origin = stbl_get(ps->pm->builtin, "list");
        InstanceSymbol *inst_sym = find_or_add_instance(ps->pm->stbl, origin, tp_args, ps->pm);
        inst_sym->arg = elem_ts;
        vector_destroy(tp_args);

        exp->ts = inst_sym->instance_ts;
        exp->sym = (Symbol *)inst_sym;
    } else if (exp->kind == EXPR_MAP_KIND) {
        map_as_expected(ps, exp, expected, vec);
    }
}

int lambda_needs_infer(Expr *exp)
{
    if (!exp || exp->kind != EXPR_ANONY_KIND || exp->ts) return 0;

    LambdaExpr *lambda = (LambdaExpr *)exp;
    ParamDecl *param;
    vector_foreach(param, lambda->params) {
        if (param && !param->type) return 1;
    }
    return 0;
}

/* a composite literal forwards its expected type down to its own elements, so an
   untyped lambda nested anywhere inside one still needs the enclosing construct
   to defer it until the expected type is known */
int expr_needs_infer(Expr *exp)
{
    if (!exp || exp->ts) return 0;
    if (lambda_needs_infer(exp)) return 1;

    Expr *e;
    switch (exp->kind) {
        case EXPR_LIST_KIND:
            vector_foreach(e, ((ListExpr *)exp)->vec) {
                if (expr_needs_infer(e)) return 1;
            }
            return 0;

        case EXPR_TUPLE_KIND:
            vector_foreach(e, ((TupleExpr *)exp)->vec) {
                if (expr_needs_infer(e)) return 1;
            }
            return 0;

        case EXPR_MAP_KIND:
            vector_foreach(e, ((MapExpr *)exp)->vec) {
                if (!e) continue;
                MapEntryExpr *entry = (MapEntryExpr *)e;
                if (expr_needs_infer(entry->key) || expr_needs_infer(entry->val)) return 1;
            }
            return 0;

        default:
            return 0;
    }
}

void resolve_as_expected(ParserState *ps, Expr *exp, TypeSpec *expected)
{
    if (!expr_needs_infer(exp)) return;

    exp->ctx = EXPR_CTX_LOAD;
    exp->expected = expected;
    parser_visit_expr(ps, exp);
}

static void check_kw_arg(ParserState *ps, Vector *params, KeyWordExpr *kw, int i__)
{
    log_info("check keyword argument: '%s' at %d", kw->key.name, i__);

    int param_size = vector_size(params);
    for (int i = i__; i < param_size; i++) {
        ArgInfo *arg = vector_get(params, i);
        if (!strcmp(arg->name, kw->key.name)) {
            parse_literal_as_expected(ps, kw->value, arg->ts);
            if (check_int_to_enum(ps, kw->loc, arg->ts, kw->value->ts)) return;
            if (!type_spec_compatible(arg->ts, kw->value->ts)) {
                kl_error_incompatible_type(kw->loc, arg->ts, kw->value->ts);
            }
            return;
        }
    }

    kl_error(kw->loc, "unexpected keyword argument: '%s'", kw->key.name);
}

static void check_dfl_param(ParserState *ps, Vector *params, Vector *exprs, int i__, int j__)
{
    int expr_size = vector_size(exprs);
    for (int j = j__; j < expr_size; j++) {
        Expr *e = vector_get(exprs, j);
        ArgInfo *arg = vector_get(params, i__);
        if (!e) {
            // use default values for the rest parameters
            log_info("kw-param: %s, no value passed, skip the rest.", arg->name);
            return;
        }

        if (e->kind == EXPR_KW_KIND) {
            KeyWordExpr *kw = (KeyWordExpr *)e;
            log_info("kw-arg: '%s', check kw-arg", kw->key.name);
            check_kw_arg(ps, params, kw, i__);
        } else {
            if (!arg) {
                kl_error(e->loc, "too many positional arguments in function call.");
                return;
            }
            log_info("kw-param: '%s', check value passed by positional argument", arg->name);
            parse_literal_as_expected(ps, e, arg->ts);
            if (check_int_to_enum(ps, e->loc, arg->ts, e->ts)) {
                // reported
            } else if (!type_spec_compatible(arg->ts, e->ts)) {
                kl_error_incompatible_type(e->loc, arg->ts, e->ts);
            }
            ++i__;
        }
    }
}

static void check_valist_arg(ParserState *ps, TypeSpec *expected, Vector *exprs, int i__,
                             int *next_j)
{
    TypeSpec *arg_ts;
    int expr_size = vector_size(exprs);
    for (int j = i__; j < expr_size; j++) {
        Expr *e = vector_get(exprs, j);

        if (e->kind == EXPR_KW_KIND) {
            *next_j = j;
            return;
        }

        log_info("var-arg: check value passed by positional argument");

        resolve_as_expected(ps, e, expected);
        if (!e->ts) return;

        arg_ts = e->ts;

        if ((e->kind == EXPR_BANG_KIND) && (match_sequence(e->ts, NULL, &arg_ts))) {
            log_info("var-arg: arg is unpacked by '!', check unpacked type:");
            log_type_spec(arg_ts);
        }

        if (!type_spec_compatible(expected, arg_ts)) {
            kl_error_incompatible_type(e->loc, expected, arg_ts);
        }
    }
    *next_j = expr_size;
}

/*
 * `check_call_args` walks ArgInfo, but a proto type only carries the parameter
 * types, so wrap them before the call checks run on a call through a function
 * value, e.g. `cb(v)`. mm_alloc zeroes the ArgInfo: no default value, no var-arg.
 */
static Vector *arg_infos_from_arg_types(Vector *arg_types)
{
    Vector *params = vector_create_ptr();

    TypeSpec *ts;
    vector_foreach(ts, arg_types) {
        ArgInfo *arg = mm_alloc_obj(arg);
        arg->ts = ts;
        vector_push_back(params, &arg);
    }

    return params;
}

/*
 * Resolves an argument against its parameter and checks the type. Shared by the
 * positional and the keyword path so both report the same diagnostic.
 * Returns 0 when compatible, 1 when an error was reported, -1 when the argument
 * could not be resolved at all and the caller has to stop.
 */
static int check_arg_type(ParserState *ps, ArgInfo *param, Expr *e)
{
    if (expr_needs_infer(e)) {
        log_info("arg needs an expected type, resolve it with the parameter type:");
        log_type_spec(param->ts);
        resolve_as_expected(ps, e, param->ts);
        if (!e->ts) return -1;
    } else if (e->kind == EXPR_LITERAL_KIND) {
        log_info("arg is literal, parse literal expr again with expected type:");
        log_type_spec(e->ts);
        e->ctx = EXPR_CTX_LOAD;
        e->expected = param->ts;
        parser_visit_expr(ps, e);
        if (!e->ts) return -1;
    } else if (e->kind == EXPR_ID_KIND && e->sym && e->sym->kind == SYM_VAR) {
        VarSymbol *var_sym = (VarSymbol *)e->sym;
        if (var_sym->flags & SYM_FLAGS_CONST) {
            log_info("arg is a constant variable");
            e->ctx = EXPR_CTX_LOAD;
            e->expected = param->ts;
            parser_visit_expr(ps, e);
            if (!e->ts) return -1;
        }
    } else {
        parse_literal_as_expected(ps, e, param->ts);
        if (!e->ts) return -1;
    }

    /* the parameter already errored at its declaration; no type to check against */
    if (!param->ts) return 0;

    if (check_int_to_enum(ps, e->loc, param->ts, e->ts)) return 1;
    if (!type_spec_compatible(param->ts, e->ts)) {
        kl_error_incompatible_type(e->loc, param->ts, e->ts);
        return 1;
    }
    return 0;
}

/* Number of leading positional arguments; the grammar puts every keyword after them. */
static int count_positional_args(Vector *exprs)
{
    int n = 0;
    int size = vector_size(exprs);
    while (n < size) {
        Expr *e = vector_get(exprs, n);
        if (e->kind == EXPR_KW_KIND) break;
        n++;
    }
    return n;
}

/*
 * The argument that fills parameter `index`, for the passes that run before the
 * call arguments are reordered. The positional prefix lines up with the leading
 * parameters, but a keyword suffix may come in any order, so once the argument at
 * `index` is a keyword the real one has to be found by name.
 */
Expr *call_arg_for_param(Vector *call_args, int index, const char *name)
{
    Expr *exp = vector_get(call_args, index);
    if (!exp || exp->kind != EXPR_KW_KIND) return exp;

    int size = vector_size(call_args);
    for (int j = 0; j < size; j++) {
        Expr *e = vector_get(call_args, j);
        if (e->kind != EXPR_KW_KIND) continue;
        KeyWordExpr *kw = (KeyWordExpr *)e;
        if (str_equal(kw->key.name, name)) return kw->value;
    }
    return NULL;
}

static KeyWordExpr *find_kw_arg(Vector *exprs, int npos, const char *name)
{
    int size = vector_size(exprs);
    for (int j = npos; j < size; j++) {
        Expr *e = vector_get(exprs, j);
        KeyWordExpr *kw = (KeyWordExpr *)e;
        if (str_equal(kw->key.name, name)) return kw;
    }
    return NULL;
}

static int find_param_by_name(Vector *params, const char *name)
{
    int size = vector_size(params);
    for (int i = 0; i < size; i++) {
        ArgInfo *arg = vector_get(params, i);
        if (arg->name && str_equal(arg->name, name)) return i;
    }
    return -1;
}

/*
 * A call with a keyword suffix. The positional prefix fills the leading parameters
 * in order, the keywords fill the rest by name and may come in any order. Every
 * required parameter has to end up covered by one or the other, and no parameter
 * may be filled twice.
 *
 * Parameters are laid out as [required...] [va_list?] [defaulted...], so the
 * va_list, when present, sits at index `nreq` and swallows the whole positional
 * tail: a defaulted parameter after it can only be passed by keyword.
 */
static void check_call_args_kw(Vector *params, Vector *exprs, ParserState *ps, Loc fn_loc, int npos)
{
    int param_size = vector_size(params);
    int expr_size = vector_size(exprs);

    // A function value carries only the parameter types, so there is no name to match.
    ArgInfo *p;
    vector_foreach(p, params) {
        if (p && !p->name) {
            Expr *e = vector_get(exprs, npos);
            kl_error(e->loc, "keyword argument is not allowed in a function value call");
            return;
        }
    }

    int nreq = 0;
    int ndfl = 0;
    int valist_index = -1;
    for (int i = 0; i < param_size; i++) {
        ArgInfo *arg = vector_get(params, i);
        if (arg->has_dfl_val) {
            ndfl++;
        } else if (arg->ts && type_is_valist(arg->ts)) {
            valist_index = i;
        } else {
            nreq++;
        }
    }

    if (valist_index < 0 && npos > nreq + ndfl) {
        Expr *e = vector_get(exprs, nreq + ndfl);
        kl_error(e->loc, "too many positional arguments in function call.");
        return;
    }

    // The positional prefix is checked first so that a bad value is reported even
    // when a later keyword argument turns out to be unknown.
    int pos_end = (valist_index >= 0) ? valist_index : npos;
    if (pos_end > npos) pos_end = npos;
    for (int i = 0; i < pos_end; i++) {
        Expr *e = vector_get(exprs, i);
        ArgInfo *arg = vector_get(params, i);
        if (check_arg_type(ps, arg, e) < 0) return;
    }

    // A parameter is filled positionally when it precedes the va_list and the
    // positional prefix reaches it.
    for (int j = npos; j < expr_size; j++) {
        Expr *e = vector_get(exprs, j);
        KeyWordExpr *kw = (KeyWordExpr *)e;
        int pi = find_param_by_name(params, kw->key.name);
        if (pi < 0 || pi == valist_index) {
            kl_error(kw->loc, "unexpected keyword argument: '%s'", kw->key.name);
            return;
        }
        if (pi < npos && (valist_index < 0 || pi < valist_index)) {
            kl_error(kw->loc, "duplicate argument for parameter '%s'", kw->key.name);
            return;
        }
    }

    for (int i = 0; i < nreq; i++) {
        if (i < npos) continue;
        ArgInfo *arg = vector_get(params, i);
        if (find_kw_arg(exprs, npos, arg->name)) continue;
        kl_error(fn_loc, "missing argument for parameter '%s'", arg->name);
        return;
    }

    for (int j = npos; j < expr_size; j++) {
        Expr *e = vector_get(exprs, j);
        KeyWordExpr *kw = (KeyWordExpr *)e;
        ArgInfo *arg = vector_get(params, find_param_by_name(params, kw->key.name));
        if (check_arg_type(ps, arg, kw->value) < 0) return;
    }

    if (valist_index >= 0 && npos > valist_index) {
        ArgInfo *arg = vector_get(params, valist_index);
        TypeSpec *src = arg->ts->va_list.src;
        if (!type_is_any(src)) {
            log_info("param '%s' is var-arg, check the positional tail", arg->name);
            int next_j = 0;
            check_valist_arg(ps, src, exprs, valist_index, &next_j);
        }
    }
}

static void check_call_args(Vector *params, Vector *exprs, ParserState *ps, Loc fn_loc)
{
    Expr *e1;
    vector_foreach(e1, exprs) {
        Expr *e2;
        vector_foreach(e2, exprs) {
            if (e1 == e2) continue;
            if (e1->kind == EXPR_KW_KIND && e2->kind == EXPR_KW_KIND) {
                KeyWordExpr *kw1 = (KeyWordExpr *)e1;
                KeyWordExpr *kw2 = (KeyWordExpr *)e2;
                if (strcmp(kw1->key.name, kw2->key.name) == 0) {
                    kl_error(kw2->loc, "duplicate keyword argument: '%s'", kw2->key.name);
                    return;
                }
            }
        }
    }

    int npos = count_positional_args(exprs);
    if (npos < vector_size(exprs)) {
        check_call_args_kw(params, exprs, ps, fn_loc, npos);
        return;
    }

    int param_size = vector_size(params);
    int i__ = 0;
    int j__ = 0;
    while (i__ < param_size) {
        ArgInfo *arg = vector_get(params, i__);
        // 1. kw-param: pass value with keyword argument, e.g. foo(x=10)
        // 2. pass value only, e.g. foo(10)
        // 3. skip it, e.g. foo() for foo(x=10)
        if (arg->has_dfl_val) {
            check_dfl_param(ps, params, exprs, i__, j__);
            return;
        }

        Expr *e = vector_get(exprs, j__++);

        // a NULL ts means the parameter already errored at its declaration
        if (!arg->ts || !type_is_valist(arg->ts)) {
            // required param, caller must pass value
            if (!e) {
                kl_error(fn_loc, "too few arguments in function call.");
                return;
            }

            log_info("param '%s' is positional argument", arg->name);

            if (check_arg_type(ps, arg, e) < 0) return;

            ++i__;
        } else {
            // no more arguments passed.
            if (!e) return;

            if (e->kind == EXPR_KW_KIND) {
                log_info("skip var-arg param '%s' check and go to next kw-arg check", arg->name);
                KeyWordExpr *kw = (KeyWordExpr *)e;
                check_kw_arg(ps, params, kw, i__ + 1);
            } else {
                // var-arg, caller can pass 0 or more values
                TypeSpec *src = arg->ts->va_list.src;
                if (type_is_any(src)) {
                    log_info("param '%s' is var-arg of type 'any', no check for var-arg type",
                             arg->name);
                } else {
                    int next_j = 0;
                    check_valist_arg(ps, src, exprs, i__, &next_j);
                    ++i__;
                    j__ = next_j;
                }
            }
        }
    }

    // all parameters are matched, an argument left over is an extra one
    if (j__ < vector_size(exprs)) {
        Expr *e = vector_get(exprs, j__);
        kl_error(e->loc, "too many arguments in function call.");
    }
}

static void parse_type(ParserState *ps, Expr *exp)
{
    TypeSpec *ts = exp->ts;

    if (exp->sym) {
        log_info("type expr is already resolved.");
        return;
    }

    ts = resolve_type(ps, ts);
    if (!ts) return;
    if (!check_type(ps, ts)) return;

    if (type_is_optional(ts)) {
        log_warn("optional type has no symbol, check source type:\n");
        log_type_spec(ts->opt.src);
        exp->ts = ts;
        return;
    }

    ASSERT(ts->sym_id >= 0);
    exp->sym = get_symbol_by_id(ts->sym_id);
    // ASSERT(exp->sym->kind == SYM_CLASS);
    // update expr type as symbol type
    // int -> exp->ts is type type, symbol is int
    // Foo -> exp->ts is type type, symbol is Foo
    exp->ts = exp->sym->ts;
    log_info("type '%s' is resolved as: ", exp->sym->name);
    log_type_spec(exp->ts);
}

static TypeSpec *instance_type_spec(TypeSpec *ts, KlassSymbol *origin, InstanceSymbol *sym,
                                    ParserState *ps)
{
    Vector *kls_tps = &origin->tps;
    Vector *tp_args = sym->tp_args;

    TypeSpec *inst_ts;
    if (ts->kind == TYPE_GENERIC_VAR) {
        if (strcmp(ts->generic_var.owner, origin->name) != 0) return ts;
        TypeParamSymbol *tp_sym = vector_get(kls_tps, ts->generic_var.index);
        ASSERT(tp_sym);
        if (!strcmp(tp_sym->name, ts->generic_var.name) &&
            (ts->sym_id < 0 || tp_sym->id == ts->sym_id)) {
            if (tp_sym->which == TP_INFER) {
                ASSERT(sym->arg);
                inst_ts = sym->arg;
                log_info("generic var '%s' is inferred as '%s' for instance specialization",
                         ts->generic_var.name, inst_ts->signature);
            } else {
                log_info("generic var '%s' matches klass's tp, use tp_args to get instance type",
                         ts->generic_var.name);
                inst_ts = vector_get(tp_args, ts->generic_var.index);
            }
        } else {
            // generic var must be defined in method's tps, not klass's tps.
            log_info("generic var '%s' does not match klass's tp, keep it.", ts->generic_var.name);
            inst_ts = ts;
        }
    } else if (ts->kind == TYPE_GENERIC_REF) {
        Vector *_tp_args = vector_create_ptr();
        TypeSpec *_ts;
        vector_foreach(_ts, ts->generic_ref.args) {
            TypeSpec *inst_arg = instance_type_spec(_ts, origin, sym, ps);
            vector_push_back(_tp_args, &inst_arg);
        }
        Symbol *_sym = get_symbol_by_id(ts->sym_id);
        InstanceSymbol *inst_sym = find_or_add_instance(ps->pm->stbl, _sym, _tp_args, ps->pm);
        inst_ts = inst_sym->instance_ts;
        vector_destroy(_tp_args);
    } else if (type_is_valist(ts)) {
        TypeSpec *src = ts->va_list.src;
        if (src->kind == TYPE_GENERIC_VAR) {
            if (strcmp(src->generic_var.owner, origin->name) != 0) return ts;
            TypeParamSymbol *tp_sym = vector_get(kls_tps, src->generic_var.index);
            ASSERT(tp_sym);
            if (!strcmp(tp_sym->name, src->generic_var.name) &&
                (src->sym_id < 0 || tp_sym->id == src->sym_id)) {
                TypeSpec *inst_src_ts;
                if (tp_sym->which == TP_INFER) {
                    ASSERT(sym->arg);
                    inst_src_ts = sym->arg;
                    log_info(
                        "var-arg generic var '%s' is inferred as '%s' for instance "
                        "specialization",
                        src->generic_var.name, inst_src_ts->signature);
                } else {
                    log_info(
                        "var-arg generic var '%s' matches klass's tp, use tp_args to get "
                        "instance type",
                        src->generic_var.name);
                    inst_src_ts = vector_get(tp_args, src->generic_var.index);
                    log_info("var-arg generic var '%s' is resolved as '%s'", src->generic_var.name,
                             inst_src_ts->signature);
                }
                // update var-arg type as var-arg of instance type.
                inst_ts = va_list_type_spec_intern(inst_src_ts);
            } else {
                // generic var must be defined in method's tps, not klass's tps.
                log_info("var-arg generic var '%s' does not match klass's tp, keep it.",
                         src->generic_var.name);
                inst_ts = ts;
            }
        } else {
            log_info("var-arg source type is not generic var, keep it.");
            inst_ts = ts;
        }
    } else if (ts->kind == TYPE_OPTIONAL) {
        TypeSpec *src = instance_type_spec(ts->opt.src, origin, sym, ps);
        inst_ts = optional_type_spec_intern(src);
    } else if (ts->kind == TYPE_PROTO) {
        Vector *args = vector_create_ptr();
        TypeSpec *arg;
        vector_foreach(arg, ts->proto_type.args) {
            TypeSpec *inst_arg = instance_type_spec(arg, origin, sym, ps);
            vector_push_back(args, &inst_arg);
        }
        TypeSpec *ret = instance_type_spec(ts->proto_type.ret, origin, sym, ps);
        inst_ts = func_type_spec_intern(args, ret);
    } else {
        inst_ts = ts;
    }
    return inst_ts;
}

static Vector *build_instance_params(Vector *params, InstanceSymbol *sym, ParserState *ps)
{
    KlassSymbol *origin = (KlassSymbol *)sym->origin;
    ASSERT(origin);

    Vector *inst_params = vector_create_ptr();
    ArgInfo *arg;
    vector_foreach(arg, params) {
        if (!arg) continue;
        TypeSpec *ts = instance_type_spec(arg->ts, origin, sym, ps);
        ArgInfo *inst_arg = mm_alloc_obj(inst_arg);
        inst_arg->name = arg->name;
        inst_arg->ts = ts;
        inst_arg->has_dfl_val = arg->has_dfl_val;
        inst_arg->dfl_val = arg->dfl_val;
        vector_push_back(inst_params, &inst_arg);
    }
    return inst_params;
}

static inline int func_has_infer_tp(FuncSymbol *fn_sym)
{
    if (vector_size(&fn_sym->tps) <= 0) return 0;

    TypeParamSymbol *tp_sym = vector_get(&fn_sym->tps, 0);
    if (tp_sym->which != TP_INFER) return 0;
    return 1;
}

static Vector *get_func_real_params(FuncSymbol *fn_sym, Vector *_tp_args)
{
    Vector *real_params = vector_create_ptr();
    ArgInfo *arg;
    vector_foreach(arg, fn_sym->params) {
        if (!arg) continue;
        TypeSpec *ts = arg->ts;
        if (ts->kind == TYPE_GENERIC_VAR) {
            TypeSpec *_ts = vector_get(_tp_args, ts->generic_var.index);
            ASSERT(_ts);
            ASSERT(!strcmp(fn_sym->name, ts->generic_var.owner));
            ts = _ts;
        }

        ArgInfo *real_arg = mm_alloc_obj(real_arg);
        real_arg->name = arg->name;
        real_arg->ts = ts;
        real_arg->has_dfl_val = arg->has_dfl_val;
        real_arg->dfl_val = arg->dfl_val;
        vector_push_back(real_params, &real_arg);
    }
    return real_params;
}

static TypeSpec *get_func_real_ret(FuncSymbol *fn_sym, Vector *_tp_args)
{
    TypeSpec *ret = fn_sym->ret;
    ASSERT(!strcmp(fn_sym->name, ret->generic_var.owner));
    if (ret->kind == TYPE_GENERIC_VAR) {
        TypeSpec *_ts = vector_get(_tp_args, ret->generic_var.index);
        ret = _ts;
    }
    return ret;
}

typedef struct _TpInfo {
    HashMapEntry hnode;
    char *name;
    TypeSpec *real;
} TpInfo;

static int __tpinfo_eq__(const TpInfo *a, const TpInfo *b)
{
    char *a_name = a->name;
    char *b_name = b->name;

    ASSERT(a_name);
    ASSERT(b_name);
    return !strcmp(a_name, b_name);
}

static void __tpinfo_free__(void *info, void *arg) { mm_free(info); }

static Symbol *add_instance_method(InstanceSymbol *inst_sym, FuncSymbol *origin_fn_sym,
                                   ParserState *ps)
{
    KlassSymbol *origin = (KlassSymbol *)inst_sym->origin;
    ASSERT(origin);

    // If origin_fn_sym is an inherited symbol, get the actual function symbol
    FuncSymbol *actual_fn_sym = origin_fn_sym;
    if (origin_fn_sym->kind == SYM_INHERITED) {
        actual_fn_sym = ((InheritedFunc *)origin_fn_sym)->origin;
        ASSERT(actual_fn_sym);
    }

    Vector *inst_params = build_instance_params(actual_fn_sym->params, inst_sym, ps);
    TypeSpec *origin_ret_ts = actual_fn_sym->ret;
    TypeSpec *inst_ret_ts = instance_type_spec(origin_ret_ts, origin, inst_sym, ps);

    Symbol *inst_fn_sym = stbl_add_func(inst_sym->stbl, actual_fn_sym->name, inst_ret_ts,
                                        inst_params, actual_fn_sym->flags);
    inst_fn_sym->parent = inst_sym;
    // an instance method belongs to the package of its origin method
    inst_fn_sym->path = actual_fn_sym->path;
    inst_fn_sym->ts = func_type_spec_from_arginfo(inst_params, inst_ret_ts);
    ((FuncSymbol *)inst_fn_sym)->origin = actual_fn_sym;
    return inst_fn_sym;
}

// Get the instance method of an instance symbol by name. If the method is not found in the
// instance, it will look up the method in the origin class and its base types, and add it to the
// instance's symbol table for faster future lookup.
Symbol *get_instance_method(InstanceSymbol *inst_sym, char *name, ParserState *ps)
{
    Symbol *inst_fn_sym = stbl_get(inst_sym->stbl, name);
    if (inst_fn_sym) return inst_fn_sym;

    KlassSymbol *origin = (KlassSymbol *)inst_sym->origin;
    ASSERT(origin);

    Symbol *_sym = stbl_get(origin->stbl, name);
    if (_sym) {
        if (_sym->kind == SYM_INHERITED) {
            goto inherit;
        }
        ASSERT(_sym->kind == SYM_FUNC);
        FuncSymbol *origin_fn_sym = (FuncSymbol *)_sym;
        return add_instance_method(inst_sym, origin_fn_sym, ps);
    }

inherit:
    TypeSpec *base_ts;
    vector_foreach(base_ts, inst_sym->bases) {
        Symbol *base_sym = get_symbol_by_id(base_ts->sym_id);
        ASSERT(base_sym);

        if (base_sym->kind == SYM_TRAIT) {
            inst_fn_sym = stbl_get(base_sym->stbl, name);
        } else {
            ASSERT(base_sym->kind == SYM_INSTANCE);
            InstanceSymbol *base_inst_sym = (InstanceSymbol *)base_sym;
            inst_fn_sym = get_instance_method(base_inst_sym, name, ps);
        }

        if (inst_fn_sym) {
            // add to inst_sym's stbl for faster lookup next time
            ASSERT(inst_fn_sym->kind == SYM_FUNC);
            FuncSymbol *_fn_sym = (FuncSymbol *)inst_fn_sym;

            Vector *real_params = vector_create_ptr();
            ArgInfo *arg;
            vector_foreach(arg, _fn_sym->params) {
                if (!arg) continue;
                ArgInfo *_real_param = mm_alloc_obj(_real_param);
                _real_param->name = arg->name;
                _real_param->ts = arg->ts;
                _real_param->has_dfl_val = arg->has_dfl_val;
                _real_param->dfl_val = arg->dfl_val;
                vector_push_back(real_params, &_real_param);
            }

            inst_fn_sym = stbl_add_func(inst_sym->stbl, _fn_sym->name, _fn_sym->ret, real_params,
                                        _fn_sym->flags);
            inst_fn_sym->parent = inst_sym;
            // an instance method belongs to the package of its origin method
            inst_fn_sym->path = _fn_sym->path;
            inst_fn_sym->ts = func_type_spec_from_arginfo(real_params, _fn_sym->ret);
            return inst_fn_sym;
        }
    }

    return NULL;
}

// Every type conforms to Hashable, Equatable and Printable automatically (the class does not
// list them), so a type parameter has their methods even when its bound does not.
static Symbol *get_auto_trait_func(ParserState *ps, char *name)
{
    char *traits[] = { "Hashable", "Equatable", "Printable" };
    for (int i = 0; i < COUNT_OF(traits); i++) {
        Symbol *trait = stbl_get(ps->pm->builtin, traits[i]);
        if (!trait) continue;
        Symbol *fn = stbl_get(trait->stbl, name);
        if (fn) return fn;
    }
    return NULL;
}

// the method comes from an automatic trait, not from the bound of a type parameter
static int is_auto_trait_func(ParserState *ps, Symbol *fn)
{
    return fn && get_auto_trait_func(ps, fn->name) == fn;
}

static Symbol *get_func_from_tp(ParserState *ps, TypeParamSymbol *tp_sym, char *name)
{
    Symbol *fn = NULL;

    TypeSpec *ts;
    vector_foreach(ts, &tp_sym->bound) {
        if (!ts) continue;
        Symbol *bound_sym = get_symbol_by_id(ts->sym_id);
        ASSERT(bound_sym->kind == SYM_TRAIT || bound_sym->kind == SYM_INSTANCE);

        if (bound_sym->kind == SYM_INSTANCE) {
            fn = get_instance_method((InstanceSymbol *)bound_sym, name, ps);
        } else {
            fn = stbl_get(bound_sym->stbl, name);
        }
        if (fn) break;
    }

    // the bound first, then the automatic Hashable / Equatable / Printable
    if (!fn) fn = get_auto_trait_func(ps, name);

    return fn;
}

static Symbol *get___init___instance(InstanceSymbol *inst_sym, ParserState *ps)
{
    char *name = "__init__";
    Symbol *inst_fn_sym = stbl_get(inst_sym->stbl, name);
    if (inst_fn_sym) return inst_fn_sym;

    KlassSymbol *origin = (KlassSymbol *)inst_sym->origin;
    ASSERT(origin);

    Symbol *_sym = stbl_get(origin->stbl, name);
    ASSERT(_sym);

    if (origin->__init__ == NULL) origin->__init__ = _sym;

    return add_instance_method(inst_sym, (FuncSymbol *)_sym, ps);
}

/**
 * A number type satisfies a generic bound through a declared base with a compatible type argument.
 * `int8` declares `Comparable[int64]`, and an `int8` can be passed as an `int64`, so `int8`
 * satisfies `Comparable[int8]`:
 *
 *     pub class int8 : Comparable[int64] & ToString { ... }
 *
 *     func max[T : Comparable](x T, y T) T { ... }
 *
 *     max(int8(1), int8(2))    // ok, T is int8
 *
 * The base may be inherited: `Comparable[int64]` brings `Equatable[int64]`, so `int8` satisfies
 * `Equatable[int8]`. Other types stay invariant: `list[int64]` does not satisfy `Sequence[int8]`.
 */
static int number_base_satisfies_bound(TypeSpec *bound, TypeSpec *ts, ParserState *ps)
{
    if (ts->kind != TYPE_INT && ts->kind != TYPE_FLOAT && ts->kind != TYPE_BFLOAT16) return 0;

    Symbol *bound_sym = get_symbol_by_id(bound->sym_id);
    if (bound_sym->kind != SYM_INSTANCE) return 0;
    InstanceSymbol *bound_inst = (InstanceSymbol *)bound_sym;

    Vector *base_args = find_matched_tp_args((Symbol *)bound_inst->origin, ts, ps);
    if (!base_args) return 0;

    TypeSpec *base_arg = vector_get(base_args, 0);
    TypeSpec *bound_arg = vector_get(bound_inst->tp_args, 0);
    return type_spec_compatible(base_arg, bound_arg);
}

/**
 * Check whether `ts` satisfies all bounds declared on `tp`.
 *
 * For a declaration like `T: Comparable & Hashable`, this verifies that
 * `ts` is compatible with EVERY bound (intersection semantics).
 */
static int check_generic_bound(TypeSpec *ts, TypeParamSymbol *tp, Vector *tp_args, ParserState *ps)
{
    if (!tp || !ts) return 0;

    /* No bounds declared → unconstrained, always satisfied */
    if (vector_empty(&tp->bound)) return 1;

    int satisfied = 1;

    /* Every bound must be satisfied (AND semantics) */
    TypeSpec *bound;
    vector_foreach(bound, &tp->bound) {
        if (!bound) continue;

        int compatible = 0;

        /* an instance is created when it is first needed, so the bound has a symbol */
        TypeSpec *_bound = type_spec_specialize(bound, tp_args, ps->pm->stbl, ps->pm);

        /* Case 1: arg is a concrete type → existing logic */
        if (ts->kind != TYPE_GENERIC_VAR) {
            Symbol *ts_sym = get_symbol_by_id(ts->sym_id);
            ASSERT(ts_sym && ts_sym->ts && ts_sym->ts->kind == TYPE_TYPE);
            compatible = type_spec_compatible(_bound, ts);
            if (!compatible) compatible = number_base_satisfies_bound(_bound, ts, ps);
        } else {
            /*
             * Case 2: 'ts' is a generic variable (e.g., U passed to T).
             * U satisfies bound B if U's declared bounds contain B(or a supertype of B).
             *
             * Direction: ts_bound (provided, stricter) must be usable as
             *            bound (required, looser).
             * i.e., compatible(ts_bound, bound) ⟺ arg_bound ≤ bound
             */
            TypeParamSymbol *sym = get_symbol_by_id(ts->sym_id);
            ASSERT(sym);
            Vector *ts_bound = &sym->bound;
            if (!vector_empty(ts_bound)) {
                // tp's bounds are all satisfied by ts's bounds.
                TypeSpec *provided_ts;
                vector_foreach(provided_ts, ts_bound) {
                    if (!provided_ts) continue;
                    TypeSpec *_provided_ts =
                        type_spec_specialize(provided_ts, tp_args, ps->pm->stbl, ps->pm);
                    if (type_spec_compatible(_provided_ts, _bound)) {
                        compatible = 1;
                        break;
                    }
                }
            }
        }

        if (!compatible) {
            satisfied = 0;

            // Report every unsatisfied bound
            BUF(req_buf);
            BUF(prov_buf);
            type_spec_print(ts, &prov_buf);
            type_spec_print(_bound, &req_buf);
            log_info("generic bound not satisfied:  '%s' does not satisfy '%s' required by '%s'",
                     BUF_STR(req_buf), BUF_STR(prov_buf), tp->name);
            FINI_BUF(req_buf);
            FINI_BUF(prov_buf);
        }
    }

    return satisfied;
}

/**
 * Checks that the type arguments inferred from a call satisfy their `T: Bound`. Type arguments
 * written in the source are checked where `[...]` is parsed, but a call that infers them does not
 * go through there. Without this check the error only shows at run time.
 *
 * `tps` are the type parameters of the function or class, and `tp_args` the inferred arguments in
 * the same order. A slot with no inferred type is skipped. Reports the first argument that does
 * not satisfy its bound and returns 0, otherwise returns 1.
 *
 *     trait Shape { func area() int }
 *
 *     class Rect : Shape { pub func area() int { return 10 } }
 *     class NotShape { pub func area() int { return 10 } }    // has the method, not the trait
 *
 *     func area_of[T: Shape](v T) int { return v.area() }
 *     class Holder[T: Shape] { ... }
 *
 *     area_of(Rect())       // ok, T is Rect
 *     area_of(NotShape())   // error: type argument 'NotShape' does not satisfy constraint ...
 *     Holder(NotShape())    // error: the same, T is inferred from `__init__`
 */
static int check_inferred_bounds(Vector *tps, Vector *tp_args, Loc loc, ParserState *ps)
{
    TypeParamSymbol *tp_sym;
    vector_foreach(tp_sym, tps) {
        TypeSpec *arg_ts = vector_get(tp_args, i__);
        if (!arg_ts) continue;
        if (!check_generic_bound(arg_ts, tp_sym, tp_args, ps)) {
            BUF(ts_buf);
            type_spec_print(arg_ts, &ts_buf);
            kl_error(loc, "type argument '%s' does not satisfy constraint on type parameter '%s'",
                     BUF_STR(ts_buf), tp_sym->name);
            FINI_BUF(ts_buf);
            return 0;
        }
    }

    return 1;
}

static Vector *infer_tp_from_new(KlassSymbol *cls_sym, FuncSymbol *fn_sym, CallExpr *call_exp,
                                 ParserState *ps)
{
    Vector *result = vector_create_ptr();
    Vector *tps = &cls_sym->tps;
    Vector *params = fn_sym->params;
    Vector *call_args = call_exp->args;

    HashMap map;
    hashmap_init(&map, (HashMapEqualFunc)__tpinfo_eq__);

    ArgInfo *arg;
    vector_foreach(arg, params) {
        if (!arg) continue;
        if (arg->has_dfl_val) {
            // default value param, skip it and the rest params.
            log_info("param '%s' is key-word parameter, skip the rest.", arg->name);
            break;
        }

        TypeSpec *ts = arg->ts;

        Expr *e = call_arg_for_param(call_args, i__, arg->name);
        if (!e) {
            // no more arguments passed, report error.
            if (type_is_valist(ts)) {
                // var-arg can be empty, break the loop and check the rest params.
                log_info("param '%s' is var-arg, no more arguments passed, skip the rest.",
                         arg->name);
                break;
            } else {
                kl_error(call_exp->loc,
                         "expected at least %d arguments in __init__ call, but %d got.",
                         vector_size(params), i__);
                hashmap_fini(&map, __tpinfo_free__, NULL);
                vector_destroy(result);
                return NULL;
            }
        }

        TypeSpec *real = e->ts;
        ASSERT(real);

        if (ts->kind == TYPE_GENERIC_VAR) {
            log_info("param '%s' is generic var '%s'", arg->name, ts->generic_var.name);
            ASSERT(!strcmp(ts->generic_var.owner, cls_sym->name));

            TpInfo key = { .name = ts->generic_var.name };
            hashmap_entry_init(&key, str_hash(key.name));
            TpInfo *info = hashmap_get(&map, &key);
            if (info) {
                // the same type parameter may be bound by several parameters,
                // e.g. __init__(k K, d dict[K, V]) or __init__(k K, v K).
                if (!type_spec_compatible(info->real, real)) {
                    kl_error(e->loc, "generic type '%s' is inferred as '%s', but got '%s'",
                             info->name, info->real->signature, real->signature);
                    hashmap_fini(&map, __tpinfo_free__, NULL);
                    vector_destroy(result);
                    return NULL;
                }
            } else {
                info = mm_alloc_obj(info);
                info->name = ts->generic_var.name;
                info->real = real;
                hashmap_entry_init(info, str_hash(info->name));
                void *r = hashmap_put(&map, info);
                ASSERT(!r);
            }
        } else if (ts->kind == TYPE_GENERIC_REF) {
            log_info("param '%s' is generic ref '%s'", arg->name, ts->signature);
            // reuse the generic ref type inference shared with 'infer_tp_from_call'.
            if (!infer_tp_from_generic_ref(&map, ts, real, cls_sym->name, e->loc, ps)) {
                hashmap_fini(&map, __tpinfo_free__, NULL);
                vector_destroy(result);
                return NULL;
            }
        } else if (ts->kind == TYPE_VA_LIST) {
            // var-arg must be last one parameter.
            log_info("param '%s' is var-arg", arg->name);

            // TODO: guard code
            if (i__ != (vector_size(params) - 1)) {
                // after i__ are all kw-args.
                for (int j = i__ + 1; j < vector_size(params); j++) {
                    ArgInfo *next_arg = vector_get(params, j);
                    ASSERT(next_arg->has_dfl_val);
                }
            }

            Vector *var_arg_types = vector_create_ptr();
            for (int j = i__; j < vector_size(call_args); j++) {
                Expr *e = vector_get(call_args, j);
                if (!e) continue;

                if (e->kind == EXPR_KW_KIND) {
                    KeyWordExpr *kw = (KeyWordExpr *)e;
                    log_info("var-arg '%s' is passed as keyword argument '%s', skip", arg->name,
                             kw->key.name);
                    continue;
                }

                TypeSpec *arg_ts;
                if ((e->kind == EXPR_BANG_KIND) && (match_sequence(e->ts, NULL, &arg_ts))) {
                    vector_push_back(var_arg_types, &arg_ts);
                    log_info(
                        "var-arg '%s' is passed with bang operator(sequence[T]), "
                        "sequence's type arg is added to var-arg types",
                        arg->name);
                } else {
                    vector_push_back(var_arg_types, &e->ts);
                    log_info(
                        "var-arg '%s' is passed without bang operator, argument type is "
                        "added to var-arg types",
                        arg->name);
                }
            }

            TypeSpec *src = ts->va_list.src;
            if (src->kind == TYPE_GENERIC_VAR) {
                log_info("var-arg type is generic var '%s'", src->generic_var.name);
                ASSERT(!strcmp(src->generic_var.owner, cls_sym->name));
                TypeParamSymbol *tp_sym = vector_get(tps, src->generic_var.index);
                ASSERT(tp_sym);

                if (tp_sym->which == TP_INFER) {
                    log_info("generic var '%s' is inferred", src->generic_var.name);
                    NYI();
                } else {
                    log_info("generic var '%s' is normal", src->generic_var.name);
                    real = find_lub(var_arg_types);
                    ASSERT(real);
                }

                TpInfo *info = mm_alloc_obj(info);
                info->name = src->generic_var.name;
                info->real = real;
                hashmap_entry_init(info, str_hash(info->name));
                void *r = hashmap_put(&map, info);
                ASSERT(!r);
            } else {
                NYI();
            }
        } else {
            if (ts != real) {
                kl_error_incompatible_type(e->loc, ts, real);
                hashmap_fini(&map, __tpinfo_free__, NULL);
                vector_destroy(result);
                return NULL;
            }
        }
    }

    TypeParamSymbol *tp_sym;
    vector_foreach(tp_sym, tps) {
        if (!tp_sym) continue;
        TpInfo key = { .name = tp_sym->name };
        hashmap_entry_init(&key, str_hash(key.name));
        TpInfo *info = hashmap_get(&map, &key);
        if (!info) {
            kl_error(call_exp->loc, "cannot infer type parameter '%s' for class '%s'.",
                     tp_sym->name, cls_sym->name);
            hashmap_fini(&map, __tpinfo_free__, NULL);
            vector_destroy(result);
            return NULL;
        }
        log_info("inferred type parameter '%s' for class '%s':", tp_sym->name, cls_sym->name);
        log_type_spec(info->real);
        vector_push_back(result, &info->real);
    }

    hashmap_fini(&map, __tpinfo_free__, NULL);
    return result;
}

typedef struct _KWArgInfo {
    char *name;
    TypeSpec *ts;
    Literal *dfl_val;
} KWArgInfo;

typedef struct _ParamInfo {
    int valist_index;
    int npos;
    Vector kwargs;
} ParamInfo;

static void __get_param_info(Vector *params, ParamInfo *param_info)
{
    int npos = 0;
    int i = 0;
    ArgInfo *arg;
    vector_foreach(arg, params) {
        if (!arg) continue;
        if (arg->has_dfl_val) {
            KWArgInfo info = { .name = arg->name, .ts = arg->ts, .dfl_val = arg->dfl_val };
            vector_push_back(&param_info->kwargs, &info);
            log_info("[__get_param_info] param '%s' is key-word parameter, add to kwargs",
                     arg->name);
        } else if (type_is_valist(arg->ts)) {
            param_info->valist_index = i;
            log_info("[__get_param_info] param '%s' is var-arg, valist_index is %d", arg->name,
                     param_info->valist_index);
        } else {
            npos++;
        }
        i++;
    }
    param_info->npos = npos;
    log_info("[__get_param_info] number of positional parameters: %d", param_info->npos);
}

typedef struct _CallArgInfo {
    int npos;
    Vector kwargs;
} CallArgInfo;

static void __get_call_arg_info(Vector *args, CallArgInfo *arg_info)
{
    int i = 0;
    Expr *e;
    vector_foreach(e, args) {
        if (!e) continue;
        if (e->kind == EXPR_KW_KIND) {
            KeyWordExpr *kw = (KeyWordExpr *)e;
            vector_push_back(&arg_info->kwargs, &e);
            log_info("[__get_call_arg_info] call arg is keyword argument: '%s', add to kw_args",
                     kw->key.name);
        } else {
            i++;
        }
    }
    arg_info->npos = i;
    log_info("[__get_call_arg_info] number of positional arguments in call: %d", arg_info->npos);
}

static int handle_valist_and_dfl_args(Vector *params, CallExpr *call)
{
    ParamInfo param_info;
    param_info.valist_index = -1;
    vector_init(&param_info.kwargs, sizeof(KWArgInfo));
    __get_param_info(params, &param_info);

    CallArgInfo arg_info;
    arg_info.npos = 0;
    vector_init_ptr(&arg_info.kwargs);
    __get_call_arg_info(call->args, &arg_info);

    Vector *vec = vector_create_ptr();
    Vector *real_args = vector_create_ptr();
    int eaten_kw_args = 0;
    int changed = 0;

    // A required parameter is either filled by the positional prefix or, once that
    // runs out, by a keyword argument. check_call_args has already verified that
    // every one of them is covered.
    for (int i = 0; i < param_info.npos; i++) {
        Expr *arg = NULL;
        if (i < arg_info.npos) {
            arg = vector_get(call->args, i);
        } else {
            ArgInfo *param = vector_get(params, i);
            KeyWordExpr *kw;
            vector_foreach(kw, &arg_info.kwargs) {
                if (str_equal(kw->key.name, param->name)) {
                    arg = kw->value;
                    break;
                }
            }
            ASSERT(arg);
            log_info("[handle_valist_and_dfl_args] required param '%s' is passed by keyword.",
                     param->name);
            changed = 1;
        }
        vector_push_back(real_args, &arg);
    }

    if (param_info.valist_index >= 0) {
        for (int i = param_info.valist_index; i < arg_info.npos; i++) {
            Expr *arg = vector_get(call->args, i);
            vector_push_back(vec, &arg);
        }
    } else {
        // no var-arg param, kw-args may be passed by positional arguments
        eaten_kw_args = arg_info.npos - param_info.npos;

        for (int i = param_info.npos; i < arg_info.npos; i++) {
            Expr *arg = vector_get(call->args, i);
            vector_push_back(real_args, &arg);
        }

        log_info(
            "[handle_valist_and_dfl_args] no var-arg param, and %d kw-args are passed by "
            "positional arguments.",
            eaten_kw_args);
    }

    if (!vector_empty(vec)) {
        // If a single va_list variable is passed to a va_list parameter,
        // skip tuple wrapping to avoid double encapsulation.
        int skip_tuple = 0;
        ArgInfo *valist_param = vector_get(params, param_info.valist_index);
        if (vector_size(vec) == 1 && type_is_valist(valist_param->ts)) {
            Expr *e = vector_get(vec, 0);
            if (e->ts && type_is_valist(e->ts)) {
                log_info(
                    "[handle_valist_and_dfl_args] var-arg is already a va_list value, "
                    "pass directly without tuple wrapping.");
                vector_push_back(real_args, &e);
                skip_tuple = 1;
            }
        }

        if (skip_tuple) {
            vector_destroy(vec);
            vec = NULL;
        } else {
            log_info(
                "[handle_valist_and_dfl_args] var-arg is passed by positional arguments(%d-%d), "
                "build tuple for caller.",
                param_info.valist_index, arg_info.npos - 1);
            Expr *valist = expr_from_tuple(vec);
            vector_push_back(real_args, &valist);
            changed = 1;
        }
    } else {
        log_info("[handle_valist_and_dfl_args] no var-arg is passed by positional arguments.");
        if (param_info.valist_index >= 0) {
            // The variadic parameter still occupies one positional slot even when
            // zero variadic arguments are passed. Fill the slot with an empty
            // tuple so that the call argument count always matches the prototype
            // parameter count (see update_call_args in irgen).
            log_info("[handle_valist_and_dfl_args] build an empty tuple for var-arg parameter.");
            Expr *valist = expr_from_tuple(vec);
            vector_push_back(real_args, &valist);
            changed = 1;
        } else {
            vector_destroy(vec);
        }
        vec = NULL;
    }

    int j = 0;
    KWArgInfo *kwarg;
    vector_foreach_ptr(kwarg, &param_info.kwargs) {
        if (j < eaten_kw_args) {
            j++;
            continue;
        }
        int found = 0;
        KeyWordExpr *kw;
        vector_foreach(kw, &arg_info.kwargs) {
            if (str_equal(kw->key.name, kwarg->name)) {
                log_info("[handle_valist_and_dfl_args] kw-arg '%s' is passed, add to caller args.",
                         kwarg->name);
                vector_push_back(real_args, &kw->value);
                found = 1;
                break;
            }
        }

        if (!found) {
            log_info(
                "[handle_valist_and_dfl_args] kw-arg '%s' is not passed, use default value for "
                "caller.",
                kwarg->name);
            Literal *dfl_val = kwarg->dfl_val;
            ASSERT(dfl_val);
            Expr *dfl_exp = expr_from_literal(dfl_val);
            /* the literal takes the parameter's type (checked at the declaration),
               e.g. 44 for 'v uint8 = 44' is a uint8, not the literal's int64/uint64 */
            dfl_exp->expected = kwarg->ts;
            vector_push_back(real_args, &dfl_exp);
            changed = 1;
        }
    }

    if (!vector_empty(real_args)) {
        log_info(
            "[handle_valist_and_dfl_args] real arguments for caller after handling var-arg and "
            "kw-arg.");
        // TODO: free old args
        vector_destroy(call->args);
        call->args = real_args;
    } else {
        log_info(
            "[handle_valist_and_dfl_args] no real argument for caller after handling var-arg "
            "and "
            "kw-arg.");
        vector_destroy(real_args);
        real_args = NULL;
    }

    return changed;
}

// Operator hook dunders must be invoked through their syntax sugar only,
// never called explicitly as functions (Swift-style rule).
static const char *operator_dunder_sugar(const char *name)
{
    static const struct {
        const char *dunder;
        const char *sugar;
    } table[] = {
        // arithmetic operators
        { "__add__", "the '+' operator" },
        { "__sub__", "the '-' operator" },
        { "__mul__", "the '*' operator" },
        { "__div__", "the '/' operator" },
        { "__mod__", "the '%' operator" },
        { "__neg__", "the unary '-' operator" },
        { "__iadd__", "the '+=' operator" },
        { "__isub__", "the '-=' operator" },
        { "__imul__", "the '*=' operator" },
        { "__idiv__", "the '/=' operator" },
        { "__imod__", "the '%=' operator" },
        // bitwise operators
        { "__bitand__", "the '&' operator" },
        { "__bitor__", "the '|' operator" },
        { "__bitxor__", "the '^' operator" },
        { "__bitnot__", "the '~' operator" },
        { "__shl__", "the '<<' operator" },
        { "__shr__", "the '>>' operator" },
        { "__ibitand__", "the '&=' operator" },
        { "__ibitor__", "the '|=' operator" },
        { "__ibitxor__", "the '^=' operator" },
        { "__ishl__", "the '<<=' operator" },
        { "__ishr__", "the '>>=' operator" },
        // logical operators
        { "__and__", "the '&&' operator" },
        { "__or__", "the '||' operator" },
        { "__not__", "the '!' operator" },
        // comparison operators
        { "__eq__", "the '==' operator" },
        { "__ne__", "the '!=' operator" },
        { "__lt__", "the '<' operator" },
        { "__le__", "the '<=' operator" },
        { "__gt__", "the '>' operator" },
        { "__ge__", "the '>=' operator" },
        // subscript access
        { "__getitem__", "the subscript syntax 'x[i]'" },
        { "__setitem__", "the subscript syntax 'x[i] = v'" },
        { "__getslice__", "the slice syntax 'x[a:b]'" },
        { "__setslice__", "the slice syntax 'x[a:b] = v'" },
        // callable
        { "__call__", "the call syntax 'obj(...)'" },
        // membership
        { "__contains__", "the 'in' operator" },
        // constructor
        { "__init__", "the constructor syntax 'Type(...)'" },
        // resource management
        { "__enter__", "the 'with' statement" },
        { "__exit__", "the 'with' statement" },
    };

    for (size_t i = 0; i < sizeof(table) / sizeof(table[0]); i++) {
        if (str_equal(name, table[i].dunder)) {
            return table[i].sugar;
        }
    }
    return NULL;
}

static void parse_call(ParserState *ps, Expr *exp)
{
    CallExpr *call = (CallExpr *)exp;

    if (call->done) {
        log_info("call expr is already parsed.");
        return;
    }

    Expr *arg;
    vector_foreach(arg, call->args) {
        if (!arg) continue;
        arg->ctx = EXPR_CTX_LOAD;
        /* an untyped lambda takes its parameter types from the callee's parameter,
           which is not known until the callee is resolved: check_call_args fills it
           in, and a composite literal forwards it to the lambdas nested inside */
        Expr *value = (arg->kind == EXPR_KW_KIND) ? ((KeyWordExpr *)arg)->value : arg;
        if (expr_needs_infer(value)) continue;
        parser_visit_expr(ps, arg);
        if (!arg->ts) return;
    }

    Expr *lhs = call->lhs;

    /* callee may be NULL after the `primary_expr '.' error` recovery rule, e.g.
     * `l.0(1)`; the error is already reported. */
    if (!lhs) return;

    lhs->ctx = EXPR_CTX_CALL;
    lhs->arg = call;
    parser_visit_expr(ps, call->lhs);
    lhs->arg = NULL;
    if (!lhs->ts) return;

    Symbol *lhs_sym = lhs->sym;

    // operator hook dunders go through syntax sugar, explicit calls are forbidden
    if (lhs->kind == EXPR_DOT_KIND && lhs_sym) {
        const char *sugar = operator_dunder_sugar(lhs_sym->name);
        if (sugar) {
            kl_error(lhs->loc, "'%s' cannot be called explicitly, use %s instead.", lhs_sym->name,
                     sugar);
            return;
        }
    }

    Vector *params = NULL;
    if (!lhs_sym && lhs->ts->kind == TYPE_PROTO) {
        exp->ts = lhs->ts->proto_type.ret;
        params = arg_infos_from_arg_types(lhs->ts->proto_type.args);
    } else if (!lhs_sym) {
        kl_error(lhs->loc, "expression is not callable.");
        return;
    } else if (lhs_sym->kind == SYM_VAR) {
        TypeSpec *ts = lhs_sym->ts;

        log_info("call lhs is variable of type:");
        log_type_spec(ts);

        if (ts->kind == TYPE_PROTO) {
            log_info("call lhs is proto variable.");
            // proto variable call
            exp->ts = ts->proto_type.ret;
            params = arg_infos_from_arg_types(ts->proto_type.args);
        } else if (ts->kind == TYPE_KLASS) {
            Symbol *_sym = get_symbol_by_id(ts->sym_id);
            if (!_sym) {
                UNREACHABLE();
                kl_error(lhs->loc, "'%s' is not callable", lhs_sym->name);
                return;
            }

            if (_sym->kind != SYM_CLASS) {
                kl_error(lhs->loc, "'%s' is not callable", lhs_sym->name);
                return;
            }

            // constructor call
            KlassSymbol *cls_sym = (KlassSymbol *)_sym;
            Symbol *call_fn_sym = stbl_get(cls_sym->stbl, "__call__");
            if (!call_fn_sym) {
                kl_error(lhs->loc, "'%s' is not callable.", lhs_sym->name);
                return;
            }
            exp->ts = ((FuncSymbol *)call_fn_sym)->ret;
            params = ((FuncSymbol *)call_fn_sym)->params;
            exp->arg = call_fn_sym;
        } else {
            kl_error(lhs->loc, "'%s' is not callable", lhs_sym->name);
            return;
        }
    } else if (lhs_sym->kind == SYM_CLASS) {
        // constructor call without type parameters, e.g. Foo(100)
        KlassSymbol *cls_sym = (KlassSymbol *)lhs_sym;
        log_info("call lhs is class '%s' without type parameters", cls_sym->name);

        Symbol *_fn_sym = stbl_get(cls_sym->stbl, "__init__");
        if (!_fn_sym) {
            if (cls_sym->flags & SYM_FLAGS_EXT) {
                kl_error(lhs->loc, "class '%s' is external, but has no public '__init__()'.",
                         cls_sym->name);
                return;
            } else {
                log_info("class '%s' has no constructor, add default __init__() for it.",
                         lhs_sym->name);
                _fn_sym = stbl_add_func(cls_sym->stbl, "__init__", no_type_spec(), NULL,
                                        cls_sym->flags | SYM_FLAGS_PUBLIC);
                // the default __init__ belongs to the package of its class
                _fn_sym->path = cls_sym->path;
            }
        }

        if (cls_sym->__init__ == NULL) cls_sym->__init__ = _fn_sym;

        if (str_equal(cls_sym->name, "tuple")) {
            log_info("infer tuple type parameters from __init__ arguments.");

            Vector *tp_args = vector_create_ptr();
            Expr *arg;
            vector_foreach(arg, call->args) {
                if (!arg) continue;
                vector_push_back(tp_args, &arg->ts);
            }

            InstanceSymbol *inst_sym = find_or_add_instance(ps->pm->stbl, lhs_sym, tp_args, ps->pm);
            vector_destroy(tp_args);

            Symbol *_fn = get___init___instance(inst_sym, ps);
            ASSERT(_fn);

            // func call type is instance type
            exp->ts = inst_sym->instance_ts;
            params = ((FuncSymbol *)_fn)->params;
            ASSERT(cls_sym->__init__);
        } else {
            if (vector_size(&cls_sym->tps) > 0) {
                log_info(
                    "class '%s' has type parameters, try to infer them from __init__ "
                    "arguments.",
                    cls_sym->name);

                Vector *tp_args;

                if (vector_empty(call->args)) {
                    log_info("no arguments passed to __init__, set all tp as any");
                    tp_args = vector_create_ptr();
                    TypeSpec *any_ts = any_type_spec();
                    for (int i = 0; i < vector_size(&cls_sym->tps); i++) {
                        vector_push_back(tp_args, &any_ts);
                    }
                } else {
                    tp_args = infer_tp_from_new(cls_sym, ((FuncSymbol *)_fn_sym), call, ps);
                }

                if (!tp_args) {
                    kl_error(lhs->loc, "failed to infer type parameters for class '%s'.",
                             cls_sym->name);
                    return;
                }

                if (!check_inferred_bounds(&cls_sym->tps, tp_args, lhs->loc, ps)) {
                    vector_destroy(tp_args);
                    return;
                }

                InstanceSymbol *inst_sym =
                    find_or_add_instance(ps->pm->stbl, lhs_sym, tp_args, ps->pm);
                vector_destroy(tp_args);

                Symbol *_fn = get___init___instance(inst_sym, ps);
                ASSERT(_fn);

                // func call type is instance type
                exp->ts = inst_sym->instance_ts;
                params = ((FuncSymbol *)_fn)->params;
                ASSERT(cls_sym->__init__);
            } else {
                log_info("class '%s' has no type parameters.", cls_sym->name);

                if (str_equal(cls_sym->name, "range")) {
                    log_info("special handling for 'range' constructor call.");

                    /* range(stop) and range(start, stop) fill in the parameters the
                       caller left out. The transform is positional, so collect the
                       arguments into the three parameter slots first: a keyword
                       suffix may arrive in any order. */
                    Vector *range_params = ((FuncSymbol *)_fn_sym)->params;
                    int nargs = vector_size(call->args);
                    int npos = count_positional_args(call->args);
                    Expr *slots[3] = { NULL, NULL, NULL };

                    if (nargs <= 3) {
                        for (int i = 0; i < npos; i++) slots[i] = vector_get(call->args, i);
                        for (int j = npos; j < nargs; j++) {
                            Expr *e = vector_get(call->args, j);
                            KeyWordExpr *kw = (KeyWordExpr *)e;
                            int pi = find_param_by_name(range_params, kw->key.name);
                            if (pi >= 0 && pi < 3) slots[pi] = kw->value;
                        }

                        // range(stop): the lone positional argument is the end
                        if (!slots[1] && slots[0] && npos == 1 && npos == nargs) {
                            slots[1] = slots[0];
                            slots[0] = NULL;
                        }
                    }

                    if (nargs <= 3 && slots[1]) {
                        if (!slots[0]) {
                            slots[0] = expr_from_lit_int("0", 1, 0, 0);
                            slots[0]->ctx = EXPR_CTX_LOAD;
                            parser_visit_expr(ps, slots[0]);
                        }
                        if (!slots[2]) {
                            slots[2] = expr_from_lit_int("1", 1, 0, 1);
                            slots[2]->ctx = EXPR_CTX_LOAD;
                            parser_visit_expr(ps, slots[2]);
                        }

                        vector_clear(call->args);
                        for (int i = 0; i < 3; i++) vector_push_back(call->args, &slots[i]);
                        log_info("transformed range call to range(start, end, step).");
                    }
                }

                // func call type is instance type
                exp->ts = cls_sym->instance_ts;
                params = ((FuncSymbol *)_fn_sym)->params;
                ASSERT(cls_sym->__init__);
            }
        }
    } else if (lhs_sym->kind == SYM_FUNC) {
        FuncSymbol *fn_sym = (FuncSymbol *)lhs_sym;
        if (func_has_infer_tp(fn_sym)) {
            Vector *_tp_args = infer_func_tp(fn_sym, call->args, NULL, ps);
            if (!_tp_args) {
                kl_error(lhs->loc, "failed to infer type parameters for function '%s'.",
                         fn_sym->name);
                return;
            }

            log_info("inferred type parameters for function '%s':", fn_sym->name);
            TypeSpec *_tp_arg;
            vector_foreach(_tp_arg, _tp_args) {
                log_type_spec(_tp_arg);
            }
            params = get_func_real_params(fn_sym, _tp_args);
            exp->ts = get_func_real_ret(fn_sym, _tp_args);
            vector_destroy(_tp_args);
        } else if (vector_size(&fn_sym->tps) > 0) {
            // function has type parameters, do inference from call arguments
            Vector *_tp_args = lhs->tp_args;

            if (!_tp_args) {
                log_info(
                    "function '%s' has type parameters, try to infer them from call arguments.",
                    fn_sym->name);
                _tp_args = infer_tp_from_call(fn_sym, NULL, call, ps);
                if (!_tp_args) return;

                if (!check_inferred_bounds(&fn_sym->tps, _tp_args, lhs->loc, ps)) return;
            }

            Symbol *_fn_sym = find_or_add_func_instance(fn_sym, _tp_args, ps->pm->stbl, ps);
            if (_fn_sym->kind == SYM_FUNC) {
                exp->ts = ((FuncSymbol *)_fn_sym)->ret;
                params = ((FuncSymbol *)_fn_sym)->params;
                // update lhs->ts and lhs->sym
                lhs->ts = _fn_sym->ts;
                lhs->sym = _fn_sym;
                lhs_sym = _fn_sym;
            } else {
                InstanceFuncSymbol *fn_inst_sym = (InstanceFuncSymbol *)_fn_sym;
                params = fn_inst_sym->real_params;
                exp->ts = fn_inst_sym->ret_ts;
                lhs->ts = fn_inst_sym->ts;
                // type args with type variables (a call in a generic body): kept on the call, so
                // that a specialized clone of the body can route it with concrete type args
                if (tps_are_generic(_tp_args)) exp->tp_args = _tp_args;
            }
        } else {
            if (lhs->ts->kind == TYPE_OPTIONAL) {
                log_info("call lhs is optional of proto.");
                // optional proto function call
                if (fn_sym->ret->kind == TYPE_OPTIONAL) {
                    exp->ts = fn_sym->ret;
                } else {
                    exp->ts = optional_type_spec_intern(fn_sym->ret);
                }
            } else if (lhs->ts->kind == TYPE_PROTO) {
                log_info("call lhs is proto.");
                // proto function call
                exp->ts = fn_sym->ret;
            }
            params = fn_sym->params;
        }
    } else if (lhs_sym->kind == SYM_INSTANCE) {
        // constructor call with type parameters, e.g. Foo[int](100)
        InstanceSymbol *inst_sym = (InstanceSymbol *)lhs_sym;

        log_info("call lhs is instance of class '%s' with type parameters:", lhs_sym->name);

        KlassSymbol *origin = (KlassSymbol *)inst_sym->origin;
        if (origin->kind == SYM_TRAIT) {
            kl_error(lhs->loc, "trait '%s' cannot be instantiated.", origin->name);
            return;
        }

        if (origin->flags & SYM_FLAGS_EXT) {
            if (!stbl_get(origin->stbl, "__init__")) {
                kl_error(lhs->loc, "class '%s' is external, but has no public '__init__()'.",
                         origin->name);
                return;
            }
            inst_sym->flags |= SYM_FLAGS_EXT;
        }

        Symbol *init_fn_sym = get___init___instance(inst_sym, ps);
        ASSERT(init_fn_sym);

        if (origin->flags & SYM_FLAGS_EXT) init_fn_sym->flags |= SYM_FLAGS_EXT;

        if (str_equal(origin->name, "tuple")) {
            log_info("check __init__ args with specialized tuple type parameters.");
            if (vector_size(inst_sym->tp_args) != vector_size(call->args)) {
                kl_error(lhs->loc,
                         "tuple instance expects %d type arguments, but %d were provided.",
                         vector_size(inst_sym->tp_args), vector_size(call->args));
                return;
            }

            for (int i = 0; i < vector_size(inst_sym->tp_args); i++) {
                TypeSpec *tp_arg = vector_get(inst_sym->tp_args, i);
                Expr *arg = vector_get(call->args, i);
                if (!type_spec_compatible(tp_arg, arg->ts)) {
                    kl_error(arg->loc,
                             "tuple instance argument type is not compatible with "
                             "specialized type parameter.");
                    return;
                }
            }
        }

        // func call type is instance type
        exp->ts = inst_sym->instance_ts;
        params = ((FuncSymbol *)init_fn_sym)->params;
    } else if (lhs_sym->kind == SYM_INHERITED) {
        InheritedFunc *inherited = (InheritedFunc *)lhs_sym;
        FuncSymbol *origin_fn_sym = inherited->origin;
        log_info("call lhs is inherited function '%s'", inherited->name);
        exp->ts = origin_fn_sym->ret;
        params = origin_fn_sym->params;
    } else if (lhs_sym->kind == SYM_INSTANCE_FUNC) {
        InstanceFuncSymbol *inst_fn_sym = (InstanceFuncSymbol *)lhs_sym;
        exp->ts = inst_fn_sym->ret_ts;
        params = inst_fn_sym->real_params;
    } else {
        UNREACHABLE();
    }

    check_call_args(params, call->args, ps, lhs->loc);

    if (ps->errors > 0) return;

    // handle valist & default parameters for caller
    if (handle_valist_and_dfl_args(params, call)) {
        log_info(
            "arguments are changed after handling var-arg and kw-arg, re-parse argument types.");
        Expr *arg;
        vector_foreach(arg, call->args) {
            if (!arg) continue;
            arg->ctx = EXPR_CTX_LOAD;
            parser_visit_expr(ps, arg);
            if (!arg->ts) return;
        }
    }
    log_info("call expression type is:");
    log_type_spec(exp->ts);
    call->done = 1;
}

static TypeSpec *opt_dot_type(TypeSpec *ts, int opt_or_bang)
{
    if (opt_or_bang == DOT_OPTIONAL && ts->kind != TYPE_OPTIONAL) {
        return optional_type_spec_intern(ts);
    }
    return ts;
}

static void copy_tps(Vector *dst, Vector *src)
{
    TypeParamSymbol *tp_sym;
    vector_foreach(tp_sym, src) {
        if (!tp_sym) continue;
        TypeParamSymbol *new_tp_sym = mm_alloc_obj(new_tp_sym);
        new_tp_sym->name = tp_sym->name;
        new_tp_sym->which = tp_sym->which;
        vector_push_back(dst, &new_tp_sym);
    }
}

static void parse_enum_member(ParserState *ps, DotExpr *dot, EnumSymbol *enum_sym)
{
    Expr *exp = (Expr *)dot;
    if (dot->opt_or_bang != DOT_NORMAL) {
        kl_error(dot->id.loc, "enum member can't use `!.` or `?.`");
        return;
    }

    if (exp->ctx == EXPR_CTX_STORE || exp->ctx == EXPR_CTX_LOAD_STORE) {
        kl_error(dot->id.loc, "cannot assign to enum member '%s'", dot->id.name);
        return;
    }

    EnumMemberSym *item;
    vector_foreach_ptr(item, &enum_sym->members) {
        if (str_equal(item->name, dot->id.name)) {
            exp->ts = enum_sym->ts;
            /* irgen folds the member into its int64 value via the enum symbol */
            exp->sym = (Symbol *)enum_sym;
            return;
        }
    }

    kl_error(dot->id.loc, "enum '%s' has no member '%s'", enum_sym->name, dot->id.name);
}

static void parse_dot(ParserState *ps, Expr *exp)
{
    DotExpr *dot = (DotExpr *)exp;
    int opt_or_bang = dot->opt_or_bang;

    Expr *lhs = dot->lhs;

    /* The error-recovery rule `primary_expr '.' error` (koala.y) reports the
     * error and yields NULL, so a chained access such as `l.0.len()` reaches
     * this handler with a NULL lhs. The error has already been reported, just
     * stop analysing the expression instead of dereferencing NULL. */
    if (!lhs) return;

    /* `A.MEMBER`: the only way to refer an enum member */
    if (lhs->kind == EXPR_ID_KIND) {
        Symbol *esym = find_symbol(ps, &((IdentExpr *)lhs)->id);
        if (esym && esym->kind == SYM_ENUM) {
            parse_enum_member(ps, dot, (EnumSymbol *)esym);
            return;
        }
    }

    lhs->ctx = EXPR_CTX_LOAD;
    parser_visit_expr(ps, lhs);
    if (!lhs->ts) return;

    if (lhs->sym && lhs->sym->kind == SYM_ENUM) {
        parse_enum_member(ps, dot, (EnumSymbol *)lhs->sym);
        return;
    }

    if (type_is_enum(lhs->ts)) {
        kl_error(lhs->loc, "enum '%s' has no members.",
                 ((Symbol *)get_symbol_by_id(lhs->ts->sym_id))->name);
        return;
    }

    // '?.' is disabled for now: it type-checks but irgen cannot emit it yet
    if (opt_or_bang == DOT_OPTIONAL) {
        kl_error(exp->loc, "'?.' is not supported yet, use 'if let' or '!.'.");
        return;
    }

    if (lhs->ts->kind == TYPE_TYPE) {
        // static method?
        if (opt_or_bang != DOT_NORMAL) {
            kl_error(lhs->loc, "static method can't use `!.` or `?.`");
            return;
        }

        Symbol *lhs_sym = lhs->sym;
        if (lhs_sym->kind != SYM_CLASS) {
            kl_error(lhs->loc, "symbol '%s' is not a class", lhs_sym->name);
            return;
        }

        KlassSymbol *kls_sym = (KlassSymbol *)lhs_sym;
        Ident *ident = &dot->id;
        Symbol *st_sym = stbl_get(kls_sym->stbl, ident->name);
        if (!st_sym) {
            kl_error(ident->loc, "static method '%s' not found", ident->name);
            return;
        }

        if (!(st_sym->flags & SYM_FLAGS_STATIC)) {
            kl_error(ident->loc, "'%s' is not a static method.", ident->name);
            return;
        }

        exp->ts = st_sym->ts;
        exp->sym = st_sym;
        log_info("found static method '%s' in class '%s'", st_sym->name, kls_sym->name);
        return;
    }

    if (type_is_optional(lhs->ts)) {
        if (opt_or_bang == DOT_BANG) {
            // force unwrap, error if nil
            log_info("force unwrap optional type.");
            TypeSpec *src_ts = lhs->ts->opt.src;
            ASSERT(src_ts);
        } else if (opt_or_bang == DOT_OPTIONAL) {
            // safe unwrap, don't change lhs type and pass next
            log_info("safe unwrap optional type.");
            TypeSpec *src_ts = lhs->ts->opt.src;
            ASSERT(src_ts);
        } else {
            kl_error(lhs->loc, "optional cannot use dot operator.");
            return;
        }
    } else {
        Symbol *lhs_sym = lhs->sym;
        if (lhs_sym && lhs_sym->kind == SYM_SHADOW_VAR) {
            ShadowVarSymbol *shadow_sym = (ShadowVarSymbol *)lhs_sym;
            if (!shadow_sym->is_null) {
                if (opt_or_bang == DOT_OPTIONAL) {
                    kl_warn(lhs->loc,
                            "variable '%s' is non-nullable, but '?.' operator is used, "
                            "which is redundant.",
                            shadow_sym->name);
                } else if (opt_or_bang == DOT_BANG) {
                    kl_warn(lhs->loc,
                            "variable '%s' is non-nullable, but '!.' operator is used, "
                            "which is redundant.",
                            shadow_sym->name);
                }
            } else {
                UNREACHABLE();
            }
        } else {
            if (opt_or_bang == DOT_OPTIONAL) {
                kl_error(lhs->loc, "only optional type can use optional dot operator.");
                return;
            } else if (opt_or_bang == DOT_BANG) {
                kl_error(lhs->loc, "only optional type can use bang dot operator.");
                return;
            }
        }
    }

    int sym_id = lhs->ts->sym_id;
    if (type_is_optional(lhs->ts)) {
        log_info("unwrap optional type for member access.");
        ASSERT(lhs->ts->opt.src);
        ASSERT(lhs->ts->opt.src->sym_id >= 0);
        sym_id = lhs->ts->opt.src->sym_id;
    }

    Symbol *lhs_ts_sym = get_symbol_by_id(sym_id);

    if (type_is_valist(lhs->ts)) {
        // the symbol of a var-arg type is created when first asked for, as for `names[0]`
        lhs_ts_sym = get_type_symbol(lhs->ts, ps);
    }

    if (!lhs_ts_sym) {
        // TODO: print detailed typespec
        kl_error(lhs->loc, "type symbol is not found.");
        return;
    }

    HashMap *lhs_stbl = lhs_ts_sym->stbl;
    Ident *ident = &dot->id;
    Symbol *sym = stbl_get(lhs_stbl, ident->name);
    if (sym) {
        if (sym->kind == SYM_CLASS) {
            // exp->ts = opt_dot_type(((KlassSymbol *)sym)->instance_ts, opt_or_bang);
            exp->ts = opt_dot_type(sym->ts, opt_or_bang);
        } else if (sym->kind == SYM_INHERITED) {
            InheritedFunc *inherited = (InheritedFunc *)sym;
            exp->ts = opt_dot_type(inherited->origin->ts, opt_or_bang);
        } else {
            if (sym->flags & SYM_FLAGS_STATIC) {
                kl_error(ident->loc, "static method '%s' cannot be called by instance object",
                         ident->name);
                return;
            }
            exp->ts = opt_dot_type(sym->ts, opt_or_bang);
        }

        exp->sym = sym;
        log_info("dot member resolved: %s", sym->name);
        if (sym->kind == SYM_FUNC) {
            log_info("ret type is:");
            log_type_spec(((FuncSymbol *)sym)->ret);
        } else {
            log_type_spec(exp->ts);
        }
        return;
    }

    // hash(), fmt() are default functions, need to be found in any class
    if (!strcmp(ident->name, "hash")) {
        // TODO:
    } else if (!strcmp(ident->name, "fmt")) {
        // TODO:
    }

    // instance field/method from origin klass
    if (lhs_ts_sym->kind == SYM_INSTANCE) {
        log_info("try to find instance member from origin klass.");
        InstanceSymbol *inst_sym = (InstanceSymbol *)lhs_ts_sym;
        KlassSymbol *origin = (KlassSymbol *)inst_sym->origin;
        sym = stbl_get(origin->stbl, ident->name);
        if (sym) {
            // field/method of instance
            if (sym->kind == SYM_VAR) {
                log_info("found field '%s' from origin klass '%s'.", ident->name, origin->name);
                VarSymbol *origin_var_sym = (VarSymbol *)sym;
                TypeSpec *ts = instance_type_spec(origin_var_sym->ts, origin, inst_sym, ps);
                Symbol *inst_var_sym =
                    stbl_add_var(lhs_stbl, origin_var_sym->name, ts, origin_var_sym->flags);
                exp->ts = opt_dot_type(inst_var_sym->ts, opt_or_bang);
                exp->sym = inst_var_sym;
                log_info("dot member resolved: %s", inst_var_sym->name);
                log_type_spec(exp->ts);
                return;
            } else if (sym->kind == SYM_FUNC) {
                log_info("found func '%s' from origin klass '%s'.", ident->name, origin->name);
                // method of instance
                FuncSymbol *origin_fn_sym = (FuncSymbol *)sym;
                Symbol *inst_fn_sym = get_instance_method(inst_sym, ident->name, ps);
                ASSERT(inst_fn_sym);
                // copy method's tps to instance method
                // TODO: comments copy_tops will be test/test-kl/test_tuple.kl failed
                copy_tps(&((FuncSymbol *)inst_fn_sym)->tps, &origin_fn_sym->tps);
                exp->ts = opt_dot_type(inst_fn_sym->ts, opt_or_bang);
                exp->sym = inst_fn_sym;
                log_info("dot member resolved: %s", inst_fn_sym->name);
                log_type_spec(exp->ts);
                return;
            } else if (sym->kind == SYM_INHERITED) {
                InheritedFunc *inherited = (InheritedFunc *)sym;
                log_info("found inherited func '%s' from origin klass '%s'.", ident->name,
                         origin->name);

                // method of instance
                FuncSymbol *origin_fn_sym = (FuncSymbol *)sym;
                Symbol *inst_fn_sym = get_instance_method(inst_sym, ident->name, ps);
                ASSERT(inst_fn_sym);

                // copy method's tps to instance method
                // TODO: comments copy_tps here, there is nothing failed.
                // copy_tps(&((FuncSymbol *)inst_fn_sym)->tps, &origin_fn_sym->tps);
                exp->ts = opt_dot_type(inst_fn_sym->ts, opt_or_bang);
                exp->sym = inst_fn_sym;
                log_info("dot member resolved: %s", inst_fn_sym->name);
                log_type_spec(exp->ts);
                return;
            } else {
                // nothing
                UNREACHABLE();
            }
        }
    }

    if (lhs_ts_sym->kind == SYM_TYPE_PARAM) {
        Symbol *fn = get_func_from_tp(ps, (TypeParamSymbol *)lhs_ts_sym, ident->name);
        if (fn) {
            log_info("found func '%s' from type param '%s'.", ident->name, lhs_ts_sym->name);
            if (fn->kind == SYM_INHERITED) {
                exp->ts = ((InheritedFunc *)fn)->origin->ts;
            } else {
                exp->ts = opt_dot_type(fn->ts, opt_or_bang);
            }
            exp->sym = fn;
            log_info("dot member resolved: %s", fn->name);
            log_type_spec(exp->ts);
            return;
        }
    }

    char *prefix = "";

    if (lhs_ts_sym->kind == SYM_CLASS) {
        prefix = "class";
    } else if (lhs_ts_sym->kind == SYM_INSTANCE) {
        prefix = "instance of class";
    } else if (lhs_ts_sym->kind == SYM_TRAIT) {
        prefix = "trait";
    } else if (lhs_ts_sym->kind == SYM_PACKAGE) {
        prefix = "package";
    }

    kl_error(ident->loc, "'%s' is not found in %s '%s'", ident->name, prefix,
             type_spec_friendly_name(lhs_ts_sym->name));
}

static void parse_index_load(ParserState *ps, Symbol *lhs_sym, IndexExpr *index)
{
    Expr *lhs = index->lhs;

    Symbol *__fn_sym = stbl_get(lhs_sym->stbl, "__getitem__");

    if (lhs_sym->kind == SYM_INSTANCE) {
        if (!__fn_sym) {
            // try to find __getitem__ from origin klass
            InstanceSymbol *inst_sym = (InstanceSymbol *)lhs_sym;
            KlassSymbol *origin = (KlassSymbol *)inst_sym->origin;
            __fn_sym = stbl_get(origin->stbl, "__getitem__");
            if (!__fn_sym) {
                kl_error(lhs->loc, "type '%s' is not subscriptable (missing __getitem__)",
                         lhs_sym->name);
                return;
            }

            if (__fn_sym->kind == SYM_INHERITED) {
                // inherited from a base type: resolve it through the bases (no own tps)
                __fn_sym = get_instance_method(inst_sym, "__getitem__", ps);
            } else {
                // create function symbol for instance __getitem__
                Symbol *inst_fn_sym = add_instance_method(inst_sym, (FuncSymbol *)__fn_sym, ps);

                // copy __getitem__'s tps to instance __getitem__
                // TODO: comments copy_tps, some tests are failed.
                copy_tps(&((FuncSymbol *)inst_fn_sym)->tps, &((FuncSymbol *)__fn_sym)->tps);
                __fn_sym = inst_fn_sym;
            }
        }
    } else if (lhs_sym->kind == SYM_CLASS || lhs_sym->kind == SYM_TRAIT) {
        // do nothing, __getitem__ is defined on class/trait type itself, no need to
        // create new symbol for it.
    } else {
        kl_error(lhs->loc, "type '%s' is not subscriptable (missing __getitem__)", lhs_sym->name);
        return;
    }

    if (!__fn_sym) {
        kl_error(lhs->loc, "type '%s' is not subscriptable (missing __getitem__)", lhs_sym->name);
        return;
    }

    FuncSymbol *fn_sym = (FuncSymbol *)__fn_sym;

    if (func_has_infer_tp(fn_sym)) {
        Vector *_tp_args = infer_func_tp(fn_sym, index->vec, lhs->ts, ps);
        if (!_tp_args) {
            kl_error(lhs->loc, "failed to infer type parameters for function '%s'.", fn_sym->name);
            return;
        }

        Vector *params = get_func_real_params(fn_sym, _tp_args);
        check_call_args(params, index->vec, ps, lhs->loc);
        index->ts = get_func_real_ret(fn_sym, _tp_args);
        index->sym = get_symbol_by_id(index->ts->sym_id);
        vector_destroy(_tp_args);
    } else {
        index->ts = fn_sym->ret;
        index->sym = get_symbol_by_id(index->ts->sym_id);
        check_call_args(fn_sym->params, index->vec, ps, lhs->loc);
    }

    log_info("index load resolved to __getitem__:");
    log_type_spec(index->ts);
}

static void parse_index_store(ParserState *ps, Symbol *lhs_sym, IndexExpr *index)
{
    Expr *lhs = index->lhs;
    Symbol *__fn_sym = stbl_get(lhs_sym->stbl, "__setitem__");

    if (lhs_sym->kind == SYM_INSTANCE) {
        if (!__fn_sym) {
            // try to find __setitem__ from origin klass
            InstanceSymbol *inst_sym = (InstanceSymbol *)lhs_sym;
            KlassSymbol *origin = (KlassSymbol *)inst_sym->origin;
            __fn_sym = stbl_get(origin->stbl, "__setitem__");
            if (!__fn_sym) {
                kl_error(lhs->loc, "type '%s' is not subscriptable (missing __setitem__)",
                         lhs_sym->name);
                return;
            }

            if (__fn_sym->kind == SYM_INHERITED) {
                // inherited from a base type: resolve it through the bases (no own tps)
                __fn_sym = get_instance_method(inst_sym, "__setitem__", ps);
            } else {
                // create function symbol for instance __setitem__
                Symbol *inst_fn_sym = add_instance_method(inst_sym, (FuncSymbol *)__fn_sym, ps);

                // copy __setitem__'s tps to instance __setitem__
                // TODO: comments copy_tps, no failed.
                // copy_tps(&((FuncSymbol *)inst_fn_sym)->tps, &((FuncSymbol *)__fn_sym)->tps);
                __fn_sym = inst_fn_sym;
            }
        }
    } else if (lhs_sym->kind == SYM_CLASS || lhs_sym->kind == SYM_TRAIT) {
        // do nothing, __setitem__ is defined on class/trait type itself, no need to
        // create new symbol for it.
    } else {
        kl_error(lhs->loc, "type '%s' is not subscriptable (missing __setitem__)", lhs_sym->name);
        return;
    }

    if (!__fn_sym) {
        kl_error(lhs->loc, "type '%s' is not subscriptable (missing __setitem__)", lhs_sym->name);
        return;
    }

    FuncSymbol *fn_sym = (FuncSymbol *)__fn_sym;

    Vector *args = vector_create_ptr();

    ASSERT(vector_size(index->vec) == 1);
    Expr *e = vector_get(index->vec, 0);
    vector_push_back(args, &e);

    ASSERT(index->arg);
    vector_push_back(args, &index->arg);

    check_call_args(fn_sym->params, args, ps, lhs->loc);

    vector_destroy(args);

    // irgen converts the stored value to it, e.g. a class value into list[Trait]
    if (vector_size(fn_sym->params) == 2) {
        ArgInfo *value_param = vector_get(fn_sym->params, 1);
        index->value_ts = value_param->ts;
    }

    index->ts = e->ts;
    index->sym = get_symbol_by_id(index->ts->sym_id);

    log_info("index store resolved to __setitem__:");
    log_type_spec(index->ts);
}

static void parse_slice_load(ParserState *ps, Symbol *lhs_sym, IndexExpr *index)
{
    Expr *lhs = index->lhs;

    Symbol *__fn_sym = stbl_get(lhs_sym->stbl, "__getslice__");

    if (lhs_sym->kind == SYM_INSTANCE) {
        if (!__fn_sym) {
            // try to find __getslice__ from origin klass
            InstanceSymbol *inst_sym = (InstanceSymbol *)lhs_sym;
            KlassSymbol *origin = (KlassSymbol *)inst_sym->origin;
            __fn_sym = stbl_get(origin->stbl, "__getslice__");
            if (!__fn_sym) {
                kl_error(lhs->loc, "type '%s' is not subscriptable (missing __getslice__)",
                         lhs_sym->name);
                return;
            }

            if (__fn_sym->kind == SYM_INHERITED) {
                // inherited from a base type: resolve it through the bases (no own tps)
                __fn_sym = get_instance_method(inst_sym, "__getslice__", ps);
            } else {
                // create function symbol for instance __getslice__
                Symbol *inst_fn_sym = add_instance_method(inst_sym, (FuncSymbol *)__fn_sym, ps);

                // copy __getslice__'s tps to instance __getslice__
                // TODO: comments copy_tps, no failed.
                // copy_tps(&((FuncSymbol *)inst_fn_sym)->tps, &((FuncSymbol *)__fn_sym)->tps);
                __fn_sym = inst_fn_sym;
            }
        }
    } else if (lhs_sym->kind == SYM_CLASS || lhs_sym->kind == SYM_TRAIT) {
        // do nothing, __getslice__ is defined on class/trait type itself, no need to
        // create new symbol for it.
    } else {
        kl_error(lhs->loc, "type '%s' is not subscriptable (missing __getslice__)", lhs_sym->name);
        return;
    }

    if (!__fn_sym) {
        kl_error(lhs->loc, "type '%s' is not subscriptable (missing __getslice__)", lhs_sym->name);
        return;
    }

    FuncSymbol *fn_sym = (FuncSymbol *)__fn_sym;

    index->ts = fn_sym->ret;
    index->sym = get_symbol_by_id(index->ts->sym_id);
    check_call_args(fn_sym->params, index->vec, ps, lhs->loc);

    log_info("index load resolved to __getslice__:");
    log_type_spec(index->ts);
}

static void parse_slice_store(ParserState *ps, Symbol *lhs_sym, IndexExpr *index, Expr *value)
{
    Expr *lhs = index->lhs;

    Symbol *__fn_sym = stbl_get(lhs_sym->stbl, "__setslice__");

    if (lhs_sym->kind == SYM_INSTANCE) {
        if (!__fn_sym) {
            // try to find __setslice__ from origin klass
            InstanceSymbol *inst_sym = (InstanceSymbol *)lhs_sym;
            KlassSymbol *origin = (KlassSymbol *)inst_sym->origin;
            __fn_sym = stbl_get(origin->stbl, "__setslice__");
            if (!__fn_sym) {
                kl_error(lhs->loc, "type '%s' is not subscriptable (missing __setslice__)",
                         lhs_sym->name);
                return;
            }

            if (__fn_sym->kind == SYM_INHERITED) {
                // inherited from a base type: resolve it through the bases (no own tps)
                __fn_sym = get_instance_method(inst_sym, "__setslice__", ps);
            } else {
                // create function symbol for instance __setslice__
                Symbol *inst_fn_sym = add_instance_method(inst_sym, (FuncSymbol *)__fn_sym, ps);
                __fn_sym = inst_fn_sym;
            }
        }
    }

    TypeSpec *fn_ts = __fn_sym->ts;
    if (fn_ts->kind != TYPE_PROTO) {
        kl_error(lhs->loc, "symbol '%s' is not a func", __fn_sym->name);
        return;
    }

    Vector *params = fn_ts->proto_type.args;
    if (vector_size(params) != 2) {
        kl_error(lhs->loc, "func '%s' must have 2 parameters", __fn_sym->name);
        return;
    }

    TypeSpec *ret_ts = fn_ts->proto_type.ret;
    if (ret_ts->kind != TYPE_NO_TYPE) {
        kl_error(lhs->loc, "func '%s' must return None", __fn_sym->name);
        return;
    }

    TypeSpec *arg1_ts = vector_at(params, 0);
    TypeSpec *arg2_ts = vector_at(params, 1);

    Expr *arg1 = vector_at(index->vec, 0);
    if (!type_spec_compatible(arg1_ts, arg1->ts)) {
        kl_error(arg1->loc,
                 "argument type is not compatible with __setslice__ first parameter type");
    }

    if (!type_spec_compatible(arg2_ts, value->ts)) {
        kl_error(value->loc,
                 "value type is not compatible with __setslice__ second parameter type");
    }
}

/*
 * The type a type argument names. A tuple in this position is a tuple type, whose items are types,
 * and not a tuple value:
 *
 *     list[(str, int)]()            // a list of tuple[str, int]
 *     list[(str, (int, bool))]()    // the items can be tuples too
 */
static TypeSpec *type_arg_of_expr(Expr *arg, ParserState *ps)
{
    if (arg->kind == EXPR_ID_KIND) {
        Symbol *sym = find_symbol(ps, &((IdentExpr *)arg)->id);
        if (sym && sym->kind == SYM_ENUM) return sym->ts;
    }

    /* Foo[Bar?](), Foo[Bar[Z, int]?]() */
    if (arg->kind == EXPR_OPTIONAL_TYPE_KIND) {
        TypeSpec *src_ts = type_arg_of_expr(((OptionalTypeExpr *)arg)->src, ps);
        if (!src_ts) return NULL;
        return optional_type_spec_intern(src_ts);
    }

    if (arg->kind == EXPR_TUPLE_KIND) {
        Vector *item_types = vector_create_ptr();

        Expr *item;
        vector_foreach(item, ((TupleExpr *)arg)->vec) {
            TypeSpec *item_ts = type_arg_of_expr(item, ps);
            if (!item_ts) {
                vector_destroy(item_types);
                return NULL;
            }
            vector_push_back(item_types, &item_ts);
        }

        TypeSpec *infer_ts = find_lub(item_types);
        Symbol *origin = stbl_get(ps->pm->builtin, "tuple");
        InstanceSymbol *inst_sym = find_or_add_instance(ps->pm->stbl, origin, item_types, ps->pm);
        inst_sym->arg = infer_ts;
        vector_destroy(item_types);
        return inst_sym->instance_ts;
    }

    /* the symbol of a literal is the class of its value, so `1` must not be read as `int64` */
    if (arg->kind == EXPR_LITERAL_KIND) {
        kl_error(arg->loc, "type argument must be a class/trait type.");
        return NULL;
    }

    arg->ctx = EXPR_CTX_LOAD;
    parser_visit_expr(ps, arg);
    if (!arg->ts) return NULL;

    if (type_is_optional(arg->ts)) return arg->ts;

    Symbol *arg_sym = arg->sym;
    if (arg_sym && arg_sym->kind == SYM_ENUM && arg->kind == EXPR_DOT_KIND) {
        Expr *lhs = ((DotExpr *)arg)->lhs;
        if (lhs && lhs->ts && lhs->ts->kind == TYPE_PACKAGE) return arg_sym->ts;
    }

    if (arg_sym && (arg_sym->kind == SYM_CLASS || arg_sym->kind == SYM_TRAIT)) {
        return ((KlassSymbol *)arg_sym)->instance_ts;
    }

    if (arg_sym && arg_sym->kind == SYM_INSTANCE) {
        return ((InstanceSymbol *)arg_sym)->instance_ts;
    }

    if (arg_sym && arg_sym->kind == SYM_TYPE_PARAM) return arg_sym->ts;

    kl_error(arg->loc, "type argument must be a class/trait type.");
    return NULL;
}

/* Reached only outside a type argument, where type_arg_of_expr takes `src?`. */
static void parse_optional_type(ParserState *ps, Expr *exp)
{
    kl_error(exp->loc, "'?' is only allowed in a type argument.");
}

static void parse_index_new_type(ParserState *ps, IndexExpr *index)
{
    Expr *lhs = index->lhs;
    KlassSymbol *kls_sym = (KlassSymbol *)lhs->sym;

    // generic types, e.g. List[int], Dict[str, int], ListNode[T]

    if (kls_sym->kind != SYM_CLASS && kls_sym->kind != SYM_TRAIT) {
        kl_error(lhs->loc, "type '%s' is not a class or trait type.", lhs->sym->name);
        return;
    }

    int tp_size = vector_size(&kls_sym->tps);
    if (tp_size != vector_size(index->vec)) {
        if (!strcmp(kls_sym->name, "tuple")) {
            log_info("tuple is inferred type parameter, skip type argument number check.");
        } else {
            kl_error(lhs->loc, "type '%s' expects %d type arguments, but %d were provided.",
                     kls_sym->name, tp_size, vector_size(index->vec));
            return;
        }
    }

    // Phase 1: Collect all type args into tp_args

    Vector *tp_args = vector_create_ptr();
    vector_reserve(tp_args, tp_size);

    Expr *arg;
    vector_foreach(arg, index->vec) {
        if (!arg) continue;

        TypeSpec *arg_ts = type_arg_of_expr(arg, ps);
        if (!arg_ts) {
            vector_destroy(tp_args);
            return;
        }

        // Place at the correct index position
        vector_set(tp_args, i__, &arg_ts);
    }

    // Phase 2: Bound check with complete tp_args
    for (int i = 0; i < tp_size; i++) {
        TypeSpec *arg_ts = vector_get(tp_args, i);
        TypeParamSymbol *tp_sym = vector_get(&kls_sym->tps, i);
        if (!arg_ts || !tp_sym) continue;

        if (!check_generic_bound(arg_ts, tp_sym, tp_args, ps)) {
            BUF(ts_buf);
            type_spec_print(arg_ts, &ts_buf);
            kl_error(lhs->loc,
                     "type argument '%s' does not satisfy constraint on type parameter '%s'",
                     BUF_STR(ts_buf), tp_sym->name);
            FINI_BUF(ts_buf);
            vector_destroy(tp_args);
            return;
        }
    }

    // Phase 3: Create or find instance symbol(List<int>)
    InstanceSymbol *inst_sym =
        find_or_add_instance(ps->pm->stbl, (Symbol *)kls_sym, tp_args, ps->pm);
    vector_destroy(tp_args);
    index->ts = inst_sym->ts;
    index->sym = (Symbol *)inst_sym;
    log_info("generic type instance created/got: %s", inst_sym->name);
    log_type_spec(inst_sym->instance_ts);
}

/**
 * Parse explicit generic specialization: func_name[TypeArgs...]
 *
 * Handles: max[int], sort[Comparable, int], Map[string, int]
 * LHS is a generic function/type whose signature is modeled as TYPE_PROTO.
 * RHS is a type argument list (EXPR_TYPE_LIST or single EXPR_TYPE).
 *
 * Produces a TYPE_GENERIC_REF representing the specialized entity,
 * with all bounds checked against the declared type parameters.
 */
static void parse_index_func_tp(ParserState *ps, IndexExpr *index)
{
    Expr *lhs = index->lhs;

    ASSERT(lhs->ts && lhs->ts->kind == TYPE_PROTO);

    Symbol *lhs_sym = lhs->sym;
    ASSERT(lhs_sym && lhs_sym->kind == SYM_FUNC);
    FuncSymbol *fn_sym = (FuncSymbol *)lhs_sym;

    if (vector_empty(&fn_sym->tps)) {
        kl_error(lhs->loc, "func '%s' is not a generic func.", lhs_sym->name);
        return;
    }

    if (vector_size(&fn_sym->tps) != vector_size(index->vec)) {
        kl_error(lhs->loc, "func '%s' expects %d type arguments, but %d were provided.",
                 lhs_sym->name, vector_size(&fn_sym->tps), vector_size(index->vec));
        return;
    }

    Vector *tp_args = vector_create_ptr();

    Expr *arg;
    vector_foreach(arg, index->vec) {
        if (!arg) continue;

        // the same type arguments as a generic class takes: a class, a generic instance
        // (`list[str]`), a tuple type (`(int, str)`), `T?`, a type parameter
        TypeSpec *arg_ts = type_arg_of_expr(arg, ps);
        if (!arg_ts) {
            vector_destroy(tp_args);
            return;
        }
        vector_push_back(tp_args, &arg_ts);
    }

    TypeParamSymbol *tp_sym;
    vector_foreach(tp_sym, &fn_sym->tps) {
        TypeSpec *_ts = vector_get(tp_args, i__);
        if (!check_generic_bound(_ts, tp_sym, tp_args, ps)) {
            BUF(ts_buf);
            type_spec_print(_ts, &ts_buf);
            kl_error(lhs->loc,
                     "type argument '%s' does not satisfy constraint on type parameter '%s'",
                     BUF_STR(ts_buf), tp_sym->name);
            FINI_BUF(ts_buf);
            vector_destroy(tp_args);
            return;
        }
    }

    // Only a top-level generic function is routed to a specialized one by its mangled name: the
    // specialized functions are top-level, so for a method, `pass_int64` would be unrelated.
    Symbol *fn_parent = lhs_sym->parent;
    int is_method = fn_parent && (fn_parent->kind == SYM_CLASS || fn_parent->kind == SYM_INSTANCE ||
                                  fn_parent->kind == SYM_TRAIT);

    // Type args with a type variable or an optional are no concrete instantiation that has a
    // mangled name, as in find_or_add_func_instance().
    int routable = !is_method && !tps_are_generic(tp_args);
    TypeSpec *tp_arg;
    vector_foreach(tp_arg, tp_args) {
        if (type_is_optional(tp_arg)) routable = 0;
    }

    Symbol *specialized_fn_sym = NULL;
    if (routable) {
        char *mangled_name = mangle_func_name(lhs_sym->name, tp_args);
        specialized_fn_sym = stbl_get(ps->pm->stbl, mangled_name);
    }
    if (specialized_fn_sym) {
        ASSERT(specialized_fn_sym->kind == SYM_FUNC ||
               specialized_fn_sym->kind == SYM_INSTANCE_FUNC);
        index->sym = specialized_fn_sym;
        index->ts = specialized_fn_sym->ts;
        vector_destroy(tp_args);
        return;
    }

    // no specialized func, use the generic func
    // Symbol *_fn_sym = find_or_add_func_instance(fn_sym, tp_args, ps->pm->stbl, ps);
    // index->sym = _fn_sym;
    // index->ts = _fn_sym->ts;
    index->sym = lhs_sym;
    index->ts = lhs_sym->ts;
    // vector_destroy(tp_args);
    index->tp_args = tp_args;
}

static void parse_index(ParserState *ps, Expr *exp)
{
    IndexExpr *index = (IndexExpr *)exp;
    Expr *lhs = index->lhs;

    /* lhs may be NULL after the `primary_expr '.' error` recovery rule, e.g.
     * `l.0[0]`; the error is already reported. */
    if (!lhs) return;

    lhs->ctx = EXPR_CTX_LOAD;
    parser_visit_expr(ps, lhs);
    if (!lhs->ts) return;

    if (lhs->ts->kind == TYPE_TYPE) {
        log_info("index type");
        parse_index_new_type(ps, index);
        return;
    }

    if (lhs->ts->kind == TYPE_PROTO) {
        log_info("index type param");
        parse_index_func_tp(ps, index);
        return;
    }

    if (vector_size(index->vec) != 1) {
        kl_error(lhs->loc, "only single index is supported.");
        return;
    }

    Expr *arg;
    vector_foreach(arg, index->vec) {
        if (!arg) continue;
        arg->ctx = EXPR_CTX_LOAD;
        parser_visit_expr(ps, arg);
        if (!arg->ts) return;
    }

    // the symbol of the indexed type: a variable gives its type's symbol; a call result
    // has no symbol (e.g. 'r.addrs()[0]'), it gives the symbol of its result type
    Symbol *lhs_sym = lhs->sym;
    if (!lhs_sym || lhs_sym->kind == SYM_VAR || lhs_sym->kind == SYM_SHADOW_VAR) {
        TypeSpec *ts = lhs_sym ? lhs_sym->ts : lhs->ts;
        lhs_sym = get_type_symbol(ts, ps);
        if (!lhs_sym) {
            kl_error(lhs->loc, "type '%s' is not subscriptable", ts->signature);
            return;
        }
    }

    if (index->ctx == EXPR_CTX_LOAD || index->ctx == EXPR_CTX_CALL) {
        if (arg->kind == EXPR_SLICE_KIND) {
            parse_slice_load(ps, lhs_sym, index);
        } else {
            parse_index_load(ps, lhs_sym, index);
        }
        /* the result is the callee of a call, so it is a plain value rather than a
           named symbol: clearing it makes `parse_call` take the function-value path,
           and report 'expression is not callable' when the element is not a function */
        if (index->ctx == EXPR_CTX_CALL) index->sym = NULL;
    } else if (index->ctx == EXPR_CTX_STORE) {
        if (arg->kind == EXPR_SLICE_KIND) {
            parse_slice_store(ps, lhs_sym, index, exp->arg);
        } else {
            parse_index_store(ps, lhs_sym, index);
        }
    } else {
        ASSERT(index->ctx == EXPR_CTX_LOAD_STORE);
    }
}

static void parse_slice(ParserState *ps, Expr *exp)
{
    SliceExpr *slice = (SliceExpr *)exp;

    Expr *start = slice->start;
    if (start) {
        start->ctx = EXPR_CTX_LOAD;
        parser_visit_expr(ps, start);
        if (!start->ts) return;
        if (start->ts->kind != TYPE_INT) {
            kl_error(start->loc, "slice start index must be of int type.");
            return;
        }
    } else {
        start = expr_from_lit_int("-9223372036854775808", 1, 0, INT64_MIN);
        // start = expr_from_lit_int("0", 1, 0, 0);
        start->ctx = EXPR_CTX_LOAD;
        parser_visit_expr(ps, start);
        slice->start = start;
    }

    Expr *end = slice->end;
    if (end) {
        end->ctx = EXPR_CTX_LOAD;
        parser_visit_expr(ps, end);
        if (!end->ts) return;
        if (end->ts->kind != TYPE_INT) {
            kl_error(end->loc, "slice end index must be of int type.");
            return;
        }
    } else {
        end = expr_from_lit_int("-9223372036854775808", 1, 0, INT64_MIN);
        end->ctx = EXPR_CTX_LOAD;
        parser_visit_expr(ps, end);
        slice->end = end;
    }

    Expr *step = slice->step;
    if (step) {
        step->ctx = EXPR_CTX_LOAD;
        parser_visit_expr(ps, step);
        if (!step->ts) return;
        if (step->ts->kind != TYPE_INT) {
            kl_error(step->loc, "slice step index must be of int type.");
            return;
        }
    } else {
        step = expr_from_lit_int("1", 1, 0, 1);
        step->ctx = EXPR_CTX_LOAD;
        parser_visit_expr(ps, step);
        slice->step = step;
    }

    exp->ts = klass_type_spec("std/builtin", "slice");
    exp->sym = get_symbol_by_id(exp->ts->sym_id);
}

static char *get_binary_op_name(BiOpKind op)
{
    switch (op) {
        case BINARY_ADD:
            return "__add__";
        case BINARY_SUB:
            return "__sub__";
        case BINARY_MUL:
            return "__mul__";
        case BINARY_DIV:
            return "__div__";
        case BINARY_MOD:
            return "__mod__";
        case BINARY_SHL:
            return "__shl__";
        case BINARY_SHR:
            return "__shr__";

        case BINARY_BIT_AND:
            return "__bitand__";
        case BINARY_BIT_OR:
            return "__bitor__";
        case BINARY_BIT_XOR:
            return "__bitxor__";

        case BINARY_LT:
            return "__lt__";
        case BINARY_LE:
            return "__le__";
        case BINARY_GT:
            return "__gt__";
        case BINARY_GE:
            return "__ge__";
        case BINARY_EQ:
            return "__eq__";
        case BINARY_NEQ:
            return "__ne__";

        case BINARY_AND:
            return "__and__";
        case BINARY_OR:
            return "__or__";
        default:
            return "unknown_op";
    }
}

static char *get_binary_op_str(BiOpKind op)
{
    switch (op) {
        case BINARY_ADD:
            return "+";
        case BINARY_SUB:
            return "-";
        case BINARY_MUL:
            return "*";
        case BINARY_DIV:
            return "/";
        case BINARY_MOD:
            return "%";
        case BINARY_SHL:
            return "<<";
        case BINARY_SHR:
            return ">>";
        case BINARY_BIT_AND:
            return "&";
        case BINARY_BIT_OR:
            return "|";
        case BINARY_BIT_XOR:
            return "^";
        case BINARY_LT:
            return "<";
        case BINARY_LE:
            return "<=";
        case BINARY_GT:
            return ">";
        case BINARY_GE:
            return ">=";
        case BINARY_EQ:
            return "==";
        case BINARY_NEQ:
            return "!=";
        case BINARY_AND:
            return "&&";
        case BINARY_OR:
            return "||";
        default:
            UNREACHABLE();
            return "";
    }
}

/*
 * The result type of `-x` / `~x` on a number: the return type of its `__neg__` / `__bitnot__`
 * in `std/builtin/number.kl`, e.g. `int8.__neg__() int64`, `uint64.__neg__() uint64`.
 */
static TypeSpec *number_unary_ret(ParserState *ps, UnaryExpr *unary, TypeSpec *ts, char *op_name, char *op_str)
{
    Symbol *sym = get_symbol_by_id(ts->sym_id);
    FuncSymbol *fn = NULL;
    if (sym && sym->kind == SYM_CLASS) {
        fn = (FuncSymbol *)stbl_get(((KlassSymbol *)sym)->stbl, op_name);
    }

    if (!fn || fn->kind != SYM_FUNC || vector_size(fn->params) != 0 || !fn->ret) {
        kl_error(unary->op_loc, "operator '%s' is not defined in type '%s'.", op_str,
                 type_spec_friendly_name(sym ? sym->name : "?"));
        return NULL;
    }

    log_info("unary operator '%s' for type '%s' resolved.", op_str, sym->name);
    log_type_spec(fn->ret);
    return fn->ret;
}

static void parse_unary(ParserState *ps, Expr *exp)
{
    UnaryExpr *unary = (UnaryExpr *)exp;
    UnOpKind op = unary->op;

    Expr *e = unary->exp;

    /* operand may be NULL after the `primary_expr '.' error` recovery rule,
     * e.g. `-l.0`; the error is already reported. */
    if (!e) return;

    e->ctx = EXPR_CTX_LOAD;

    parser_visit_expr(ps, e);
    if (!e->ts) return;

    if (type_is_enum(e->ts)) {
        kl_error(unary->op_loc, "enum does not support unary operators.");
        return;
    }

    if (op == UNARY_PLUS) {
        if (e->ts->kind != TYPE_INT && e->ts->kind != TYPE_FLOAT) {
            kl_error(unary->op_loc, "unary '+' operator requires int or float type.");
            return;
        }
        exp->ts = e->ts;
    } else if (op == UNARY_NEG) {
        if (e->ts->kind != TYPE_INT && e->ts->kind != TYPE_FLOAT) {
            kl_error(unary->op_loc, "unary '-' operator requires int or float type.");
            return;
        }

        exp->ts = number_unary_ret(ps, unary, e->ts, "__neg__", "-");
    } else if (op == UNARY_BIT_NOT) {
        if (e->ts->kind != TYPE_INT) {
            kl_error(unary->op_loc, "unary '~' operator requires int type.");
            return;
        }

        exp->ts = number_unary_ret(ps, unary, e->ts, "__bitnot__", "~");
    } else if (op == UNARY_NOT) {
        if (e->ts->kind != TYPE_BOOL) {
            kl_error(unary->op_loc, "unary '!' operator requires bool type.");
            return;
        }
        exp->ts = bool_type_spec();
        log_info("unary '!' operator resolved.");
        log_type_spec(exp->ts);
    } else {
        UNREACHABLE();
    }
}

/* The value of a literal, a negated literal or a literal `const`; returns 0 for anything else. */
static int shift_count_value(Expr *e, int *negative, uint64_t *value)
{
    if (e->kind == EXPR_UNARY_KIND) {
        UnaryExpr *unary = (UnaryExpr *)e;
        if (unary->op != UNARY_NEG || !unary->exp) return 0;

        int inner_negative = 0;
        if (!shift_count_value(unary->exp, &inner_negative, value)) return 0;

        *negative = !inner_negative && *value != 0;
        return 1;
    }

    uint64_t ival = 0;
    int sign = 0;

    if (e->kind == EXPR_LITERAL_KIND) {
        LitExpr *lit = (LitExpr *)e;
        if (lit->which != LIT_EXPR_INT) return 0;
        ival = lit->ival;
        sign = lit->sign;
    } else {
        Symbol *sym = e->sym;
        if (!sym || sym->kind != SYM_VAR || !(sym->flags & SYM_FLAGS_CONST)) return 0;

        Literal *lit = ((VarSymbol *)sym)->lit;
        if (!lit || lit->which != LIT_INT) return 0;
        ival = lit->ival;
        sign = lit->sign;
    }

    *negative = sign && (int64_t)ival < 0;
    *value = *negative ? 0 - ival : ival;
    return 1;
}

/*
 * Check a constant shift count of a built-in integer, whatever the left side is: integers are
 * promoted to 64 bits, so the count must be in [0, 64). A class may give `__shl__` its own meaning,
 * and a count that is not constant, or a trait value, is checked at run time.
 */
void check_shift_count(ParserState *ps, TypeSpec *lhs_ts, Expr *count)
{
    if (lhs_ts->kind != TYPE_INT) return;

    int negative = 0;
    uint64_t value = 0;
    if (!shift_count_value(count, &negative, &value)) return;

    if (negative) {
        kl_error(count->loc, "negative shift count: -%" PRIu64, value);
    } else if (value >= 64) {
        kl_error(count->loc, "shift count out of range: %" PRIu64, value);
    } else if (value == 0) {
        kl_warn(count->loc, "shift count is 0");
    }
}

static void parse_binary(ParserState *ps, Expr *exp)
{
    BinaryExpr *bin = (BinaryExpr *)exp;
    BiOpKind op = bin->op;
    Expr *lhs = bin->lhs;
    Expr *rhs = bin->rhs;

    /* an operand may be NULL after the `primary_expr '.' error` recovery rule,
     * e.g. `l.0 + 1`; the error is already reported. */
    if (!lhs || !rhs) return;

    lhs->ctx = EXPR_CTX_LOAD;
    parser_visit_expr(ps, lhs);

    rhs->ctx = EXPR_CTX_LOAD;
    if (rhs->kind == EXPR_LITERAL_KIND && lhs->ts && type_is_uint64(lhs->ts)) {
        rhs->expected = lhs->ts;
    }
    parser_visit_expr(ps, rhs);

    if (!lhs->ts || !rhs->ts) return;

    if (rhs->kind == EXPR_ID_KIND) {
        Symbol *rhs_sym = rhs->sym;
        if (!type_is_uint64(lhs->ts) && rhs_sym && rhs_sym->flags & SYM_FLAGS_CONST) {
            rhs->expected = lhs->ts;
            // re-parse the rhs expression with the expected type set to lhs->ts
            parser_visit_expr(ps, rhs);
        }
    }

    if (!lhs->ts || !rhs->ts) return;

    if (type_is_enum(lhs->ts) || type_is_enum(rhs->ts)) {
        if (op != BINARY_EQ && op != BINARY_NEQ) {
            kl_error(bin->op_loc, "enum only supports '==' and '!=', not '%s'.",
                     get_binary_op_str(op));
            return;
        }
        if (type_is_enum(lhs->ts) && type_is_enum(rhs->ts) && lhs->ts != rhs->ts) {
            kl_error(bin->op_loc, "cannot compare different enums.");
            return;
        }
    }

    TypeSpec *lhs_op_ts = type_is_enum(lhs->ts) ? int64_type_spec() : lhs->ts;
    TypeSpec *rhs_op_ts = type_is_enum(rhs->ts) ? int64_type_spec() : rhs->ts;

    if (op == BINARY_SHL || op == BINARY_SHR) check_shift_count(ps, lhs_op_ts, rhs);

    if (type_is_optional(lhs_op_ts) && (op != BINARY_EQ && op != BINARY_NEQ)) {
        Symbol *opt_sym = lhs->sym;
        if (opt_sym && opt_sym->kind == SYM_SHADOW_VAR) {
            // check shadow variable nil state
            ShadowVarSymbol *shadow_sym = (ShadowVarSymbol *)opt_sym;
            log_info("  note: symbol '%s' is a shadow variable.", opt_sym->name);
            log_info("  value is nil: %s", shadow_sym->is_null ? "true" : "false");
            if (shadow_sym->is_null) {
                kl_error(bin->op_loc,
                         "optional type cannot be used with '%s' operator when value "
                         "is nil.",
                         get_binary_op_str(op));
                return;
            }
        }

        kl_error(bin->op_loc, "optional type cannot be used with '%s' operator.",
                 get_binary_op_str(op));
        return;
    }

    if (op == BINARY_EQ || op == BINARY_NEQ) {
        // special handling for optional and nil comparison
        if (type_is_optional(lhs_op_ts) && type_is_optional(rhs_op_ts)) {
            if (expr_is_literal_null(lhs) || expr_is_literal_null(rhs)) {
                // allow optional type compared with nil literal
                exp->ts = bool_type_spec();
                log_info("binary operator '%s' resolved.", get_binary_op_str(op));
                log_type_spec(exp->ts);
                return;
            }

            if (!type_spec_compatible(lhs_op_ts, rhs_op_ts)) {
                kl_error(lhs->loc, "two optional types are not compatible for '%s' operator.",
                         get_binary_op_str(op));
                log_info("lhs type:");
                log_type_spec(lhs_op_ts);
                log_info("rhs type:");
                log_type_spec(rhs_op_ts);
            } else {
                exp->ts = bool_type_spec();
                log_info("binary operator '%s' resolved.", get_binary_op_str(op));
                log_type_spec(exp->ts);
            }
            return;
        }

        if (type_is_optional(lhs_op_ts) && !type_is_optional(rhs_op_ts)) {
            kl_error(bin->op_loc,
                     "cannot compare optional type with non-optional type for '%s' "
                     "operator.",
                     get_binary_op_str(op));
            return;
        }

        if (!type_is_optional(lhs_op_ts) && type_is_optional(rhs_op_ts)) {
            kl_error(bin->op_loc,
                     "cannot compare non-optional type with optional type for '%s' "
                     "operator.",
                     get_binary_op_str(op));
            return;
        }

        // fall through to normal operator resolution
    }

    Symbol *sym = get_symbol_by_id(lhs_op_ts->sym_id);
    if (!sym) {
        kl_error(bin->op_loc, "type is not found.");
        return;
    }

    /* `&&` / `||` are bool-only and are not overloadable (only bool may define `__and__` /
     * `__or__`): fail early instead of emitting a non-bool branch.
     */
    if ((op == BINARY_AND || op == BINARY_OR) && !type_is_bool(lhs_op_ts)) {
        kl_error(bin->op_loc, "operator '%s' is not defined in type '%s'.", get_binary_op_str(op),
                 type_spec_friendly_name(sym->name));
        return;
    }

    Symbol *fn = NULL;
    char *op_name = get_binary_op_name(op);

    if (sym->kind == SYM_TYPE_PARAM) {
        fn = get_func_from_tp(ps, (TypeParamSymbol *)sym, op_name);
        if (!fn) {
            kl_error(bin->op_loc, "operator '%s' is not defined for type parameter '%s'.",
                     get_binary_op_str(op), type_spec_friendly_name(sym->name));
            return;
        }

        // the automatic Equatable: the operands are two values of the type parameter,
        // compared at runtime by their own __eq__ / __ne__ (irgen emits a plain compare)
        if (is_auto_trait_func(ps, fn)) {
            if (!type_spec_compatible(lhs_op_ts, rhs_op_ts)) {
                kl_error(bin->op_loc, "argument type mismatch for operator '%s' of type '%s'.",
                         get_binary_op_str(op), type_spec_friendly_name(sym->name));
                return;
            }
            exp->ts = bool_type_spec();
            return;
        }
    } else if (sym->kind == SYM_INSTANCE) {
        // trait-typed operand, e.g. 'a Arithmetic[int]': resolve the operator
        // dunder through the instance interface like an explicit member call
        fn = get_instance_method((InstanceSymbol *)sym, op_name, ps);

        if (!fn) {
            kl_error(bin->op_loc, "operator '%s' is not defined in type '%s'.",
                     get_binary_op_str(op), type_spec_friendly_name(sym->name));
            return;
        }
    } else {
        HashMap *stbl = ((KlassSymbol *)sym)->stbl;
        fn = stbl_get(stbl, op_name);

        if (!fn) {
            kl_error(bin->op_loc, "operator '%s' is not defined in type '%s'.",
                     get_binary_op_str(op), type_spec_friendly_name(sym->name));
            return;
        }
    }

    FuncSymbol *fn_sym = (FuncSymbol *)fn;

    Vector *args = fn_sym->params;
    if (vector_size(args) != 1) {
        kl_error(bin->op_loc, "expected %d argument for operator '%s' of type '%s', but got 1.",
                 vector_size(args), get_binary_op_str(op), type_spec_friendly_name(sym->name));
        return;
    }

    ArgInfo *arg_info = vector_get(args, 0);
    ASSERT(arg_info);

    TypeSpec *arg_ts = arg_info->ts;
    if (!type_spec_compatible(arg_ts, rhs_op_ts)) {
        kl_error(bin->op_loc, "argument type mismatch for operator '%s' of type '%s'.",
                 get_binary_op_str(op), type_spec_friendly_name(sym->name));
        log_info("expected type:");
        log_type_spec(arg_ts);
        log_info("actual type:");
        log_type_spec(rhs_op_ts);
        return;
    }

    exp->ts = fn_sym->ret;

    log_info("binary operator '%s' for type '%s' resolved.", get_binary_op_str(op), sym->name);
    log_type_spec(exp->ts);
}

static void parse_keyword(ParserState *ps, Expr *exp)
{
    KeyWordExpr *kw = (KeyWordExpr *)exp;
    Expr *e = kw->value;
    e->ctx = EXPR_CTX_LOAD;
    parser_visit_expr(ps, e);
    if (!e->ts) return;
    exp->ts = e->ts;
}

static void parse_bang(ParserState *ps, Expr *exp)
{
    BangExpr *bang = (BangExpr *)exp;
    Expr *e = bang->exp;
    e->ctx = EXPR_CTX_LOAD;
    parser_visit_expr(ps, e);
    if (!e->ts) return;

    TypeSpec *it_ts;

    if (!type_is_optional(e->ts)) {
        Symbol *sym = e->sym;
        /* a call result or other temporary has no symbol */
        if (sym && sym->kind == SYM_SHADOW_VAR) {
            // check shadow variable nil state
            ShadowVarSymbol *shadow_sym = (ShadowVarSymbol *)sym;
            log_info("  note: symbol '%s' is a shadow variable.", sym->name);
            log_info("  value is nil: %s", shadow_sym->is_null ? "true" : "false");
            if (shadow_sym->is_null) {
                kl_error(bang->loc, "bang operator cannot be applied when value is nil.");
                return;
            } else {
                kl_warn(bang->loc, "bang operator applied on non-nullable variable('%s').",
                        sym->name);
                log_info("bang operator resolved on shadow variable, value is not nil.");
                log_type_spec(exp->ts);
                exp->ts = e->ts;
                exp->sym = e->sym;
                return;
            }
        } else if (match_sequence(e->ts, &it_ts, NULL)) {
            exp->ts = it_ts;
            exp->sym = get_symbol_by_id(exp->ts->sym_id);
            log_info("update bang expr type as sequence type, unwrap sequence object.");
            log_type_spec(exp->ts);
            return;
        } else {
            kl_error(bang->loc, "only optional type/Sequence's subtype can use bang operator.");
            return;
        }
    }

    exp->ts = e->ts->opt.src;
    /* An unwrapped function value is callable by its prototype, not by the
       optional variable symbol that held it. */
    exp->sym = (exp->ctx == EXPR_CTX_CALL && type_is_proto(exp->ts)) ? NULL : e->sym;
    log_info("bang operator resolved, unwrap optional type.");
    log_type_spec(exp->ts);
}

static int check_type_cast(TypeSpec *src, TypeSpec *target, ParserState *ps)
{
    if (src == target) {
        log_info("type cast is valid, source and target type are the same. cast is safe.");
        return 1;
    }

    if (target == any_type_spec()) {
        log_info("type cast is valid, target type is 'any'. cast is safe.");
        return 1;
    }

    if (type_spec_equal_exact(src, target)) {
        log_info("type cast is valid, source and target type are equal. cast is safe.");
        return 1;
    }

    Symbol *src_sym = get_symbol_by_id(src->sym_id);
    ASSERT(src_sym->kind == SYM_CLASS || src_sym->kind == SYM_INSTANCE ||
           src_sym->kind == SYM_TRAIT);
    Vector *bases = &((KlassSymbol *)src_sym)->bases;
    if (src_sym->kind == SYM_INSTANCE) bases = ((InstanceSymbol *)src_sym)->bases;
    if (!bases) return 0;

    TypeSpec *ts;
    vector_foreach(ts, bases) {
        if (!ts) continue;
        if (ts == target) {
            log_info("type cast is valid, '%s' is a subtype of '%s'. cast is safe.", src->signature,
                     target->signature);
            return 1;
        }

        Symbol *base_sym = get_symbol_by_id(ts->sym_id);
        ASSERT(base_sym->kind == SYM_CLASS || base_sym->kind == SYM_INSTANCE ||
               base_sym->kind == SYM_TRAIT);
        if (check_type_cast(ts, target, ps)) return 1;
    }

    log_info(
        "checking type cast from '%s' to '%s', not found in base types. \nMaybe throw "
        "exception at runtime.",
        src->signature, target->signature);
    return 0;
}

static int is_class_type_symbol(Symbol *sym)
{
    if (!sym) return 0;

    if (sym->kind == SYM_CLASS) return 1;

    if (sym->kind == SYM_INSTANCE) {
        InstanceSymbol *instance = (InstanceSymbol *)sym;
        return instance->origin && instance->origin->kind == SYM_CLASS;
    }

    return 0;
}

static int is_trait_type_symbol(Symbol *sym)
{
    if (!sym) return 0;

    if (sym->kind == SYM_TRAIT) return 1;

    if (sym->kind == SYM_INSTANCE) {
        InstanceSymbol *instance = (InstanceSymbol *)sym;
        return instance->origin && instance->origin->kind == SYM_TRAIT;
    }

    return 0;
}

static int union_has_member(TypeSpec *union_ts, TypeSpec *target_ts)
{
    TypeSpec *member;
    vector_foreach(member, union_ts->union_type.args) {
        if (member->sym_id == target_ts->sym_id) return 1;
    }

    return 0;
}

/*
 * Union members and target must be concrete class types.
 * If the target type is not a member of the union, it is a type error.
 * If the target type is a member of the union, a runtime check is still required.
 */
static int validate_union_target(ParserState *ps, Loc loc, TypeSpec *src_ts, TypeSpec *target_ts)
{
    TypeSpec *inner_ts = type_is_optional(src_ts) ? src_ts->opt.src : src_ts;
    if (inner_ts->kind != TYPE_UNION) return 1;

    if (union_has_member(inner_ts, target_ts)) return 1;

    kl_error(loc, "target type '%s' is not a member of union type '%s'.", target_ts->signature,
             inner_ts->signature);
    return 0;
}

/*
 * Determines whether the result is statically known or requires a runtime check.
 * Union membership is validated separately before calling this function.
 */
static IsAsResult resolve_cast_check(Expr *src_exp, TypeSpec *src_ts, TypeSpec *target_ts,
                                     ParserState *ps)
{
    int optional = type_is_optional(src_ts);
    TypeSpec *inner_ts = optional ? src_ts->opt.src : src_ts;

    if (inner_ts->kind == TYPE_UNION) {
        return RUNTIME_CHECK;
    }

    // `any` may hold any runtime class, so it cannot be statically classified.
    if (inner_ts == any_type_spec()) {
        return RUNTIME_CHECK;
    }

    Symbol *src_sym = get_symbol_by_id(inner_ts->sym_id);
    if (!src_sym) return RUNTIME_CHECK;

    if (!is_class_type_symbol(src_sym) && !is_trait_type_symbol(src_sym)) {
        return RUNTIME_CHECK;
    }

    // A nullable source may be nil, so even a provable upcast needs a runtime check.
    if (check_type_cast(inner_ts, target_ts, ps)) {
        return optional ? RUNTIME_CHECK : ALWAYS_TRUE;
    }

    if (is_trait_type_symbol(src_sym)) {
        /*
         * A trait value may contain a target class only if that class implements
         * the source trait. If it does, the runtime value still determines the result.
         */
        return check_type_cast(target_ts, inner_ts, ps) ? RUNTIME_CHECK : ALWAYS_FALSE;
    }

    /*
     * The source is a concrete class (including a specialized class instance).
     * Class inheritance is unsupported, so a failed upcast means it cannot be
     * the target class.
     */
    return ALWAYS_FALSE;
}

static char *result_string(IsAsResult result)
{
    switch (result) {
        case RUNTIME_CHECK:
            return "Runtime Check";
        case ALWAYS_TRUE:
            return "Always True";
        case ALWAYS_FALSE:
            return "Always False";
        default:
            UNREACHABLE();
            return "Unknown";
    }
}

static void warn_static_cast(ParserState *ps, Loc loc, const char *op, IsAsResult result,
                             TypeSpec *src_ts, TypeSpec *target_ts)
{
    if (result == ALWAYS_TRUE) {
        kl_warn(loc, "'%s' check is always true: '%s' is always '%s'.", op, src_ts->signature,
                target_ts->signature);
    } else if (result == ALWAYS_FALSE) {
        kl_warn(loc, "'%s' check is always false: '%s' can never be '%s'.", op, src_ts->signature,
                target_ts->signature);
    }
}

static int validate_cast_target(ParserState *ps, Loc loc, const char *op, TypeSpec *target_ts)
{
    if (target_ts->kind == TYPE_OPTIONAL || target_ts->kind == TYPE_UNION) {
        kl_error(loc, "target type of '%s' operator must be a class type.", op);
        return 0;
    }

    Symbol *target_sym = get_symbol_by_id(target_ts->sym_id);
    if (!target_sym) {
        kl_error(loc, "type is not found.");
        return 0;
    }

    if (!is_class_type_symbol(target_sym)) {
        kl_error(loc, "target type of '%s' operator must be a class type.", op);
        return 0;
    }

    if (!strcmp(target_sym->name, "NilType")) {
        kl_error(loc, "target type of '%s' operator cannot be NilType.", op);
        return 0;
    }

    return 1;
}

static int check_is_src_type(TypeSpec *src_ts)
{
    if (src_ts->kind == TYPE_NO_TYPE || src_ts->kind == TYPE_VA_LIST ||
        src_ts->kind == TYPE_PACKAGE || src_ts->kind == TYPE_PROTO ||
        src_ts->kind == TYPE_UNRESOLVED || src_ts->kind == TYPE_MANGLED ||
        src_ts->kind == TYPE_GENERIC_VAR) {
        return 0;
    }

    if (type_is_optional(src_ts)) return 0;

    return 1;
}

// A builtin value type (int, str, ...) can never be narrowed by 'as'.
static int check_as_src_type(TypeSpec *src_ts)
{
    if (!check_is_src_type(src_ts)) return 0;

    return !(src_ts->kind == TYPE_INT || src_ts->kind == TYPE_FLOAT ||
             src_ts->kind == TYPE_BFLOAT16 || src_ts->kind == TYPE_BOOL ||
             src_ts->kind == TYPE_STR);
}

// Two instances of the same generic class with different type arguments,
// e.g. list[int] and list[str].
static int is_different_instance_of_same_origin(TypeSpec *src_ts, TypeSpec *target_ts)
{
    Symbol *src_sym = get_symbol_by_id(src_ts->sym_id);
    Symbol *target_sym = get_symbol_by_id(target_ts->sym_id);
    if (!src_sym || !target_sym) return 0;
    if (src_sym->kind != SYM_INSTANCE || target_sym->kind != SYM_INSTANCE) return 0;
    if (src_sym == target_sym) return 0;

    return ((InstanceSymbol *)src_sym)->origin == ((InstanceSymbol *)target_sym)->origin;
}

static void parse_as(ParserState *ps, Expr *exp)
{
    AsExpr *as = (AsExpr *)exp;
    if (exp->ts) {
        log_info("'as' operator is already resolved, result = %s.", result_string(as->result));
        return;
    }

    Expr *e = as->exp;
    e->ctx = EXPR_CTX_LOAD;
    parser_visit_expr(ps, e);
    if (!e->ts) return;

    if (!check_as_src_type(e->ts)) {
        kl_error(as->loc, "src of 'as' operator is not a valid type.");
        return;
    }

    Loc loc = as->type->loc;

    TypeSpec *target_ts = resolve_type(ps, as->type);
    if (!target_ts) {
        kl_error(loc, "type is not found.");
        return;
    }

    if (!validate_cast_target(ps, loc, "as", target_ts)) return;
    if (!validate_union_target(ps, loc, e->ts, target_ts)) return;

    if (is_different_instance_of_same_origin(e->ts, target_ts)) {
        BUF(src_buf);
        BUF(target_buf);
        type_spec_print(e->ts, &src_buf);
        type_spec_print(target_ts, &target_buf);
        kl_error(loc, "cannot cast '%s' to '%s' with 'as': type arguments are different.",
                 BUF_STR(src_buf), BUF_STR(target_buf));
        FINI_BUF(src_buf);
        FINI_BUF(target_buf);
        return;
    }

    as->target_ts = target_ts;
    as->result = resolve_cast_check(e, e->ts, target_ts, ps);
    warn_static_cast(ps, loc, "as", as->result, e->ts, target_ts);

    // A statically successful cast has type T; a runtime or failed cast has type T?.
    exp->ts = as->result == ALWAYS_TRUE ? target_ts : optional_type_spec_intern(target_ts);
    exp->sym = get_symbol_by_id(exp->ts->sym_id);

    Symbol *target_sym = get_symbol_by_id(target_ts->sym_id);
    if (!target_sym) {
        kl_error(loc, "type is not found.");
        return;
    }
    as->target_sym = target_sym;

    log_info("'as' operator resolved for target '%s': %s.", target_ts->signature,
             result_string(as->result));
}

static void parse_is(ParserState *ps, Expr *exp)
{
    IsExpr *is = (IsExpr *)exp;
    if (exp->ts) {
        log_info("'is' operator is already resolved, result = %s.", result_string(is->result));
        return;
    }

    Expr *e = is->exp;
    e->ctx = EXPR_CTX_LOAD;
    parser_visit_expr(ps, e);
    if (!e->ts) return;

    if (!check_is_src_type(e->ts)) {
        kl_error(is->loc, "src of 'is' operator is not a valid type.");
        return;
    }

    Loc loc = is->type->loc;

    TypeSpec *target_ts = resolve_type(ps, is->type);
    if (!target_ts) {
        kl_error(loc, "type is not found.");
        return;
    }

    if (!validate_cast_target(ps, loc, "is", target_ts)) return;
    if (!validate_union_target(ps, loc, e->ts, target_ts)) return;

    is->target_ts = target_ts;
    is->result = resolve_cast_check(e, e->ts, target_ts, ps);
    warn_static_cast(ps, loc, "is", is->result, e->ts, target_ts);

    exp->ts = bool_type_spec();
    exp->sym = get_symbol_by_id(exp->ts->sym_id);

    Symbol *target_sym = get_symbol_by_id(target_ts->sym_id);
    if (!target_sym) {
        kl_error(loc, "type is not found.");
        return;
    }
    is->target_sym = target_sym;

    log_info("'is' operator resolved for target '%s': %s.", target_ts->signature,
             result_string(is->result));
}

static void parse_in(ParserState *ps, Expr *exp)
{
    InExpr *in = (InExpr *)exp;
    Expr *e = in->lhs;
    e->ctx = EXPR_CTX_LOAD;
    parser_visit_expr(ps, e);
    if (!e->ts) return;

    Expr *container = in->rhs;
    container->ctx = EXPR_CTX_LOAD;
    parser_visit_expr(ps, container);
    if (!container->ts) return;

    Symbol *container_ts_sym = get_symbol_by_id(container->ts->sym_id);
    ASSERT(container_ts_sym);

    Symbol *_fn_sym = NULL;

    if (container_ts_sym->kind == SYM_INSTANCE) {
        _fn_sym = get_instance_method((InstanceSymbol *)container_ts_sym, "__contains__", ps);
    } else if (container_ts_sym->kind == SYM_CLASS || container_ts_sym->kind == SYM_TRAIT) {
        _fn_sym = stbl_get(container_ts_sym->stbl, "__contains__");
    }

    if (!_fn_sym) {
        kl_error(in->op_loc, "type '%s' does not support '__contains__' method.",
                 container_ts_sym->name);
        return;
    }

    if (_fn_sym->kind != SYM_FUNC) {
        kl_error(in->op_loc, "'__contains__' is not a function.");
        return;
    }

    Vector *params = ((FuncSymbol *)_fn_sym)->params;
    Vector _args;
    vector_init_ptr(&_args);
    vector_push_back(&_args, &e);
    check_call_args(params, &_args, ps, in->op_loc);
    vector_fini(&_args);

    if (ps->errors > 0) return;

    exp->ts = bool_type_spec();
    exp->sym = get_symbol_by_id(exp->ts->sym_id);
    log_info("'in' operator resolved.");
}

static void parse_const_placeholder(ParserState *ps, Expr *exp)
{
    ConstPlaceholderExpr *cp = (ConstPlaceholderExpr *)exp;
    log_info("const placeholder: %s", cp->name);

    if (exp->ctx != EXPR_CTX_LOAD) {
        kl_error(exp->loc, "const placeholder is readonly.");
        return;
    }

    ConstPlaceHolder *cph = get_const_placeholder(cp->name);
    if (!cph) {
        kl_error(cp->loc, "const '%s' is not found.", cp->name);
        return;
    }

    cp->ts = cph->ts;
    cp->lit = &cph->lit;
    exp->sym = get_symbol_by_id(exp->ts->sym_id);
}

// The type of `if c { a } else { b }` without an expected type: the same type; with a 'nil'
// branch, the other one as optional; otherwise the branch type the other one fits into
// (T -> T?, class -> trait). NULL when the branches do not fit together.
static TypeSpec *if_expr_result_type(TypeSpec *a, TypeSpec *b)
{
    // type specs are interned
    if (a == b) return a;

    int a_nil = type_is_optional(a) && !a->opt.src;
    int b_nil = type_is_optional(b) && !b->opt.src;
    if (a_nil && b_nil) return a;
    if (a_nil) return type_is_optional(b) ? b : optional_type_spec_intern(b);
    if (b_nil) return type_is_optional(a) ? a : optional_type_spec_intern(a);

    if (type_spec_compatible(a, b)) return a;
    if (type_spec_compatible(b, a)) return b;
    return NULL;
}

// if c { a } else { b } as a value: only the grammar's value positions reach here
static void parse_if_expr(ParserState *ps, Expr *exp)
{
    IfExpr *ie = (IfExpr *)exp;

    // 'else' is missing, the grammar has reported it
    if (!ie->else_exp) return;

    // 1. the condition is a bool
    Expr *cond = ie->cond;
    cond->ctx = EXPR_CTX_LOAD;
    parser_visit_expr(ps, cond);
    if (!cond->ts) return;
    if (cond->ts->kind != TYPE_BOOL) {
        kl_error(cond->loc, "if condition must be boolean type.");
        return;
    }

    // 2. both branches get the expected type, e.g. `let x Fly = if c { Bird() } else { Fish() }`;
    //    the condition narrows them as in an 'if' statement: `if v == nil { 0 } else { v.len() }`
    Expr *then_exp = ie->then_exp;
    Expr *else_exp = ie->else_exp;

    enter_scope(ps, SCOPE_BLOCK, IF_BLOCK, "if-expr-then");

    Vector shadows = VECTOR_INIT_PTR;

    unwrap_for_if(ps, cond, &shadows);
    then_exp->ctx = EXPR_CTX_LOAD;
    then_exp->expected = exp->expected;
    parser_visit_expr(ps, then_exp);

    exit_scope(ps);

    enter_scope(ps, SCOPE_BLOCK, ELSE_BLOCK, "if-expr-else");

    inherit_if_shadows(ps, &shadows);
    else_exp->ctx = EXPR_CTX_LOAD;
    else_exp->expected = exp->expected;
    parser_visit_expr(ps, else_exp);

    exit_scope(ps);

    if (!then_exp->ts || !else_exp->ts) return;

    // 3. the result type: the expected type, or the one both branches fit into
    TypeSpec *ts = exp->expected;
    if (ts) {
        if (!type_spec_compatible(ts, then_exp->ts)) {
            kl_error_incompatible_type(then_exp->loc, ts, then_exp->ts);
            return;
        }
        if (!type_spec_compatible(ts, else_exp->ts)) {
            kl_error_incompatible_type(else_exp->loc, ts, else_exp->ts);
            return;
        }
    } else {
        ts = if_expr_result_type(then_exp->ts, else_exp->ts);
        if (!ts) {
            BUF(buf1);
            type_spec_print(then_exp->ts, &buf1);
            BUF(buf2);
            type_spec_print(else_exp->ts, &buf2);
            kl_error(exp->loc, "branches of 'if' expression have incompatible types '%s' and '%s'.",
                     BUF_STR(buf1), BUF_STR(buf2));
            FINI_BUF(buf1);
            FINI_BUF(buf2);
            return;
        }
    }

    exp->ts = ts;
    exp->sym = get_symbol_by_id(ts->sym_id);
    log_info("'if' expression resolved:");
    log_type_spec(exp->ts);
}

static void parse_coalesce(ParserState *ps, Expr *exp)
{
    CoalesceExpr *co = (CoalesceExpr *)exp;
    Expr *lhs = co->lhs;
    Expr *rhs = co->rhs;

    /* operands may be NULL after the `coalesce_expr COALESCE error` recovery
     * rule; the error is already reported. */
    if (!lhs || !rhs) return;

    lhs->ctx = EXPR_CTX_LOAD;
    parser_visit_expr(ps, lhs);
    if (!lhs->ts) return;

    /* `src_ts` is the unwrapped value type of lhs:
     *   - lhs is `T?`         => src_ts = T
     *   - lhs is non-optional => rejected: `??` requires an optional lhs, a
     *                            never-nil lhs makes the rhs dead code; the
     *                            only exception is a shadow variable known to
     *                            be nil, whose static type is non-optional
     *   - lhs is bare `nil`   => src_ts = NULL, the result takes rhs type */
    TypeSpec *src_ts = NULL;

    if (!type_is_optional(lhs->ts)) {
        Symbol *sym = lhs->sym;
        if (sym && sym->kind == SYM_SHADOW_VAR) {
            /* unlike `!`, a shadow variable known to be nil is a legitimate
             * `\?\?` operand: nil-coalescing is designed exactly for that;
             * a shadow variable known to be non-nil is rejected like any
             * other never-nil lhs. */
            ShadowVarSymbol *shadow_sym = (ShadowVarSymbol *)sym;
            log_info("  note: symbol '%s' is a shadow variable.", sym->name);
            log_info("  value is nil: %s", shadow_sym->is_null ? "true" : "false");
            if (!shadow_sym->is_null) {
                kl_error(co->loc, "'\?\?' operator cannot be applied when value is never nil.");
                return;
            }
        } else {
            /* the rhs would be dead code: `??` on a value that can never be
             * nil is meaningless, reject it instead of silently degrading. */
            kl_error(co->loc, "left operand of '\?\?' must be an optional type.");
            return;
        }
        src_ts = lhs->ts;
    } else {
        src_ts = lhs->ts->opt.src;
    }

    /* give rhs a type hint so literal defaults get the concrete type */
    rhs->ctx = EXPR_CTX_LOAD;
    if (src_ts && rhs->kind == EXPR_LITERAL_KIND) rhs->expected = src_ts;
    parser_visit_expr(ps, rhs);
    if (!rhs->ts) return;

    if (!src_ts) {
        /* lhs is bare `nil`: the result is simply rhs type */
        exp->ts = rhs->ts;
        exp->sym = get_symbol_by_id(exp->ts->sym_id);
        log_info("'\?\?' operator resolved, lhs is nil type, take rhs type.");
        log_type_spec(exp->ts);
        return;
    }

    if (type_is_optional(rhs->ts)) {
        TypeSpec *rhs_src = rhs->ts->opt.src;
        if (!rhs_src) {
            /* rhs is bare `nil`: `T? ?? nil` stays `T?` */
            exp->ts = lhs->ts;
            exp->sym = lhs->sym;
            log_info("'\?\?' operator resolved, rhs is nil type, take lhs type.");
            log_type_spec(exp->ts);
            return;
        }
        /* chained form: `T? ?? U?` stays optional, so `a ?? b ?? c` needs no
         * parentheses; unwrapped types must be compatible. */
        if (!type_spec_compatible(src_ts, rhs_src)) {
            kl_error(co->loc, "two optional types are not compatible for '\?\?' operator.");
            log_info("lhs type:");
            log_type_spec(lhs->ts);
            log_info("rhs type:");
            log_type_spec(rhs->ts);
            return;
        }
        exp->ts = rhs->ts;
        exp->sym = lhs->sym;
        log_info("'\?\?' operator resolved, rhs is optional, result stays optional.");
        log_type_spec(exp->ts);
        return;
    }

    /* `T? ?? T` gives `T`: rhs provides the non-optional default value */
    if (!type_spec_compatible(src_ts, rhs->ts)) {
        kl_error(co->loc,
                 "default value type is not compatible with unwrapped type for '\?\?' "
                 "operator.");
        log_info("lhs unwrapped type:");
        log_type_spec(src_ts);
        log_info("rhs type:");
        log_type_spec(rhs->ts);
        return;
    }

    exp->ts = src_ts;
    exp->sym = lhs->sym;
    log_info("'\?\?' operator resolved, unwrap optional type with default value.");
    log_type_spec(exp->ts);
}

static void parse_lambda(ParserState *ps, Expr *exp)
{
    LambdaExpr *lambda = (LambdaExpr *)exp;
    if (exp->ts) return;
    if (!lambda->body) return;
    for (int i = 0; i < vector_size(lambda->params); i++) {
        ParamDecl *param = vector_get(lambda->params, i);
        for (int j = 0; j < i; j++) {
            ParamDecl *previous = vector_get(lambda->params, j);
            if (str_equal(param->id.name, previous->id.name)) {
                kl_error(param->id.loc, "duplicate lambda parameter '%s'", param->id.name);
                return;
            }
        }
    }

    TypeSpec *expected = exp->expected;
    if (expected && type_is_optional(expected)) expected = expected->opt.src;
    if (expected && !type_is_proto(expected)) expected = NULL;
    if (expected) {
        /* an unspecialized `func(T) R` inside a generic body names a type
           variable, which is not a type a lambda parameter can have */
        TypeSpec *proto_part;
        vector_foreach(proto_part, expected->proto_type.args) {
            if (type_is_generic_var(proto_part) || type_is_generic_ref(proto_part)) {
                kl_error(exp->loc,
                         "lambda parameter type cannot be inferred from the generic function "
                         "type '%s', annotate the lambda parameter.",
                         expected->signature);
                return;
            }
        }
        if (type_is_generic_var(expected->proto_type.ret) ||
            type_is_generic_ref(expected->proto_type.ret)) {
            kl_error(exp->loc,
                     "lambda return type cannot be inferred from the generic function type '%s'.",
                     expected->signature);
            return;
        }
    }
    if (expected && vector_size(expected->proto_type.args) != vector_size(lambda->params)) {
        kl_error(exp->loc, "lambda parameter count does not match expected function type");
        return;
    }

    if (!lambda->scope) lambda->scope = stbl_new();
    ParserScope *scope = enter_scope(ps, SCOPE_ANONY, 0, "lambda");
    scope->stbl = lambda->scope;
    Vector *args = vector_create_ptr();
    int errors = ps->errors;
    for (int i = 0; i < vector_size(lambda->params); i++) {
        ParamDecl *param = vector_get(lambda->params, i);
        int has_type = param->type != NULL;
        TypeSpec *param_ts = has_type ? resolve_type(ps, param->type) : NULL;
        if (has_type) param->type = param_ts;
        TypeSpec *expected_ts = expected ? vector_get(expected->proto_type.args, i) : NULL;
        if (!param_ts && !has_type) param_ts = expected_ts;
        if (!param_ts) {
            if (!has_type) {
                kl_error(param->id.loc, "lambda parameter '%s' needs a type", param->id.name);
            }
            break;
        }
        if (!check_type(ps, param_ts)) break;
        if (expected_ts && !type_spec_equal_exact(param_ts, expected_ts)) {
            kl_error_incompatible_type(param->id.loc, expected_ts, param_ts);
            break;
        }
        Symbol *sym = stbl_add_var(scope->stbl, param->id.name, param_ts, 0);
        if (!sym) {
            kl_error(param->id.loc, "duplicate lambda parameter '%s'", param->id.name);
            break;
        }
        sym->status = SYM_RESOLVED;
        ((VarSymbol *)sym)->scope = VAR_SCOPE_PARAM;
        vector_push_back(args, &param_ts);
    }
    if (ps->errors == errors) {
        lambda->body->ctx = EXPR_CTX_LOAD;
        lambda->body->expected = expected ? expected->proto_type.ret : NULL;
        parser_visit_expr(ps, lambda->body);
    }
    exit_scope(ps);

    if (ps->errors != errors || !lambda->body->ts) {
        vector_destroy(args);
        return;
    }
    exp->ts = func_type_spec_intern(args, lambda->body->ts);
}

void parser_visit_expr(ParserState *ps, Expr *exp)
{
    if (!exp) return;

    /* if errors is greater than MAX_ERRORS, stop parsing */
    if (ps->errors >= MAX_ERRORS) return;

    /* clang-format off */
    static void (*handlers[EXPR_MAX_KIND])(ParserState *, Expr *) = {
        [EXPR_ID_KIND]      = parse_ident,
        [EXPR_UNDER_KIND]   = parse_under,
        [EXPR_LITERAL_KIND] = parse_literal,
        [EXPR_SELF_KIND]    = parse_self,
        [EXPR_LIST_KIND]    = parse_list,
        [EXPR_MAP_KIND]     = parse_map,
        [EXPR_MAP_ENTRY_KIND] = parse_map_entry,
        [EXPR_TUPLE_KIND]   = parse_tuple,
        [EXPR_ANONY_KIND]   = parse_lambda,
        [EXPR_TYPE_KIND]    = parse_type,
        [EXPR_OPTIONAL_TYPE_KIND] = parse_optional_type,
        [EXPR_CALL_KIND]    = parse_call,
        [EXPR_DOT_KIND]     = parse_dot,
        [EXPR_INDEX_KIND]   = parse_index,
        [EXPR_SLICE_KIND]   = parse_slice,
        [EXPR_UNARY_KIND]   = parse_unary,
        [EXPR_BINARY_KIND]  = parse_binary,
        [EXPR_KW_KIND]      = parse_keyword,
        [EXPR_IS_KIND]      = parse_is,
        [EXPR_AS_KIND]      = parse_as,
        [EXPR_IN_KIND]      = parse_in,
        [EXPR_BANG_KIND]    = parse_bang,
        [EXPR_COALESCE_KIND] = parse_coalesce,
        [EXPR_IF_KIND] = parse_if_expr,
        [EXPR_CONST_PLACEHOLDER] = parse_const_placeholder,
    };
    /* clang-format on */

    handlers[exp->kind](ps, exp);

    if (!exp->ts && exp->ctx != EXPR_CTX_STORE) {
        if (ps->errors == 0) {
            kl_error(exp->loc, "cannot resolve expr's type.");
        }
    }
}

#ifdef __cplusplus
}
#endif

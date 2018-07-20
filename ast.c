
#include "ast.h"
#include "log.h"

/*-------------------------------------------------------------------------*/

expr_t *expr_new(int kind)
{
  expr_t *exp = calloc(1, sizeof(expr_t));
  exp->kind = kind;
  return exp;
}

expr_t *expr_from_id(char *id)
{
  expr_t *expr = expr_new(ID_KIND);
  expr->id = id;
  return expr;
}

expr_t *expr_from_int(int64 ival)
{
  expr_t *expr = expr_new(INT_KIND);
  expr->desc = &Int_Type;
  expr->ival = ival;
  return expr;
}

expr_t *expr_from_float(float64 fval)
{
  expr_t *expr = expr_new(FLOAT_KIND);
  expr->desc = &Float_Type;
  expr->fval = fval;
  return expr;
}

expr_t *expr_from_string(char *str)
{
  expr_t *expr = expr_new(STRING_KIND);
  expr->desc = &String_Type;
  expr->str = str;
  return expr;
}

expr_t *expr_from_bool(int bval)
{
  expr_t *expr = expr_new(BOOL_KIND);
  expr->desc = &Bool_Type;
  expr->bval = bval;
  return expr;
}

expr_t *expr_from_self(void)
{
  expr_t *expr = expr_new(SELF_KIND);
  expr->desc = NULL;
  return expr;
}

expr_t *expr_from_super(void)
{
  expr_t *expr = expr_new(SUPER_KIND);
  expr->desc = NULL;
  return expr;
}

expr_t *expr_from_typeof(void)
{
  expr_t *expr = expr_new(TYPEOF_KIND);
  expr->desc = NULL;
  return expr;
}

expr_t *expr_from_expr(expr_t *exp)
{
  expr_t *expr = expr_new(EXP_KIND);
  expr->desc = NULL;
  expr->exp  = exp;
  return expr;
}

expr_t *expr_from_nil(void)
{
  expr_t *expr = expr_new(NIL_KIND);
  expr->desc = NULL;
  return expr;
}

expr_t *expr_from_array(TypeDesc *desc, Vector *dseq, Vector *tseq)
{
  expr_t *expr = expr_new(ARRAY_KIND);
  expr->desc = desc;
  expr->array.dseq = dseq;
  expr->array.tseq = tseq;
  return expr;
}

expr_t *expr_from_array_with_tseq(Vector *tseq)
{
  UNUSED_PARAMETER(tseq);
  expr_t *e = expr_new(SEQ_KIND);
  //e->vec = tseq;
  return e;
}

expr_t *expr_from_anonymous_func(Vector *pvec, Vector *rvec, Vector *body)
{
  expr_t *expr = expr_new(ANONYOUS_FUNC_KIND);
  expr->anonyous_func.pvec = pvec;
  expr->anonyous_func.rvec = rvec;
  expr->anonyous_func.body = body;
  return expr;
}

expr_t *expr_from_trailer(enum expr_kind kind, void *trailer,
                               expr_t *left)
{
  expr_t *expr = expr_new(kind);
  switch (kind) {
    case ATTRIBUTE_KIND: {
      expr->ctx = EXPR_LOAD;
      expr->attribute.left = left;
      expr->attribute.id = trailer;
      break;
    }
    case SUBSCRIPT_KIND: {
      expr->subscript.left = left;
      expr->subscript.index = trailer;
      break;
    }
    case CALL_KIND: {
      expr->call.left = left;
      expr->call.args = trailer;
      break;
    }
    default: {
      kassert(0, "unkown expression kind %d\n", kind);
    }
  }
  return expr;
}

expr_t *expr_from_binary(enum binary_op_kind kind,
                              expr_t *left, expr_t *right)
{
  expr_t *exp = expr_new(BINARY_KIND);
  exp->desc = left->desc;
  exp->binary.left = left;
  exp->binary.op = kind;
  exp->binary.right = right;
  return exp;
}

expr_t *expr_from_unary(enum unary_op_kind kind, expr_t *expr)
{
  expr_t *exp = expr_new(UNARY_KIND);
  exp->desc = expr->desc;
  exp->unary.op = kind;
  exp->unary.operand = expr;
  return exp;
}

expr_t *expr_rright_exp(expr_t *exp)
{
  expr_t *e = exp;
  while (e && e->right) {
    e = e->right;
  }
  return e;
}

/*--------------------------------------------------------------------------*/

static inline stmt_t *stmt_new(int kind)
{
  stmt_t *stmt = calloc(1, sizeof(stmt_t));
  stmt->kind = kind;
  return stmt;
}

static inline void stmt_free(stmt_t *stmt)
{
  free(stmt);
}

stmt_t *stmt_from_var(char *id, TypeDesc *desc, expr_t *exp, int bconst)
{
  stmt_t *stmt = stmt_new(VAR_KIND);
  stmt->var.id     = id;
  stmt->var.desc   = desc;
  stmt->var.exp    = exp;
  stmt->var.bconst = bconst;
  if (!desc && exp && exp->desc) stmt->var.desc = Type_Dup(exp->desc);
  return stmt;
}

stmt_t *stmt_from_varlist(Vector *ids, TypeDesc *desc, expr_t *exp, int bconst)
{
  stmt_t *stmt = stmt_new(VARLIST_KIND);
  stmt->vars.ids    = ids;
  stmt->vars.desc   = desc;
  stmt->vars.exp    = exp;
  stmt->vars.bconst = bconst;
  return stmt;
}

stmt_t *stmt_from_func(char *id, Vector *args, Vector *rets, Vector *body)
{
  stmt_t *stmt = stmt_new(FUNC_KIND);
  stmt->func.id   = id;
  stmt->func.args = args;
  stmt->func.rets = rets;
  stmt->func.body = body;
  return stmt;
}

stmt_t *stmt_from_proto(char *id, Vector *args, Vector *rets)
{
  stmt_t *stmt = stmt_new(PROTO_KIND);
  stmt->proto.id   = id;
  stmt->proto.args = args;
  stmt->proto.rets = rets;
  return stmt;
}

stmt_t *stmt_from_assign(expr_t *l, AssignOpKind op, expr_t *r)
{
  stmt_t *stmt = stmt_new(ASSIGN_KIND);
  stmt->assign.left  = l;
  stmt->assign.op    = op;
  stmt->assign.right = r;
  return stmt;
}

stmt_t *stmt_from_assigns(Vector *left, expr_t *right)
{
  stmt_t *stmt = stmt_new(ASSIGNS_KIND);
  stmt->assigns.left  = left;
  stmt->assigns.right = right;
  return stmt;
}

stmt_t *stmt_from_return(Vector *list)
{
  stmt_t *stmt = stmt_new(RETURN_KIND);
  stmt->returns = list;
  return stmt;
}

stmt_t *stmt_from_expr(expr_t *exp)
{
  stmt_t *stmt = stmt_new(EXPR_KIND);
  stmt->exp = exp;
  return stmt;
}

stmt_t *stmt_from_block(Vector *block)
{
  stmt_t *stmt = stmt_new(BLOCK_KIND);
  stmt->block = block;
  return stmt;
}

stmt_t *stmt_from_trait(char *id, Vector *traits, Vector *body)
{
  stmt_t *stmt = stmt_new(TRAIT_KIND);
  stmt->class_info.id = id;
  stmt->class_info.traits = traits;
  stmt->class_info.body = body;
  return stmt;
}

stmt_t *stmt_from_jump(int kind, int level)
{
  if (level <= 0) {
    error("break or continue level must be > 0");
    return NULL;
  }
  stmt_t *stmt = stmt_new(kind);
  stmt->jump_stmt.level = level;
  return stmt;
}

stmt_t *stmt_from_if(expr_t *test, Vector *body,
  stmt_t *orelse)
{
  stmt_t *stmt = stmt_new(IF_KIND);
  stmt->if_stmt.test = test;
  stmt->if_stmt.body = body;
  stmt->if_stmt.orelse = orelse;
  return stmt;
}

struct test_block *new_test_block(expr_t *test, Vector *body)
{
  struct test_block *tb = malloc(sizeof(struct test_block));
  tb->test = test;
  tb->body = body;
  return tb;
}

stmt_t *stmt_from_while(expr_t *test, Vector *body, int btest)
{
  stmt_t *stmt = stmt_new(WHILE_KIND);
  stmt->while_stmt.btest = btest;
  stmt->while_stmt.test  = test;
  stmt->while_stmt.body  = body;
  return stmt;
}

stmt_t *stmt_from_switch(expr_t *expr, Vector *case_seq)
{
  stmt_t *stmt = stmt_new(SWITCH_KIND);
  stmt->switch_stmt.expr = expr;
  stmt->switch_stmt.case_seq = case_seq;
  return stmt;
}

stmt_t *stmt_from_for(stmt_t *init, stmt_t *test,
                           stmt_t *incr, Vector *body)
{
  stmt_t *stmt = stmt_new(FOR_TRIPLE_KIND);
  stmt->for_triple_stmt.init = init;
  stmt->for_triple_stmt.test = test;
  stmt->for_triple_stmt.incr = incr;
  stmt->for_triple_stmt.body = body;
  return stmt;
}

/*
stmt_t *stmt_from_foreach(struct var *var, expr_t *expr,
                             Vector *body, int bdecl)
{
  stmt_t *stmt = stmt_new(FOR_EACH_KIND);
  stmt->for_each_stmt.bdecl = bdecl;
  stmt->for_each_stmt.var   = var;
  stmt->for_each_stmt.expr  = expr;
  stmt->for_each_stmt.body  = body;
  return stmt;
}
*/

stmt_t *stmt_from_go(expr_t *expr)
{
  if (expr->kind != CALL_KIND) {
    kassert(0, "syntax error:not a func call\n");
    exit(0);
  }

  stmt_t *stmt = stmt_new(GO_KIND);
  stmt->go_stmt = expr;
  return stmt;
}

stmt_t *stmt_from_typealias(char *id, TypeDesc *desc)
{
  stmt_t *stmt = stmt_new(TYPEALIAS_KIND);
  stmt->typealias.id = id;
  stmt->typealias.desc = desc;
  return stmt;
}

stmt_t *stmt_from_list(void)
{
  stmt_t *stmt = stmt_new(LIST_KIND);
  Vector_Init(&stmt->list);
  return stmt;
}

void stmt_free_list(stmt_t *stmt)
{
  stmt_t *s;
  Vector_ForEach(s, &stmt->list) {
    stmt_free(s);
  }
  stmt_free(stmt);
}

/*---------------------------------------------------------------------------*/

int binop_arithmetic(int op)
{
  if (op >= BINARY_ADD && op <= BINARY_RSHIFT)
    return 1;
  else
    return 0;
}

int binop_relation(int op)
{
  if (op >= BINARY_GT && op <= BINARY_NEQ)
    return 1;
  else
    return 0;
}

int binop_logic(int op)
{
  if (op == BINARY_LAND || op == BINARY_LOR)
    return 1;
  else
    return 0;
}

int binop_bit(int op)
{
  if (op == BINARY_BIT_AND || op == BINARY_BIT_XOR || op == BINARY_BIT_OR)
    return 1;
  else
    return 0;
}

/*--------------------------------------------------------------------------*/

static void item_stmt_free(void *item, void *arg)
{
  UNUSED_PARAMETER(arg);
  stmt_free(item);
}

void vec_stmt_free(Vector *stmts)
{
  if (!stmts) return;
  Vector_Free(stmts, item_stmt_free, NULL);
}

void vec_stmt_fini(Vector *stmts)
{
  if (!stmts) return;
  Vector_Fini(stmts, item_stmt_free, NULL);
}

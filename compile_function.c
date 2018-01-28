
#include "compile.h"

int expr_id_handler(ParserContext *ctx, struct expr *exp)
{
  Symbol *sym = parser_find_symbol(ctx, exp->name.id);
  return 0;
}

int expr_attr_handler(ParserContext *ctx, struct expr *exp)
{
  return 0;
}

int expr_call_handler(ParserContext *ctx, struct expr *exp)
{
  return 0;
}

static expr_handler_t expr_handlers[] = {
  NULL,
  expr_id_handler, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL,
  NULL, NULL, expr_attr_handler, NULL,
  expr_call_handler, NULL, NULL, NULL,
};

int expr_handler(ParserContext *ctx, struct expr *exp)
{
  ASSERT(exp->kind > 0 && exp->kind < EXPR_KIND_MAX);
  printf("expr kind:%d\n", exp->kind);
  expr_handler_t handler = expr_handlers[exp->kind];
  ASSERT_PTR(handler);
  return handler(ctx, exp);
}

int expr_stmt_handler(ParserContext *ctx, struct stmt *stmt)
{
  struct expr *exp = stmt->expr;
  return expr_handler(ctx, exp);
}

static stmt_handler_t localstmt_handlers[] = {
  NULL, /* INVALID */
  NULL,
  expr_stmt_handler,
};

int localstmt_handler(ParserContext *ctx, struct stmt *stmt)
{
  ASSERT(stmt->kind > 0 && stmt->kind < STMT_KIND_MAX);
  stmt_handler_t handler = localstmt_handlers[stmt->kind];
  ASSERT_PTR(handler);
  return handler(ctx, stmt);
}

int parse_body(ParserContext *ctx, struct stmt *stmt)
{
  Vector *vec = stmt->funcdecl.body;
  if (vec == NULL) return 0;
  int sz = Vector_Size(vec);
  struct stmt *temp;
  for (int i = 0; i < sz; i++) {
    temp = Vector_Get(vec, i);
    localstmt_handler(ctx, temp);
  }
  return 0;
}

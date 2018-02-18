
#include "ast.h"
#include "codeformat.h"
#include "log.h"

/*-------------------------------------------------------------------------*/

struct expr *expr_new(int kind)
{
  struct expr *exp = calloc(1, sizeof(struct expr));
  exp->kind = kind;
  return exp;
}

struct expr *expr_from_id(char *id)
{
  struct expr *expr = expr_new(NAME_KIND);
  expr->id = id;
  return expr;
}

struct expr *expr_from_int(int64 ival)
{
  struct expr *expr = expr_new(INT_KIND);
  expr->type = TypeDesc_From_Primitive(PRIMITIVE_INT);
  expr->ival = ival;
  return expr;
}

struct expr *expr_from_float(float64 fval)
{
  struct expr *expr = expr_new(FLOAT_KIND);
  expr->type = TypeDesc_From_Primitive(PRIMITIVE_FLOAT);
  expr->fval = fval;
  return expr;
}

struct expr *expr_from_string(char *str)
{
  struct expr *expr = expr_new(STRING_KIND);
  expr->type = TypeDesc_From_Primitive(PRIMITIVE_STRING);
  expr->str = str;
  return expr;
}

struct expr *expr_from_bool(int bval)
{
  struct expr *expr = expr_new(BOOL_KIND);
  expr->type = TypeDesc_From_Primitive(PRIMITIVE_BOOL);
  expr->bval = bval;
  return expr;
}

struct expr *expr_from_self(void)
{
  struct expr *expr = expr_new(SELF_KIND);
  expr->type = NULL;
  return expr;
}

struct expr *expr_from_expr(struct expr *exp)
{
  struct expr *expr = expr_new(EXP_KIND);
  expr->type = NULL;
  expr->exp  = exp;
  return expr;
}

struct expr *expr_from_nil(void)
{
  struct expr *expr = expr_new(NIL_KIND);
  expr->type = NULL;
  return expr;
}

struct expr *expr_from_array(TypeDesc *type, Vector *dseq, Vector *tseq)
{
  struct expr *expr = expr_new(ARRAY_KIND);
  expr->type = type;
  expr->array.dseq = dseq;
  expr->array.tseq = tseq;
  return expr;
}

struct expr *expr_from_array_with_tseq(Vector *tseq)
{
  struct expr *e = expr_new(SEQ_KIND);
  e->vec = tseq;
  return e;
}

struct expr *expr_from_anonymous_func(Vector *pvec, Vector *rvec, Vector *body)
{
  struct expr *expr = expr_new(ANONYOUS_FUNC_KIND);
  expr->anonyous_func.pvec = pvec;
  expr->anonyous_func.rvec = rvec;
  expr->anonyous_func.body = body;
  return expr;
}

struct expr *expr_from_trailer(enum expr_kind kind, void *trailer,
                               struct expr *left)
{
  struct expr *expr = expr_new(kind);
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
      fprintf(stderr, "[ERROR]unkown expression kind %d\n", kind);
      ASSERT(0);
    }
  }
  return expr;
}

struct expr *expr_from_binary(enum binary_op_kind kind,
                              struct expr *left, struct expr *right)
{
  struct expr *exp = expr_new(BINARY_KIND);
  exp->type = left->type;
  exp->binary.left = left;
  exp->binary.op = kind;
  exp->binary.right = right;
  return exp;
}

struct expr *expr_from_unary(enum unary_op_kind kind, struct expr *expr)
{
  struct expr *exp = expr_new(UNARY_KIND);
  exp->type = expr->type;
  exp->unary.op = kind;
  exp->unary.operand = expr;
  return exp;
}

/*--------------------------------------------------------------------------*/

struct stmt *stmt_new(int kind)
{
  struct stmt *stmt = calloc(1, sizeof(struct stmt));
  stmt->kind = kind;
  return stmt;
}

void stmt_free(struct stmt *stmt)
{
  free(stmt);
}

struct stmt *stmt_from_vector(int kind, Vector *vec)
{
  struct stmt *stmt = stmt_new(kind);
  stmt->vec = vec;
  return stmt;
}

struct stmt *stmt_from_expr(struct expr *exp)
{
  struct stmt *stmt = stmt_new(EXPR_KIND);
  stmt->exp = exp;
  return stmt;
}

struct stmt *stmt_from_import(char *id, char *path)
{
  struct stmt *stmt = stmt_new(IMPORT_KIND);
  stmt->import.id = id;
  stmt->import.path = path;
  return stmt;
}

struct stmt *stmt_from_vardecl(Vector *varvec, Vector *expvec,
                               int bconst, TypeDesc *type)
{
  Vector *vec = Vector_New();
  int vsz = Vector_Size(varvec);
  int esz = (expvec != NULL) ? Vector_Size(expvec) : 0;
  int error = 0;
  if (esz != vsz) {
    if (esz != 0)
      error("cannot assign %d values to %d variables", esz, vsz);
    error = 1;
  }

  struct stmt *stmt;
  struct var *var;
  for (int i = 0; i < vsz; i++) {
    var = Vector_Get(varvec, i);
    var->bconst = bconst;
    var->type = type;
    stmt = stmt_new(VARDECL_KIND);
    stmt->vardecl.var = var;
    Vector_Append(vec, stmt);
    if (!error) {
      stmt->vardecl.exp = Vector_Get(expvec, i);
      if (var->type == NULL) {
        debug("variable '%s' type is not set", var->id);
        var->type = stmt->vardecl.exp->type;
        if (var->type == NULL) {
          debug("right expr's type is also null");
        }
      }
    }
  }

  Vector_Free(varvec, NULL, NULL);
  Vector_Free(expvec, NULL, NULL);
  return stmt_from_vector(VARDECL_LIST_KIND, vec);
}

struct stmt *stmt_from_funcdecl(char *id, Vector *pvec, Vector *rvec,
                                Vector *body)
{
  struct stmt *stmt = stmt_new(FUNCDECL_KIND);
  stmt->funcdecl.id = id;
  stmt->funcdecl.pvec = pvec;
  stmt->funcdecl.rvec = rvec;
  stmt->funcdecl.body = body;
  return stmt;
}

struct stmt *stmt_from_assign(Vector *left, Vector *right)
{
  Vector *vec = Vector_New();
  int vsz = Vector_Size(left);
  int esz = (right != NULL) ? Vector_Size(right) : 0;
  int error = 0;
  if (esz != vsz) {
    if (esz != 0)
      error("cannot assign %d values to %d variables", esz, vsz);
    error = 1;
  }

  struct stmt *stmt;
  for (int i = 0; i < vsz; i++) {
    stmt = stmt_new(ASSIGN_KIND);
    stmt->assign.left = Vector_Get(left, i);
    Vector_Append(vec, stmt);
    if (!error) {
      stmt->assign.right = Vector_Get(right, i);
      // check lvalue & rvalue's type
    }
  }

  Vector_Free(left, NULL, NULL);
  Vector_Free(right, NULL, NULL);
  return stmt_from_vector(ASSIGN_LIST_KIND, vec);
}

struct stmt *stmt_from_compound_assign(struct expr *left,
                                       enum assign_operator op,
                                       struct expr *right)
{
  struct stmt *stmt = stmt_new(COMPOUND_ASSIGN_KIND);
  stmt->compound_assign.left  = left;
  stmt->compound_assign.op    = op;
  stmt->compound_assign.right = right;
  return stmt;
}

struct stmt *stmt_from_block(Vector *block)
{
  return stmt_from_vector(BLOCK_KIND, block);
}

struct stmt *stmt_from_return(Vector *vec)
{
  return stmt_from_vector(RETURN_KIND, vec);
}

struct stmt *stmt_from_structure(char *id, Vector *vec)
{
  struct stmt *stmt = stmt_new(CLASS_KIND);
  stmt->structure.id  = id;
  stmt->structure.vec = vec;
  return stmt;
}

struct stmt *stmt_from_interface(char *id, Vector *vec)
{
  struct stmt *stmt = stmt_new(INTF_KIND);
  stmt->structure.id  = id;
  stmt->structure.vec = vec;
  return stmt;
}

struct stmt *stmt_from_jump(int kind)
{
  struct stmt *stmt = stmt_new(kind);
  return stmt;
}

struct stmt *stmt_from_if(struct test_block *if_part,
                          Vector *elseif_seq,
                          struct test_block *else_part)
{
  struct stmt *stmt = stmt_new(IF_KIND);
  stmt->if_stmt.if_part    = if_part;
  stmt->if_stmt.elseif_seq = elseif_seq;
  stmt->if_stmt.else_part  = else_part;
  return stmt;
}

struct test_block *new_test_block(struct expr *test, Vector *body)
{
  struct test_block *tb = malloc(sizeof(struct test_block));
  tb->test = test;
  tb->body = body;
  return tb;
}

struct stmt *stmt_from_while(struct expr *test, Vector *body, int b)
{
  struct stmt *stmt = stmt_new(WHILE_KIND);
  stmt->while_stmt.btest = b;
  stmt->while_stmt.test  = test;
  stmt->while_stmt.body  = body;
  return stmt;
}

struct stmt *stmt_from_switch(struct expr *expr, Vector *case_seq)
{
  struct stmt *stmt = stmt_new(SWITCH_KIND);
  stmt->switch_stmt.expr = expr;
  stmt->switch_stmt.case_seq = case_seq;
  return stmt;
}

struct stmt *stmt_from_for(struct stmt *init, struct stmt *test,
                           struct stmt *incr, Vector *body)
{
  struct stmt *stmt = stmt_new(FOR_TRIPLE_KIND);
  stmt->for_triple_stmt.init = init;
  stmt->for_triple_stmt.test = test;
  stmt->for_triple_stmt.incr = incr;
  stmt->for_triple_stmt.body = body;
  return stmt;
}

struct stmt *stmt_from_foreach(struct var *var, struct expr *expr,
                               Vector *body, int bdecl)
{
  struct stmt *stmt = stmt_new(FOR_EACH_KIND);
  stmt->for_each_stmt.bdecl = bdecl;
  stmt->for_each_stmt.var   = var;
  stmt->for_each_stmt.expr  = expr;
  stmt->for_each_stmt.body  = body;
  return stmt;
}

struct stmt *stmt_from_go(struct expr *expr)
{
  if (expr->kind != CALL_KIND) {
    fprintf(stderr, "syntax error:not a func call\n");
    exit(0);
  }

  struct stmt *stmt = stmt_new(GO_KIND);
  stmt->go_stmt = expr;
  return stmt;
}

struct var *new_var(char *id, TypeDesc *type)
{
  struct var *v = malloc(sizeof(struct var));
  v->id = id;
  v->bconst = 0;
  v->type = type;
  return v;
}

/*--------------------------------------------------------------------------*/

void type_traverse(TypeDesc *type)
{
  if (type != NULL) {
    printf("type kind & dims: %d:%d\n", type->kind, type->dims);
  } else {
    printf("no type declared\n");
  }
}

void array_traverse(struct expr *expr)
{
  type_traverse(expr->type);
  if (expr->array.tseq != NULL) {
    printf("array init's list, length:%d\n", Vector_Size(expr->array.tseq));
    for (int i = 0; i < Vector_Size(expr->array.tseq); i++) {
      expr_traverse(Vector_Get(expr->array.tseq, i));
    }
  } else {
    printf("array dim's list, length:%d\n", Vector_Size(expr->array.dseq));
    for (int i = 0; i < Vector_Size(expr->array.dseq); i++) {
      expr_traverse(Vector_Get(expr->array.dseq, i));
    }
  }
}

void expr_traverse(struct expr *expr)
{
  if (expr == NULL) return;

  switch (expr->kind) {
    case NAME_KIND: {
      printf("[id]%s\n", expr->id);
      break;
    }
    case INT_KIND: {
      printf("[int]%lld\n", expr->ival);
      break;
    }
    case FLOAT_KIND: {
      printf("[float]%f\n", expr->fval);
      break;
    }
    case STRING_KIND: {
      printf("[string]%s\n", expr->str);
      break;
    }
    case BOOL_KIND: {
      printf("%s\n", expr->bval ? "true" : "false");
      break;
    }
    case SELF_KIND: {
      printf("self\n");
      break;
    }
    case NIL_KIND: {
      printf("nil\n");
      break;
    }
    case EXP_KIND: {
      printf("[sub-expr]\n");
      expr_traverse(expr->exp);
      break;
    }
    case ARRAY_KIND: {
      printf("[new array]\n");
      array_traverse(expr);
      break;
    }
    case ANONYOUS_FUNC_KIND: {
      printf("[anonymous function]\n");
      break;
    }
    case ATTRIBUTE_KIND: {
      expr_traverse(expr->attribute.left);
      printf("[attribute].%s\n", expr->attribute.id);
      break;
    }
    case SUBSCRIPT_KIND: {
      expr_traverse(expr->subscript.left);
      printf("[subscript]\n");
      expr_traverse(expr->subscript.index);
      break;
    }
    case CALL_KIND: {
      expr_traverse(expr->call.left);
      printf("[func call]paras:\n");
      struct expr *temp;
      Vector *vec = expr->call.args;
      if (vec != NULL) {
        for (int i = 0; i < Vector_Size(vec); i++) {
          temp = Vector_Get(vec, i);
          expr_traverse(temp);
        }
      } else {
        printf("no args\n");
      }
      printf("[end func call]\n");
      break;
    }
    case UNARY_KIND: {
      printf("[unary expr]op:%d\n", expr->unary.op);
      expr_traverse(expr->unary.operand);
      break;
    }
    case BINARY_KIND: {
      printf("[binary expr]op:%d\n", expr->binary.op);
      expr_traverse(expr->binary.left);
      expr_traverse(expr->binary.right);
      break;
    }
    case SEQ_KIND: {
      printf("[vec]\n");
      for (int i = 0; i < Vector_Size(expr->vec); i++) {
        expr_traverse(Vector_Get(expr->vec, i));
      }
      printf("[end vec]\n");
      break;
    }
    default: {
      ASSERT(0);
      break;
    }
  }
}

/*--------------------------------------------------------------------------*/

void vardecl_traverse(struct stmt *stmt)
{
  printf("variable:\n");
  struct var *var = stmt->vardecl.var;
  printf("%s %s ", var->id, var->bconst ? "const":"");
  putchar('\n');

  printf("initializer:\n");
  struct expr *exp = stmt->vardecl.exp;
  if (exp != NULL) {
    expr_traverse(exp);
  } else {
    printf("var is not initialized\n");
  }
}

void stmt_traverse(struct stmt *stmt);

void func_traverse(struct stmt *stmt)
{
  Vector *vec = stmt->funcdecl.pvec;
  struct var *var;
  printf("[function]\n");
  printf("params:");
  char *typestr;
  if (vec != NULL) {
    for (int i = 0; i < Vector_Size(vec); i++) {
      var = Vector_Get(vec, i);
      typestr = TypeDesc_ToString(var->type);
      printf(" %s %s", var->id, typestr);
      free(typestr);
      if (i + 1 != Vector_Size(vec))
        printf(",");
    }
    printf("\n");
  }

  vec = stmt->funcdecl.rvec;
  printf("returns:");
  if (vec != NULL) {
    TypeDesc *t;
    Vector_ForEach(t, vec) {
      t = Vector_Get(vec, i);
      printf(" %s", TypeDesc_ToString(t));
      if (i + 1 != Vector_Size(vec))
        printf(",");
    }
    printf("\n");
  }

  vec = stmt->funcdecl.body;
  if (vec != NULL) {
    for (int i = 0; i < Vector_Size(vec); i++)
      stmt_traverse(Vector_Get(vec, i));
  }
  printf("[end function]\n");
}

/*--------------------------------------------------------------------------*/

void stmt_traverse(struct stmt *stmt)
{
  switch (stmt->kind) {
    case IMPORT_KIND: {
      printf("[import]%s:%s\n", stmt->import.id, stmt->import.path);
      break;
    }
    case EXPR_KIND: {
      printf("[expr]\n");
      expr_traverse(stmt->exp);
      printf("[end expr]\n");
      break;
    }
    case VARDECL_KIND: {
      printf("[var decl]\n");
      vardecl_traverse(stmt);
      printf("[end var decl]\n");
      break;
    }
    case FUNCDECL_KIND: {
      printf("[func decl]:%s\n", stmt->funcdecl.id);
      func_traverse(stmt);
      break;
    }
    case ASSIGN_KIND: {
      printf("[assignment list]\n");
      // struct expr *expr;
      // clist_foreach(expr, stmt->v.assign.left_list) {
      //   expr_traverse(expr);
      // }
      //
      // clist_foreach(expr, stmt->v.assign.right_list) {
      //   expr_traverse(expr);
      // }
      break;
    }
    case COMPOUND_ASSIGN_KIND: {
      printf("[compound assignment]:%d\n", stmt->compound_assign.op);
      // expr_traverse(stmt->v.compound_assign.left);
      // expr_traverse(stmt->v.compound_assign.right);
      break;
    }
    case CLASS_KIND: {
      printf("[structure]:%s\n", stmt->structure.id);
      break;
    }
    case INTF_KIND: {
      printf("[interface]:%s\n", stmt->structure.id);
      break;
    }
    case IF_KIND: {
      printf("[if statement]\n");
      // ifstmt_traverse(stmt);
      break;
    }
    case WHILE_KIND: {
      printf("[while statement]\n");
      // whilestmt_traverse(stmt);
      break;
    }
    case SWITCH_KIND: {
      printf("[switch statement]\n");
      // switchstmt_traverse(stmt);
      break;
    }
    case FOR_TRIPLE_KIND: {
      printf("[for triple statement]\n");
      break;
    }
    case FOR_EACH_KIND: {
      printf("[for each statement]\n");
      break;
    }
    case BREAK_KIND: {
      printf("[break statement]\n");
      break;
    }
    case CONTINUE_KIND: {
      printf("[continue statement]\n");
      break;
    }
    case RETURN_KIND: {
      printf("[return statement]\n");
      struct expr *exp;
      for (int i = 0; i < Vector_Size(stmt->vec); i++) {
        exp = Vector_Get(stmt->vec, i);
        expr_traverse(exp);
      }
      printf("[end return statement]\n");
      break;
    }
    case GO_KIND: {
      printf("[go statement]\n");
      expr_traverse(stmt->go_stmt);
      printf("[end go statement]\n");
      break;
    }
    case VARDECL_LIST_KIND: {
      printf("var decl list\n");
      break;
    }
    default:{
      printf("[ERROR] unknown stmt kind :%d\n", stmt->kind);
      ASSERT(0);
    }
  }
}

void ast_traverse(Vector *vec)
{
  struct stmt *s;
  Vector_ForEach(s, vec)
    stmt_traverse(s);
}

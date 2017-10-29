
#include "ast.h"
#include "namei.h"
#include "object.h"

#define SEQ_DEFAULT_SIZE 16

struct sequence *seq_new(void)
{
  struct sequence *seq = malloc(sizeof(*seq));
  seq->count = 0;
  vector_init(&seq->vec, SEQ_DEFAULT_SIZE);
  return seq;
}

void seq_free(struct sequence *seq)
{
  vector_fini(&seq->vec, NULL, NULL);
  free(seq);
}

int seq_insert(struct sequence *seq, int index, void *obj)
{
  seq_append(seq, obj);
  if (index < 0 || index > seq->count) {
    fprintf(stderr,
      "[ERROR] index %d out of bound(0-%d)\n", index, seq->count);
    return -1;
  }

  for (int i = seq->count - 1; i > index; i--) {
    vector_set(&seq->vec, i, vector_get(&seq->vec, i - 1));
  }

  vector_set(&seq->vec, index, obj);

  return 0;
}

int seq_append(struct sequence *seq, void *obj)
{
  int res = vector_set(&seq->vec, seq->count, obj);
  assert(!res);
  ++seq->count;
  return 0;
}

void *seq_get(struct sequence *seq, int index)
{
  assert(index < seq->count);
  void *obj = vector_get(&seq->vec, index);
  assert(obj != NULL);
  return obj;
}

struct type *type_from_primitive(int primitive)
{
  struct type *type = malloc(sizeof(*type));
  type->kind = PRIMITIVE_KIND;
  type->primitive = primitive;
  return type;
}

struct type *type_from_userdef(char *mod_name, char *type_name)
{
  struct type *type = malloc(sizeof(*type));
  type->kind = USERDEF_TYPE;
  type->userdef.mod_name  = mod_name;
  type->userdef.type_name = type_name;
  return type;
}

struct type *type_from_functype(struct sequence *tseq, struct sequence *rseq)
{
  struct type *type = malloc(sizeof(*type));
  type->kind = FUNCTION_TYPE;
  type->functype.tseq = tseq;
  type->functype.rseq = rseq;
  return type;
}

#if 0
struct array_tail *array_tail_from_expr(struct expr *expr)
{
  struct array_tail *tail = malloc(sizeof(*tail));
  init_list_head(&tail->link);
  tail->list   = NULL;
  tail->expr   = expr;
  return tail;
}

struct array_tail *array_tail_from_list(struct clist *list)
{
  struct array_tail *tail = malloc(sizeof(*tail));
  init_list_head(&tail->link);
  tail->list   = list;
  tail->expr   = NULL;
  return tail;
}

struct expr *trailer_from_attribute(char *id)
{
  struct expr *expr = malloc(sizeof(*expr));
  expr->kind = ATTRIBUTE_KIND;
  expr->v.attribute.expr = NULL;
  expr->v.attribute.id   = id;
  expr->v.attribute.type = 0;
  init_list_head(&expr->link);
  return expr;
}

struct expr *trailer_from_subscript(struct expr *idx)
{
  struct expr *expr = malloc(sizeof(*expr));
  expr->kind = SUBSCRIPT_KIND;
  expr->v.subscript.expr = NULL;
  expr->v.subscript.index = idx;
  init_list_head(&expr->link);
  return expr;
}

struct expr *trailer_from_call(struct clist *para)
{
  struct expr *expr = malloc(sizeof(*expr));
  expr->kind = CALL_KIND;
  expr->v.call.expr = NULL;
  expr->v.call.list = para;
  init_list_head(&expr->link);
  return expr;
}

static void trailer_set_left_expr(struct expr *expr, struct expr *left_expr)
{
  switch (expr->kind) {
    case ATTRIBUTE_KIND:
      expr->v.attribute.expr = left_expr;
      break;
    case SUBSCRIPT_KIND:
      expr->v.subscript.expr = left_expr;
      break;
    case CALL_KIND:
      expr->v.call.expr = left_expr;
      if (left_expr->kind == NAME_KIND) {
        left_expr->v.name.type = NT_FUNC;
      } else if (left_expr->kind == ATTRIBUTE_KIND) {
        left_expr->v.attribute.type = NT_FUNC;
      } else {
        printf("[DEBUG] call left_expr kind:%d\n", left_expr->kind);
      }
      break;
    case INTF_IMPL_KIND:
    default:
      assert(0);
      break;
  }
}

#endif

struct expr *expr_from_name(char *id)
{
  struct expr *expr = malloc(sizeof(*expr));
  expr->kind = NAME_KIND;
  expr->type = NULL;
  expr->name.id = id;
  return expr;
}

struct expr *expr_from_name_type(char *id, struct type *type)
{
  struct expr *expr = malloc(sizeof(*expr));
  expr->kind = NAME_KIND;
  expr->type = type;
  expr->name.id = id;
  return expr;
}

struct expr *expr_from_int(int64_t ival)
{
  struct expr *expr = malloc(sizeof(*expr));
  expr->kind = INT_KIND;
  expr->type = type_from_primitive(TYPE_INT);
  expr->ival = ival;
  return expr;
}

struct expr *expr_from_float(float64_t fval)
{
  struct expr *expr = malloc(sizeof(*expr));
  expr->kind = FLOAT_KIND;
  expr->type = type_from_primitive(TYPE_FLOAT);
  expr->fval = fval;
  return expr;
}

struct expr *expr_from_string(char *str)
{
  struct expr *expr = malloc(sizeof(*expr));
  expr->kind = STRING_KIND;
  expr->type = type_from_primitive(TYPE_STRING);
  expr->str  = str;
  return expr;
}

struct expr *expr_from_bool(int bval)
{
  struct expr *expr = malloc(sizeof(*expr));
  expr->kind = BOOL_KIND;
  expr->type = type_from_primitive(TYPE_BOOL);
  expr->bval = bval;
  return expr;
}

struct expr *expr_from_self(void)
{
  struct expr *expr = malloc(sizeof(*expr));
  expr->kind = SELF_KIND;
  expr->type = NULL;
  return expr;
}

struct expr *expr_from_expr(struct expr *exp)
{
  struct expr *expr = malloc(sizeof(*expr));
  expr->kind = EXP_KIND;
  expr->type = NULL;
  expr->exp  = exp;
  return expr;
}

struct expr *expr_from_null(void)
{
  struct expr *expr = malloc(sizeof(*expr));
  expr->kind = NULL_KIND;
  expr->type = NULL;
  return expr;
}

// struct expr *expr_from_array(struct type *type, int tail, struct clist *list)
// {
//   struct expr *expr = malloc(sizeof(*expr));
//   expr->kind = ARRAY_KIND;
//   expr->type = type;
//   expr->v.array.type = type;
//   if (tail) {
//     expr->v.array.tail_list = list;
//     expr->v.array.dim_list  = NULL;
//   } else {
//     expr->v.array.tail_list = NULL;
//     expr->v.array.dim_list  = list;
//   }
//   init_list_head(&expr->link);
//   return expr;
// }

struct expr *expr_from_anonymous_func(struct sequence *pseq,
                                      struct sequence *rseq,
                                      struct sequence *body)
{
  struct expr *expr = malloc(sizeof(*expr));
  expr->kind = ANONYOUS_FUNC_KIND;
  expr->anonyous_func.pseq = pseq;
  expr->anonyous_func.rseq = rseq;
  expr->anonyous_func.body = body;
  return expr;
}

struct expr *expr_from_trailer(enum expr_kind kind, void *trailer,
                               struct expr *left)
{
  struct expr *expr = malloc(sizeof(*expr));
  expr->kind = kind;
  switch (kind) {
    case ATTRIBUTE_KIND: {
      expr->attribute.left = left;
      expr->attribute.id   = trailer;
      break;
    }
    case SUBSCRIPT_KIND: {
      expr->subscript.left  = left;
      expr->subscript.index = trailer;
      break;
    }
    case CALL_KIND: {
      expr->call.left = left;
      expr->call.pseq = trailer;
      break;
    }
    default: {
      fprintf(stderr, "[ERROR]unkown expression kind %d\n", kind);
      assert(0);
    }
  }
  return expr;
}

#if 0

struct expr *expr_from_expr_trailers(struct clist *list, struct expr *expr)
{
  struct expr *trailer, *temp;
  clist_foreach_safe(trailer, list, temp) {
    clist_del(&trailer->link, list);
    trailer_set_left_expr(trailer, expr);
    expr = trailer;
  }

  return expr_from_expr(expr);
}

struct expr *expr_from_expr(struct expr *expr)
{
  struct expr *exp = malloc(sizeof(*exp));
  exp->kind   = ATOM_KIND;
  exp->v.expr = expr;
  init_list_head(&exp->link);
  return exp;
}

#endif

struct expr *expr_from_binary(enum operator_kind kind,
                              struct expr *left, struct expr *right)
{
  struct expr *exp = malloc(sizeof(*exp));
  exp->kind = BINARY_KIND;
  exp->bin_op.left  = left;
  exp->bin_op.op    = kind;
  exp->bin_op.right = right;
  return exp;
}

struct expr *expr_from_unary(enum unary_op_kind kind, struct expr *expr)
{
  struct expr *exp = malloc(sizeof(*exp));
  exp->kind = UNARY_KIND;
  exp->unary_op.op = kind;
  exp->unary_op.operand = expr;
  return exp;
}

struct stmt *stmt_from_expr(struct expr *expr)
{
  struct stmt *stmt = malloc(sizeof(*stmt));
  stmt->kind = EXPR_KIND;
  stmt->expr = expr;
  return stmt;
}

struct stmt *stmt_from_import(char *id, char *path)
{
  struct stmt *stmt = malloc(sizeof(*stmt));
  stmt->kind = IMPORT_KIND;
  if (id == NULL) {
    char *s = strrchr(path, '/');
    if (s == NULL)
      s = path;
    else
      s += 1;
    char *tmp = malloc(strlen(s) + 1);
    strcpy(tmp, s);
    stmt->import.id = tmp;
  } else {
    stmt->import.id = id;
  }
  stmt->import.path = path;
  return stmt;
}

int vars_add_symtable(struct clist *list, int bconst, struct type *type)
{
  return 0;
}

struct stmt *stmt_from_vardecl(struct sequence *varseq,
                               struct sequence *initseq,
                               int bconst, struct type *type)
{
  struct stmt *stmt = malloc(sizeof(*stmt));
  stmt->kind = VARDECL_KIND;
  stmt->vardecl.bconst   = bconst;
  stmt->vardecl.var_seq  = varseq;
  stmt->vardecl.expr_seq = initseq;

  if (type != NULL) {
    struct expr *var;
    for (int i = 0; i < seq_size(varseq); i++) {
      var = seq_get(varseq, i);
      assert(var->kind == NAME_KIND);
      var->type = type;
    }
  }
  return stmt;
}

struct stmt *stmt_from_funcdecl(char *sid, char *fid,
                                struct sequence *pseq, struct sequence *rseq,
                                struct sequence *body)
{
  struct stmt *stmt = malloc(sizeof(*stmt));
  stmt->kind = FUNCDECL_KIND;
  stmt->funcdecl.sid  = sid;
  stmt->funcdecl.fid  = fid;
  stmt->funcdecl.pseq = pseq;
  stmt->funcdecl.rseq = rseq;
  stmt->funcdecl.body = body;
  return stmt;
}

struct stmt *stmt_from_assign(struct sequence *left_seq,
                              struct sequence *right_seq)
{
  struct stmt *stmt = malloc(sizeof(*stmt));
  stmt->kind = ASSIGN_KIND;
  stmt->assign.left_seq  = left_seq;
  stmt->assign.right_seq = right_seq;
  return stmt;
}

struct stmt *stmt_from_compound_assign(struct expr *left,
                                       enum assign_operator op,
                                       struct expr *right)
{
  struct stmt *stmt = malloc(sizeof(*stmt));
  stmt->kind = COMPOUND_ASSIGN_KIND;
  stmt->compound_assign.left  = left;
  stmt->compound_assign.op    = op;
  stmt->compound_assign.right = right;
  return stmt;
}

struct stmt *stmt_from_block(struct sequence *block)
{
  struct stmt *stmt = malloc(sizeof(*stmt));
  stmt->kind = BLOCK_KIND;
  stmt->seq  = block;
  return stmt;
}

struct stmt *stmt_from_return(struct sequence *seq)
{
  struct stmt *stmt = malloc(sizeof(*stmt));
  stmt->kind = RETURN_KIND;
  stmt->seq  = seq;
  return stmt;
}

struct stmt *stmt_from_empty(void)
{
  struct stmt *stmt = malloc(sizeof(*stmt));
  stmt->kind = EMPTY_KIND;
  return stmt;
}

struct stmt *stmt_from_structure(char *id, struct sequence *seq)
{
  struct stmt *stmt = malloc(sizeof(*stmt));
  stmt->kind = STRUCT_KIND;
  stmt->structure.id  = id;
  stmt->structure.seq = seq;
  return stmt;
}

struct stmt *stmt_from_interface(char *id, struct sequence *seq)
{
  struct stmt *stmt = malloc(sizeof(*stmt));
  stmt->kind = INTF_KIND;
  stmt->structure.id  = id;
  stmt->structure.seq = seq;
  return stmt;
}

struct stmt *stmt_from_jump(int kind)
{
  struct stmt *stmt = malloc(sizeof(*stmt));
  stmt->kind = kind;
  return stmt;
}

struct stmt *stmt_from_if(struct test_block *if_part,
                          struct sequence *elseif_seq,
                          struct test_block *else_part)
{
  struct stmt *stmt = malloc(sizeof(*stmt));
  stmt->kind = IF_KIND;
  stmt->if_stmt.if_part    = if_part;
  stmt->if_stmt.elseif_seq = elseif_seq;
  stmt->if_stmt.else_part  = else_part;
  return stmt;
}

struct test_block *new_test_block(struct expr *test, struct sequence *body)
{
  struct test_block *tb = malloc(sizeof(*tb));
  tb->test = test;
  tb->body = body;
  return tb;
}

struct stmt *stmt_from_while(struct expr *test, struct sequence *body, int b)
{
  struct stmt *stmt = malloc(sizeof(*stmt));
  stmt->kind = WHILE_KIND;
  stmt->while_stmt.btest = b;
  stmt->while_stmt.test  = test;
  stmt->while_stmt.body  = body;
  return stmt;
}

struct stmt *stmt_from_switch(struct expr *expr, struct sequence *case_seq)
{
  struct stmt *stmt = malloc(sizeof(*stmt));
  stmt->kind = SWITCH_KIND;
  stmt->switch_stmt.expr = expr;
  stmt->switch_stmt.case_seq = case_seq;
  return stmt;
}

struct stmt *stmt_from_for(struct stmt *init, struct stmt *test,
                           struct stmt *incr, struct sequence *body)
{
  struct stmt *stmt = malloc(sizeof(*stmt));
  stmt->kind = FOR_TRIPLE_KIND;
  stmt->for_triple_stmt.init = init;
  stmt->for_triple_stmt.test = test;
  stmt->for_triple_stmt.incr = incr;
  stmt->for_triple_stmt.body = body;
  return stmt;
}

struct stmt *stmt_from_foreach(struct expr *var, struct expr *expr,
                              struct sequence *body, int bdecl)
{
  struct stmt *stmt = malloc(sizeof(*stmt));
  stmt->kind = FOR_EACH_KIND;
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

  struct stmt *stmt = malloc(sizeof(*stmt));
  stmt->kind    = GO_KIND;
  stmt->go_stmt = expr;
  return stmt;
}

struct field *new_struct_field(char *id, struct type *t, struct expr *e)
{
  return NULL;
}

struct intf_func *new_intf_func(char *id, struct sequence *pseq,
                                struct sequence *rseq)
{
  return NULL;
}

#if 0
void type_traverse(struct type *type)
{
  if (type != NULL) {
    printf("type_kind:%d\n", type->kind);
    printf("type_dims:%d\n", type->dims);
  } else {
    printf("no type declared\n");
  }
}

void array_tail_traverse(struct array_tail *tail)
{
  if (tail->list) {
    printf("subarray,length:%d\n", tail->list->count);
    assert(!tail->expr);
    int count = 0;
    struct array_tail *t;
    clist_foreach(t, tail->list) {
      count++;
      array_tail_traverse(t);
    }
    printf("real subcount222:%d\n", count);
  } else {
    printf("tail expr\n");
    assert(tail->expr);
    expr_traverse(tail->expr);
    printf("tail expr end\n");
  }
}

void array_traverse(struct expr *expr)
{
  type_traverse(expr->v.array.type);
  if (expr->v.array.tail_list != NULL) {
    printf("array tail list, length:%d\n", expr->v.array.tail_list->count);
    int count = 0;
    struct array_tail *tail;
    clist_foreach(tail, expr->v.array.tail_list) {
      count++;
      printf("start subarray\n");
      array_tail_traverse(tail);
      printf("end subarray\n");
    }
    printf("real count:%d\n", count);
  } else {
    printf("array dim list, length:%d\n", expr->v.array.dim_list->count);
  }
}

void expr_traverse(struct expr *expr)
{
  switch (expr->kind) {
    case NAME_KIND: {
      /*
      id scope
      method:
      local variable -> field variable -> module variable
      -> external module name
      function:
      module variable -> external module name
      */
      printf("[id]\n");
      printf("%s, %s\n", expr->v.name.id, ni_type_string(expr->v.name.type));
      break;
    }
    case INT_KIND: {
      printf("[integer]\n");
      printf("%lld\n", expr->v.ival);
      break;
    }
    case FLOAT_KIND: {
      printf("[float]\n");
      printf("%f\n", expr->v.fval);
      break;
    }
    case STRING_KIND: {
      printf("[string]\n");
      printf("%s\n", expr->v.str);
      break;
    }
    case BOOL_KIND: {
      printf("[boolean]\n");
      printf("%s\n", expr->v.bval ? "true" : "false");
      break;
    }
    case SELF_KIND: {
      printf("[self]\n");
      break;
    }
    case NULL_KIND: {
      printf("[null]\n");
      break;
    }
    case EXP_KIND: {
      printf("[sub-expr]\n");
      expr_traverse(expr->v.exp);
      break;
    }
    case NEW_PRIMITIVE_KIND: {
      printf("[new primitive object]\n");
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
      expr_traverse(expr->v.attribute.expr);
      printf("[attribute]\n");
      printf("%s, %s\n", expr->v.attribute.id,
             ni_type_string(expr->v.attribute.type));
      break;
    }
    case SUBSCRIPT_KIND: {
      expr_traverse(expr->v.subscript.expr);
      printf("[subscript]\n");
      expr_traverse(expr->v.subscript.index);
      break;
    }
    case CALL_KIND: {
      expr_traverse(expr->v.call.expr);
      printf("[func call]\n");
      printf("paras:\n");
      struct expr *expr;
      clist_foreach(expr, expr->v.call.list) {
        expr_traverse(expr);
      }
      break;
    }
    case INTF_IMPL_KIND:
    default: {
      printf("[ERROR] unknown expr kind :%d\n", expr->kind);
      assert(0);
      break;
    }
  }
}

void expr_traverse(struct expr *exp)
{
  if (exp == NULL) return;

  switch (exp->kind) {
    case ATOM_KIND: {
      expr_traverse(exp->v.expr);
      break;
    }
    case UNARY_KIND: {
      printf("unary expr,op:%d\n", exp->v.unary_op.op);
      expr_traverse(exp->v.unary_op.operand);
      break;
    }
    case BINARY_KIND: {
      printf("binary expr,op:%d\n", exp->v.bin_op.op);
      expr_traverse(exp->v.bin_op.left);
      expr_traverse(exp->v.bin_op.right);
      break;
    }
    default: {
      assert(0);
      break;
    }
  }
}

void vardecl_traverse(struct stmt *stmt)
{
  printf("variable declaration\n");
  printf("const variable ? %s\n",
         stmt->v.vardecl.bconst ? "true" : "false");
  printf("variables name:\n");
  struct var *var;
  struct sequence *var_seq = stmt->v.vardecl.var_seq;
  for (int i = 0; i < seq_size(var_seq); i++) {
    var = seq_get(var_seq, i);
    printf("%s ", var->id);
    type_traverse(var->type);
  }

  putchar('\n');

  struct expr *expr;
  struct sequence *expr_seq = stmt->v.vardecl.expr_seq;
  for (int i = 0; i < seq_size(expr_seq); i++) {
    expr = seq_get(expr_seq, i);
    expr_traverse(expr);
  }

  printf("end variable declaration\n");
}

void stmt_traverse(struct stmt *stmt);

void ifexpr_traverse(struct if_expr *ifexpr)
{
  if (ifexpr == NULL) return;

  if (ifexpr->test != NULL) {
    printf("test part:\n");
    expr_traverse(ifexpr->test);
  }

  if (ifexpr->body != NULL) {
    printf("body part:\n");
    struct stmt *pos;
    clist_foreach(pos, ifexpr->body) {
      stmt_traverse(pos);
    }
  }
}

void ifstmt_traverse(struct stmt *ifstmt)
{
  printf("if-condition:\n");
  ifexpr_traverse(ifstmt->v.if_stmt.if_part);
  printf("else-if-list:\n");
  struct if_expr *pos;
  clist_foreach(pos, ifstmt->v.if_stmt.elseif_list) {
    ifexpr_traverse(pos);
  }
  printf("else:\n");
  ifexpr_traverse(ifstmt->v.if_stmt.else_part);
  printf("end of if statement\n");
}

void whilestmt_traverse(struct stmt *whilestmt)
{
  printf("while condition: first test ? %d\n", whilestmt->v.while_stmt.btest);
  expr_traverse(whilestmt->v.while_stmt.test);
  printf("while body:\n");
  struct stmt *pos;
  clist_foreach(pos, whilestmt->v.while_stmt.body) {
    stmt_traverse(pos);
  }
  printf("end of while statement\n");
}

void switchstmt_traverse(struct stmt *stmt)
{
  printf("switch-expr:\n");
  expr_traverse(stmt->v.switch_stmt.expr);
  printf("case-list:\n");
  struct case_stmt *pos;
  struct stmt *s;
  clist_foreach(pos, stmt->v.switch_stmt.case_list) {
    expr_traverse(pos->expr);
    clist_foreach(s, pos->body) {
      stmt_traverse(s);
    }
  }
  printf("end of switch\n");
}

#endif

void stmt_traverse(struct stmt *stmt)
{
  switch (stmt->kind) {
    case EMPTY_KIND: {
      printf("[empty statement]\n");
      break;
    }
    case IMPORT_KIND: {
      printf("[import]\n");
      printf("%s:%s\n", stmt->import.id, stmt->import.path);
      break;
    }
    case EXPR_KIND: {
      printf("[expr]\n");
      //expr_traverse(stmt->v.expr);
      break;
    }
    case VARDECL_KIND: {
      printf("[var decl]\n");
      //vardecl_traverse(stmt);
      break;
    }
    case FUNCDECL_KIND: {
      printf("[functin decl]:%s.%s\n", stmt->funcdecl.sid, stmt->funcdecl.fid);
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
    case STRUCT_KIND: {
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
    case GO_KIND: {
      printf("[go statement]\n");
      // expr_traverse(stmt->v.expr);
      break;
    }
    default:{
      printf("[ERROR] unknown stmt kind :%d\n", stmt->kind);
      assert(0);
    }
  }
}

void ast_traverse(struct sequence *seq)
{
  for (int i = 0; i < seq_size(seq); i++)
    stmt_traverse(seq_get(seq, i));
}

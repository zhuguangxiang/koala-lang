
#include "compile.h"
#include "hash.h"

Import *import_new(char *id, char *path)
{
  Import *import = malloc(sizeof(Import));
  import->id = id;
  import->path = path;
  Init_HashNode(&import->hnode, import);
  return import;
}

uint32 import_hash(void *k)
{
  Import *import = k;
  return hash_string(import->id);
}

int import_equal(void *k1, void *k2)
{
  Import *import1 = k1;
  Import *import2 = k2;
  return !strcmp(import1->id, import2->id);
}

int init_compiler(ParserContext *ctx)
{
  Koala_Init();
  Decl_HashInfo(hashinfo, import_hash, import_equal);
  HashTable_Init(&ctx->imports, &hashinfo);
  Vector_Init(&ctx->stmts);
  STable_Init(&ctx->stable, NULL);
  Vector_Init(&ctx->__initstmts__);
  ctx->scope = 0;
  init_list_head(&ctx->scopes);
  return 0;
}

int fini_compiler(ParserContext *ctx)
{
  Koala_Fini();
  return 0;
}

static ScopeContext *scope_new(ParserContext *ctx)
{
  ScopeContext *scope = malloc(sizeof(ScopeContext));
  init_list_head(&scope->link);
  STable_Init(&scope->stable, ctx->stable.itable);
  return scope;
}

static void scope_free(ScopeContext *scope)
{
  ASSERT(list_unlinked(&scope->link));
  STable_Fini(&scope->stable);
  free(scope);
}

static ScopeContext *get_scope(ParserContext *ctx)
{
  struct list_head *first = list_first(&ctx->scopes);
  if (first != NULL) return container_of(first, ScopeContext, link);
  else return NULL;
}

static void scope_enter(ParserContext *ctx)
{
  ctx->scope++;
  ScopeContext *scope = scope_new(ctx);
  list_add(&scope->link, &ctx->scopes);
}

static void scope_exit(ParserContext *ctx)
{
  ctx->scope--;
  ScopeContext *scope = get_scope(ctx);
  ASSERT_PTR(scope);
  list_del(&scope->link);
  scope_free(scope);
}

Symbol *parser_find_symbol(ParserContext *ctx, char *name)
{
  ScopeContext *scope;
  Symbol *sym;
  list_for_each_entry(scope, &ctx->scopes, link) {
    sym = STable_Get(&scope->stable, name);
    if (sym != NULL) return sym;
  }

  /* find global symbol table */
  return STable_Get(&ctx->stable, name);
}

char *type_full_path(ParserContext *ctx, struct type *type)
{
  Import temp = {.id = type->userdef.mod};
  Import *import = HashTable_FindObject(&ctx->imports, &temp, Import);
  ASSERT_PTR(import);
  int len = strlen(import->path) + strlen(type->userdef.type) + 2;
  char *fullpath = malloc(len);
  sprintf(fullpath, "%s.%s", import->path, type->userdef.type);
  fullpath[len - 1] = '\0';
  return fullpath;
}

int type_to_desc(ParserContext *ctx, struct type *type, TypeDesc *desc)
{
  if (type == NULL) return -1;

  if (type->kind == PRIMITIVE_KIND) {
    Init_Primitive_Desc(desc, type->dims, type->primitive);
  } else if (type->kind == USERDEF_TYPE) {
    Init_UserDef_Desc(desc, type->dims, type_full_path(ctx, type));
  } else {
    ASSERT(0);
  }
  return 0;
}

void parse_variable(ParserContext *ctx, struct var *var)
{
  TypeDesc desc;
  int res = type_to_desc(ctx, var->type, &desc);
  STable_Add_Var(&ctx->stable, var->id, res < 0 ? NULL : &desc, var->bconst);
}

void parse_variables(ParserContext *ctx, struct stmt *stmt)
{
  ASSERT(stmt->kind == VARDECL_KIND);
  Vector *vec = stmt->vardecl.var_seq;
  struct var *var;
  for (int i = 0; i < Vector_Size(vec); i++) {
    var = Vector_Get(vec, i);
    ASSERT_PTR(var);
    parse_variable(ctx, var);
  }
  struct stmt *init;
  init = stmt_from_initassign(stmt->vardecl.var_seq, stmt->vardecl.expr_seq);
  Vector_Appand(&ctx->__initstmts__, init);
}

int parse_args(ParserContext *ctx, struct stmt *stmt, TypeDesc **ret)
{
  Vector *vec = stmt->funcdecl.pseq;
  if (vec != NULL) {
    int sz = Vector_Size(vec);
    struct type *type;
    struct var *var;
    TypeDesc *desc = malloc(sizeof(TypeDesc) * sz);
    ASSERT_PTR(desc);
    ScopeContext *scope;
    for (int i = 0; i < sz; i++) {
      var = Vector_Get(vec, i);
      type = var->type;
      type_to_desc(ctx, type, desc + i);
      scope = get_scope(ctx);
      if (scope != NULL)
        STable_Add_Var(&scope->stable, var->id, desc + i, 0);
    }
    if (ret != NULL)
      *ret = desc;
    else
      free(desc);
    return sz;
  }
  if (ret != NULL) *ret = NULL;
  return 0;
}

/*--------------------------------------------------------------------------*/

int expr_handler(ParserContext *ctx, struct expr *exp);

int expr_id_handler(ParserContext *ctx, struct expr *exp)
{
  char *name = exp->name.id;
  Symbol *sym = parser_find_symbol(ctx, name);
  if (sym == NULL) {
    Import k = {.id = name};
    Import *import = HashTable_FindObject(&ctx->imports, &k, Import);
    if (import == NULL) {
      debug_error("cannot find symbol:%s\n", name);
    } else {
      debug_info("external symbol, its module's path: %s\n", import->path);
      //OP_LOADM
    }
  } else {
    debug_info("find symbol, position: '%d'\n", sym->index);
    //OP_LOAD
  }
  return 0;
}

int expr_attr_handler(ParserContext *ctx, struct expr *exp)
{
  debug_info("attribute\n");
  expr_set_ctx(exp->attribute.left, CTX_LOAD);
  expr_handler(ctx, exp->attribute.left);
  debug_info("%s\n", exp->attribute.id);
  //OP_GETFIELD
  return 0;
}

int expr_subscribe_handler(ParserContext *ctx, struct expr *exp)
{
  debug_info("subscribe\n");
  expr_handler(ctx, exp->subscript.left);
  expr_handler(ctx, exp->subscript.index);
  return 0;
}

int expr_call_handler(ParserContext *ctx, struct expr *exp)
{
  debug_info("call\n");
  expr_handler(ctx, exp->call.left);

  Vector *vec = exp->call.pseq;
  if (vec == NULL) {
    debug_info("no args' function call\n");
    return 0;
  }

  struct expr *e;
  int sz = Vector_Size(vec);
  for (int i = 0; i < sz; i++) {
    e = Vector_Get(vec, i);
  }
  return 0;
}

static expr_handler_t expr_handlers[] = {
  NULL,
  expr_id_handler, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL,
  NULL, NULL, expr_attr_handler, expr_subscribe_handler,
  expr_call_handler, NULL, NULL, NULL,
};

int expr_handler(ParserContext *ctx, struct expr *exp)
{
  ASSERT(exp->kind > 0 && exp->kind < EXPR_KIND_MAX);
  //printf("expr kind:%d\n", exp->kind);
  expr_handler_t handler = expr_handlers[exp->kind];
  ASSERT_PTR(handler);
  return handler(ctx, exp);
}

int expr_stmt_handler(ParserContext *ctx, struct stmt *stmt)
{
  debug_info("expression\n");
  struct expr *exp = stmt->expr;
  return expr_handler(ctx, exp);
}

int local_vardecl_stmt_handler(ParserContext *ctx, struct stmt *stmt)
{
  debug_info("local var decl\n");
  return 0;
}

int expr_assign_handler(ParserContext *ctx, struct stmt *stmt)
{
  debug_info("=\n");
  return 0;
}

int ret_stmt_handler(ParserContext *ctx, struct stmt *stmt)
{
  debug_info("return\n");
  return 0;
}

static stmt_handler_t localstmt_handlers[] = {
  NULL, /* INVALID */
  NULL, expr_stmt_handler, local_vardecl_stmt_handler, NULL,
  NULL, expr_assign_handler, NULL, NULL,
  NULL, ret_stmt_handler, NULL, NULL,
  NULL, NULL, NULL, NULL,
  NULL, NULL, NULL,
};

int localstmt_handler(ParserContext *ctx, struct stmt *stmt)
{
  ASSERT(stmt->kind > 0 && stmt->kind < STMT_KIND_MAX);
  //printf("localstmt kind:%d\n", stmt->kind);
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

/*--------------------------------------------------------------------------*/

int parse_rets(ParserContext *ctx, struct stmt *stmt, TypeDesc **ret)
{
  Vector *vec = stmt->funcdecl.rseq;
  if (vec != NULL) {
    int sz = Vector_Size(vec);
    struct type *type;
    TypeDesc *desc = malloc(sizeof(TypeDesc) * sz);
    ASSERT_PTR(desc);
    for (int i = 0; i < sz; i++) {
      type = Vector_Get(vec, i);
      type_to_desc(ctx, type, desc + i);
    }
    if (ret != NULL)
      *ret = desc;
    else
      free(desc);
    return sz;
  }
  if (ret != NULL) *ret = NULL;
  return 0;
}

void parse_function(ParserContext *ctx, struct stmt *stmt)
{
  ASSERT(stmt->kind == FUNCDECL_KIND);
  ProtoInfo proto;
  proto.psz = parse_args(ctx, stmt, &proto.pdesc);
  proto.rsz = parse_rets(ctx, stmt, &proto.rdesc);
  STable_Add_Func(&ctx->stable, stmt->funcdecl.id, &proto);
}

int func_generate_code(ParserContext *ctx, struct stmt *stmt)
{
  ASSERT(stmt->kind == FUNCDECL_KIND);
  debug_info("parsing func %s\n", stmt->funcdecl.id);
  scope_enter(ctx);
  parse_args(ctx, stmt, NULL);
  parse_body(ctx, stmt);
  scope_exit(ctx);
  return 0;
}

int import_stmt_handler(ParserContext *ctx, struct stmt *stmt)
{
  if (ctx->state == PARSING_SYMBOLS) {
    ASSERT(stmt->kind == IMPORT_KIND);
    Import *import = import_new(stmt->import.id, stmt->import.path);
    return HashTable_Insert(&ctx->imports, &import->hnode);
  }
  return 0;
}

int vardecl_stmt_handler(ParserContext *ctx, struct stmt *stmt)
{
  if (ctx->state == PARSING_SYMBOLS) {
    parse_variables(ctx, stmt);
  }
  return 0;
}

int initdecl_stmt_handler(ParserContext *ctx, struct stmt *stmt)
{
  if (ctx->state == PARSING_INITFUNC) {
    //func_generate_code(ctx, stmt);
  }
  return 0;
}

int funcdecl_stmt_handler(ParserContext *ctx, struct stmt *stmt)
{
  if (ctx->state == PARSING_SYMBOLS) {
    parse_function(ctx, stmt);
  } else if (ctx->state == PARSING_FUNCTIONS) {
    func_generate_code(ctx, stmt);
  } else {
    ASSERT_MSG("cannot compile source file with 3rd time\n");
  }
  return 0;
}

static stmt_handler_t stmt_handlers[] = {
  NULL, /* INVALID */
  import_stmt_handler,
  NULL,
  vardecl_stmt_handler,
  initdecl_stmt_handler,
  funcdecl_stmt_handler,
};

int stmt_handler(ParserContext *ctx, struct stmt *stmt)
{
  ASSERT(stmt->kind > 0 && stmt->kind < STMT_KIND_MAX);
  stmt_handler_t handler = stmt_handlers[stmt->kind];
  ASSERT_PTR(handler);
  return handler(ctx, stmt);
}

static void show(ParserContext *ctx)
{
  printf("package:%s\n", ctx->package);
  STable_Show(&ctx->stable);
}

int compile(ParserContext *ctx)
{
  int i;
  struct stmt *stmt;
  Vector *vec = &ctx->stmts;
  printf("-----------------------\n");

  /* save all symbols */
  ctx->state = PARSING_SYMBOLS;
  for (i = 0; i < Vector_Size(vec); i++) {
    stmt = Vector_Get(vec, i);
    stmt_handler(ctx, stmt);
  }

  /* compile all functions */
  ctx->state = PARSING_FUNCTIONS;
  for (i = 0; i < Vector_Size(vec); i++) {
    stmt = Vector_Get(vec, i);
    stmt_handler(ctx, stmt);
  }

  /* compile __init__ function */
  ctx->state = PARSING_INITFUNC;
  vec = &ctx->__initstmts__;
  for (i = 0; i < Vector_Size(vec); i++) {
    stmt = Vector_Get(vec, i);
    stmt_handler(ctx, stmt);
  }

  /* generate .klc image file */

  /* show all symbols */
  show(ctx);

  return 0;
}

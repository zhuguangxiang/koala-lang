
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

int init_compiler(CompileContext *ctx)
{
  Koala_Init();
  Decl_HashInfo(hashinfo, import_hash, import_equal);
  HashTable_Init(&ctx->imports, &hashinfo);
  Vector_Init(&ctx->stmts);
  STable_Init(&ctx->stable);
  Vector_Init(&ctx->__initstmts__);
  ctx->scope = 0;
  init_list_head(&ctx->scopes);
  return 0;
}

int fini_compiler(CompileContext *ctx)
{
  Koala_Fini();
  return 0;
}

static ScopeContext *scope_new(void)
{
  ScopeContext *scope = malloc(sizeof(ScopeContext));
  init_list_head(&scope->link);
  STable_Init(&scope->stable);
  return scope;
}

static void scope_free(ScopeContext *scope)
{
  ASSERT(list_unlinked(&scope->link));
  STable_Fini(&scope->stable);
  free(scope);
}

static ScopeContext *get_scope(CompileContext *ctx)
{
  struct list_head *first = list_first(&ctx->scopes);
  if (first != NULL) return container_of(first, ScopeContext, link);
  else return NULL;
}

static void scope_enter(CompileContext *ctx)
{
  ctx->scope++;
  ScopeContext *scope = scope_new();
  list_add(&scope->link, &ctx->scopes);
}

static void scope_exit(CompileContext *ctx)
{
  ctx->scope--;
  ScopeContext *scope = get_scope(ctx);
  ASSERT_PTR(scope);
  list_del(&scope->link);
  scope_free(scope);
}

char *type_full_path(CompileContext *ctx, struct type *type)
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

int type_to_desc(CompileContext *ctx, struct type *type, TypeDesc *desc)
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

void parse_variable(CompileContext *ctx, struct var *var)
{
  TypeDesc desc;
  int res = type_to_desc(ctx, var->type, &desc);
  STable_Add_Var(&ctx->stable, var->id, res < 0 ? NULL : &desc, var->bconst);
}

void parse_variables(CompileContext *ctx, struct stmt *stmt)
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

int parse_args(CompileContext *ctx, struct stmt *stmt, TypeDesc **ret)
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

int parse_rets(CompileContext *ctx, struct stmt *stmt, TypeDesc **ret)
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

int parse_body(CompileContext *ctx, struct stmt *stmt)
{
  Vector *vec = stmt->funcdecl.body;
  if (vec == NULL) return 0;
  int sz = Vector_Size(vec);
  struct stmt *temp;
  for (int i = 0; i < sz; i++) {
    temp = Vector_Get(vec, i);
  }
}

void parse_function(CompileContext *ctx, struct stmt *stmt)
{
  ASSERT(stmt->kind == FUNCDECL_KIND);
  ProtoInfo proto;
  proto.psz = parse_args(ctx, stmt, &proto.pdesc);
  proto.rsz = parse_rets(ctx, stmt, &proto.rdesc);
  STable_Add_Func(&ctx->stable, stmt->funcdecl.id, &proto);
}

int func_generate_code(CompileContext *ctx, struct stmt *stmt)
{
  ASSERT(stmt->kind == FUNCDECL_KIND);
  printf("parsing func %s\n", stmt->funcdecl.id);
  scope_enter(ctx);
  parse_args(ctx, stmt, NULL);
  parse_body(ctx, stmt);
  scope_exit(ctx);
  return 0;
}

int import_stmt_handler(CompileContext *ctx, struct stmt *stmt)
{
  if (ctx->state == PARSING_SYMBOLS) {
    ASSERT(stmt->kind == IMPORT_KIND);
    Import *import = import_new(stmt->import.id, stmt->import.path);
    return HashTable_Insert(&ctx->imports, &import->hnode);
  }
  return 0;
}

int vardecl_stmt_handler(CompileContext *ctx, struct stmt *stmt)
{
  if (ctx->state == PARSING_SYMBOLS) {
    parse_variables(ctx, stmt);
  }
  return 0;
}

int initdecl_stmt_handler(CompileContext *ctx, struct stmt *stmt)
{
  if (ctx->state == PARSING_INITFUNC) {
    //func_generate_code(ctx, stmt);
  }
  return 0;
}

int funcdecl_stmt_handler(CompileContext *ctx, struct stmt *stmt)
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

typedef int (*stmt_handler_t)(CompileContext *, struct stmt *);

static stmt_handler_t stmt_handlers[] = {
  NULL, /* INVALID */
  import_stmt_handler,
  NULL,
  vardecl_stmt_handler,
  initdecl_stmt_handler,
  funcdecl_stmt_handler,
};

int stmt_handler(CompileContext *ctx, struct stmt *stmt)
{
  ASSERT(stmt->kind > 0 && stmt->kind < STMT_KIND_MAX);
  stmt_handler_t handler = stmt_handlers[stmt->kind];
  ASSERT_PTR(handler);
  return handler(ctx, stmt);
}

static void show(CompileContext *ctx)
{
  printf("package:%s\n", ctx->package);
  STable_Show(&ctx->stable);
}

int compile(CompileContext *ctx)
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

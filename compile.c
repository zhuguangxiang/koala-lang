
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
  ctx->stmts = Vector_Create();
  STable_Init(&ctx->stable);
  ctx->scope = 0;
  init_list_head(&ctx->scopes);
  return 0;
}

int fini_compiler(CompileContext *ctx)
{
  Koala_Fini();
  return 0;
}

static void scope_enter(CompileContext *ctx)
{
  UNUSED_PARAMETER(ctx);
}

static void scope_exit(CompileContext *ctx)
{
  UNUSED_PARAMETER(ctx);
}

static struct scope *get_scope(CompileContext *ctx)
{
  if (ctx->scope == 0) return NULL;
  struct list_head *first = list_first(&ctx->scopes);
  if (first != NULL) return container_of(first, struct scope, link);
  ASSERT(0); return NULL;
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

TypeDesc *types_to_desclist(CompileContext *ctx, int sz, struct type **type)
{
  if (sz == 0) return NULL;

  int res;
  TypeDesc *desc = malloc(sizeof(TypeDesc) * sz);
  for (int i = 0; i < sz; i++) {
    res = type_to_desc(ctx, type[i], desc + i);
    ASSERT(res >= 0);
  }
  return desc;
}

void add_variable(CompileContext *ctx, struct var *var, int bconst)
{
  TypeDesc desc;
  int res = type_to_desc(ctx, var->type, &desc);
  STable_Add_Var(&ctx->stable, var->id, res < 0 ? NULL : &desc, bconst);
}

void add_variables(CompileContext *ctx, struct stmt *stmt)
{
  ASSERT(stmt->kind == VARDECL_KIND);
  int bconst = stmt->vardecl.bconst;
  Vector *vec = stmt->vardecl.var_seq;
  struct var *var;
  for (int i = 0; i < Vector_Size(vec); i++) {
    var = Vector_Get(vec, i);
    ASSERT_PTR(var);
    add_variable(ctx, var, bconst);
  }
}

void add_function(CompileContext *ctx, struct stmt *stmt)
{
  ASSERT(stmt->kind == FUNCDECL_KIND);
  ProtoInfo proto = {0};

  Vector *vec = stmt->funcdecl.pseq;
  if (vec != NULL) {
    int psz = Vector_Size(vec);
    struct type *types[psz];
    struct var *var;
    for (int i = 0; i < psz; i++) {
      var = Vector_Get(vec, i);
      types[i] = var->type;
    }
    proto.psz = psz;
    proto.pdesc = types_to_desclist(ctx, psz, types);
  }

  vec = stmt->funcdecl.rseq;
  if (vec != NULL) {
    int rsz = Vector_Size(vec);
    struct type *types[rsz];
    for (int i = 0; i < rsz; i++) {
      types[i] = Vector_Get(vec, i);
    }
    proto.rsz = rsz;
    proto.rdesc = types_to_desclist(ctx, rsz, types);
  }

  STable_Add_Func(&ctx->stable, stmt->funcdecl.id, &proto);
}

int import_stmt_handler(CompileContext *ctx, struct stmt *stmt)
{
  if (ctx->times == 1) {
    ASSERT(stmt->kind == IMPORT_KIND);
    Import *import = import_new(stmt->import.id, stmt->import.path);
    return HashTable_Insert(&ctx->imports, &import->hnode);
  } else {
    debug_info("compile import stmt 2rd time\n");
    return 0;
  }
}

int vardecl_stmt_handler(CompileContext *ctx, struct stmt *stmt)
{
  if (ctx->times == 1) {
    add_variables(ctx, stmt);
    return 0;
  } else if (ctx->times == 2) {
    return 0;
  } else {
    ASSERT_MSG("cannot compile source file with 3rd time\n");
    return 0;
  }
}

int funcdecl_stmt_handler(CompileContext *ctx, struct stmt *stmt)
{
  if (ctx->times == 1) {
    add_function(ctx, stmt);
    return 0;
  } else if (ctx->times == 2) {
    return 0;
  } else {
    ASSERT_MSG("cannot compile source file with 3rd time\n");
    return 0;
  }
}

typedef int (*stmt_handler_t)(CompileContext *, struct stmt *);

static stmt_handler_t stmt_handlers[] = {
  NULL, /* INVALID */
  NULL, /* EMPTY_KIND */
  import_stmt_handler,
  NULL,
  vardecl_stmt_handler,
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
  struct stmt *stmt;
  Vector *vec = ctx->stmts;
  printf("-----------------------\n");

  ctx->times = 1;
  for (int i = 0; i < Vector_Size(vec); i++) {
    stmt = Vector_Get(vec, i);
    stmt_handler(ctx, stmt);
  }

  ctx->times = 2;
  for (int i = 0; i < Vector_Size(vec); i++) {
    stmt = Vector_Get(vec, i);
    stmt_handler(ctx, stmt);
  }

  show(ctx);

  return 0;
}

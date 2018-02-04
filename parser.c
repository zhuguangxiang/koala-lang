
#include "parser.h"
#include "koala.h"
#include "hash.h"
#include "codegen.h"

ParserState parser;

/*-------------------------------------------------------------------------*/

static Import *import_new(char *id, char *path, STable *stable, char *pkg)
{
  Import *import = malloc(sizeof(Import));
  if (id == NULL)
    import->id = pkg;
  else
    import->id = id;
  import->path = path;
  Init_HashNode(&import->hnode, import);
  import->stable = stable;
  import->refcnt = 0;
  return import;
}

static void import_free(Import *import)
{
  free(import);
}

static uint32 import_hash(void *k)
{
  Import *import = k;
  return hash_string(import->id);
}

static int import_equal(void *k1, void *k2)
{
  Import *import1 = k1;
  Import *import2 = k2;
  return !strcmp(import1->id, import2->id);
}

static void init_imports(ParserState *parser)
{
  Decl_HashInfo(hashinfo, import_hash, import_equal);
  HashTable_Init(&parser->imports, &hashinfo);
  Object *ob = Koala_Get_Module("koala/lang");
  ASSERT_MSG(ob != NULL, "cannot load module 'koala/lang'");
  STable *stable = Module_Get_STable(ob);
  ASSERT_PTR(stable);
  Import *import = import_new("lang", "koala/lang", stable, NULL);
  import->refcnt = 1;
  HashTable_Insert(&parser->imports, &import->hnode);
}

/*-------------------------------------------------------------------------*/

// static Scope *scope_new(Parser *parser)
// {
//   Scope *scope = malloc(sizeof(Scope));
//   init_list_head(&scope->link);
//   STable_Init(&scope->stable, Module_ItemTable(parser->module));
//   return scope;
// }

// static void scope_free(Scope *scope)
// {
//   ASSERT(list_unlinked(&scope->link));
//   STable_Fini(&scope->stable);
//   free(scope);
// }

// static Scope *get_scope(Parser *parser)
// {
//   struct list_head *first = NULL; //list_first(&parser->scopes);
//   if (first != NULL) return container_of(first, Scope, link);
//   else return NULL;
// }

// static void scope_enter(Parser *parser)
// {
//   //parser->scope++;
//   Scope *scope = scope_new(parser);
//   //list_add(&scope->link, &parser->scopes);
// }

// static void scope_exit(Parser *parser)
// {
//   //parser->scope--;
//   Scope *scope = get_scope(parser);
//   ASSERT_PTR(scope);
//   list_del(&scope->link);
//   scope_free(scope);
// }

// Symbol *parser_find_symbol(Parser *parser, char *name)
// {
//   Scope *scope;
//   Symbol *sym;
//   // list_for_each_entry(scope, &parser->scopes, link) {
//   //   sym = STable_Get(&scope->stable, name);
//   //   if (sym != NULL) return sym;
//   // }

//   /* find global symbol table */
//   return NULL;//STable_Get(&parser->stable, name);
// }

/*-------------------------------------------------------------------------*/

char *type_get_fullpath(ParserState *parser, struct type *type)
{
  Import temp = {.id = type->userdef.mod};
  Import *import = HashTable_FindObject(&parser->imports, &temp, Import);
  if (import == NULL) return NULL;
  import->refcnt = 1;

  Symbol *symbol = STable_Get(import->stable, type->userdef.type);
  if (symbol == NULL) {
    error("cannot find type: %s.%s", import->path, type->userdef.type);
    return NULL;
  } else {
    if (symbol->kind != SYM_CLASS && symbol->kind != SYM_INTF) {
      error("symbol(%d) is not class or interface", symbol->kind);
      return NULL;
    }
  }

  int len = strlen(import->path) + strlen(type->userdef.type) + 2;
  char *fullpath = malloc(len);
  sprintf(fullpath, "%s.%s", import->path, type->userdef.type);
  fullpath[len - 1] = '\0';
  return fullpath;
}

int type_to_desc(ParserState *parser, struct type *type, TypeDesc *desc)
{
  ASSERT_PTR(type);
  if (type->kind == PRIMITIVE_KIND) {
    Init_Primitive_Desc(desc, type->dims, type->primitive);
  } else if (type->kind == USERDEF_KIND) {
    char *path = type_get_fullpath(parser, type);
    if (path == NULL) return -1;
    Init_UserDef_Desc(desc, type->dims, path);
  } else {
    ASSERT(0);
  }
  return 0;
}

/*-------------------------------------------------------------------------*/

// int parse_args(Parser *parser, struct stmt *stmt, TypeDesc **ret)
// {
//   Vector *vec = stmt->funcdecl.pseq;
//   if (vec != NULL) {
//     int sz = Vector_Size(vec);
//     struct type *type;
//     struct var *var;
//     TypeDesc *desc = malloc(sizeof(TypeDesc) * sz);
//     ASSERT_PTR(desc);
//     Scope *scope;
//     for (int i = 0; i < sz; i++) {
//       var = Vector_Get(vec, i);
//       type = var->type;
//       type_to_desc(parser, type, desc + i);
//       scope = get_scope(parser);
//       if (scope != NULL)
//         STable_Add_Var(&scope->stable, var->id, desc + i, 0);
//     }
//     if (ret != NULL)
//       *ret = desc;
//     else
//       free(desc);
//     return sz;
//   }
//   if (ret != NULL) *ret = NULL;
//   return 0;
// }

// int parse_rets(Parser *parser, struct stmt *stmt, TypeDesc **ret)
// {
//   Vector *vec = stmt->funcdecl.rseq;
//   if (vec != NULL) {
//     int sz = Vector_Size(vec);
//     struct type *type;
//     TypeDesc *desc = malloc(sizeof(TypeDesc) * sz);
//     ASSERT_PTR(desc);
//     for (int i = 0; i < sz; i++) {
//       type = Vector_Get(vec, i);
//       type_to_desc(parser, type, desc + i);
//     }
//     if (ret != NULL)
//       *ret = desc;
//     else
//       free(desc);
//     return sz;
//   }
//   if (ret != NULL) *ret = NULL;
//   return 0;
// }

/*--------------------------------------------------------------------------*/

// int expr_handler(Parser *parser, struct expr *exp);

// int expr_id_handler(Parser *parser, struct expr *exp)
// {
//   char *name = exp->name.id;
//   Symbol *sym = parser_find_symbol(parser, name);
//   if (sym == NULL) {
//     Import k = {.id = name};
//     Import *import = HashTable_FindObject(&parser->imports, &k, Import);
//     if (import == NULL) {
//       error("cannot find symbol:%s\n", name);
//     } else {
//       info("external symbol, its module's path: %s\n", import->path);
//       //OP_LOADM
//     }
//   } else {
//     info("find symbol, position: '%d'\n", sym->index);
//     //OP_LOAD
//   }
//   return 0;
// }

// int expr_attr_handler(Parser *parser, struct expr *exp)
// {
//   info("attribute\n");
//   expr_set_ctx(exp->attribute.left, CTX_LOAD);
//   expr_handler(parser, exp->attribute.left);
//   info("%s\n", exp->attribute.id);
//   //OP_GETFIELD
//   return 0;
// }

// int expr_subscribe_handler(Parser *parser, struct expr *exp)
// {
//   info("subscribe\n");
//   expr_handler(parser, exp->subscript.left);
//   expr_handler(parser, exp->subscript.index);
//   return 0;
// }

// int expr_call_handler(Parser *parser, struct expr *exp)
// {
//   info("call\n");
//   expr_handler(parser, exp->call.left);

//   Vector *vec = exp->call.pseq;
//   if (vec == NULL) {
//     info("no args' function call\n");
//     return 0;
//   }

//   struct expr *e;
//   int sz = Vector_Size(vec);
//   for (int i = 0; i < sz; i++) {
//     e = Vector_Get(vec, i);
//   }
//   return 0;
// }

// static expr_handler_t expr_handlers[] = {
//   NULL,
//   expr_id_handler, NULL, NULL, NULL,
//   NULL, NULL, NULL, NULL,
//   NULL, NULL, expr_attr_handler, expr_subscribe_handler,
//   expr_call_handler, NULL, NULL, NULL,
// };

// int expr_handler(Parser *parser, struct expr *exp)
// {
//   ASSERT(exp->kind > 0 && exp->kind < EXPR_KIND_MAX);
//   //printf("expr kind:%d\n", exp->kind);
//   expr_handler_t handler = expr_handlers[exp->kind];
//   ASSERT_PTR(handler);
//   return handler(parser, exp);
// }

// int expr_stmt_handler(Parser *parser, struct stmt *stmt)
// {
//   info("expression\n");
//   struct expr *exp = stmt->expr;
//   return expr_handler(parser, exp);
// }

// int local_vardecl_stmt_handler(Parser *parser, struct stmt *stmt)
// {
//   info("local var decl\n");
//   return 0;
// }

// int expr_assign_handler(Parser *parser, struct stmt *stmt)
// {
//   info("=\n");
//   return 0;
// }

// int ret_stmt_handler(Parser *parser, struct stmt *stmt)
// {
//   info("return\n");
//   return 0;
// }

// static stmt_handler_t localstmt_handlers[] = {
//   NULL, /* INVALID */
//   NULL, expr_stmt_handler, local_vardecl_stmt_handler, NULL,
//   NULL, expr_assign_handler, NULL, NULL,
//   NULL, ret_stmt_handler, NULL, NULL,
//   NULL, NULL, NULL, NULL,
//   NULL, NULL, NULL,
// };

// int localstmt_handler(Parser *parser, struct stmt *stmt)
// {
//   ASSERT(stmt->kind > 0 && stmt->kind < STMT_KIND_MAX);
//   //printf("localstmt kind:%d\n", stmt->kind);
//   stmt_handler_t handler = localstmt_handlers[stmt->kind];
//   ASSERT_PTR(handler);
//   return handler(parser, stmt);
// }

// int parse_body(Parser *parser, struct stmt *stmt)
// {
//   Vector *vec = stmt->funcdecl.body;
//   if (vec == NULL) return 0;
//   int sz = Vector_Size(vec);
//   struct stmt *temp;
//   for (int i = 0; i < sz; i++) {
//     temp = Vector_Get(vec, i);
//     localstmt_handler(parser, temp);
//   }
//   return 0;
// }

// /*--------------------------------------------------------------------------*/

// void parse_function(Parser *parser, struct stmt *stmt)
// {
//   ASSERT(stmt->kind == FUNCDECL_KIND);
//   ProtoInfo proto;
//   proto.psz = parse_args(parser, stmt, &proto.pdesc);
//   proto.rsz = parse_rets(parser, stmt, &proto.rdesc);
//   //STable_Add_Func(&parser->stable, stmt->funcdecl.id, &proto);
// }

// int func_generate_code(Parser *parser, struct stmt *stmt)
// {
//   ASSERT(stmt->kind == FUNCDECL_KIND);
//   info("parsing func %s\n", stmt->funcdecl.id);
//   scope_enter(parser);
//   parse_args(parser, stmt, NULL);
//   parse_body(parser, stmt);
//   scope_exit(parser);
//   return 0;
// }

/*--------------------------------------------------------------------------*/

// void init_funcdata(FuncData *func, Object *owner)
// {
//   func->owner = owner;
//   func->scope = 0;
//   init_list_head(&func->scopes);
//   Buffer_Init(&func->buf, 128);
//   Decl_HashInfo(hashinfo, item_hash, item_equal);
//   func->itable = ItemTable_Create(&hashinfo, ITEM_MAX);
//   func->stacksize = 0;
// }

/*--------------------------------------------------------------------------*/

// void generate_initfunc_code(Parser *parser, struct stmt *stmt)
// {
//   expr_generate_code(&parser->func, stmt->vardecl.exp);
//   FuncData *func = &parser->func;
//   struct var *var = stmt->vardecl.var;
//   Symbol *sym = STable_Get(Object_STable(func->owner), var->id);
//   ASSERT_PTR(sym); ASSERT(sym->name_index >= 0);
//   int index = ConstItem_Set_String(func->itable, sym->name_index);
//   Buffer_Write_Byte(&func->buf, OP_SETFIELD);
//   Buffer_Write_4Bytes(&func->buf, index);
// }

/*--------------------------------------------------------------------------*/

void parse_import(ParserState *parser, char *id, char *path)
{
  Object *ob = Koala_Load_Module(path);
  if (ob == NULL) {
    error("cannot load module %s", path);
    return;
  }
  STable *stable = Module_Get_STable(ob);
  ASSERT_PTR(stable);
  Import *import = import_new(id, path, stable, Module_Name(ob));
  if (HashTable_Insert(&parser->imports, &import->hnode) < 0) {
    error("imported package '%s' is duplicated", path);
    import_free(import);
  }
}

void parse_vardecl(ParserState *parser, struct var *var)
{
  TypeDesc desc;
  int res = type_to_desc(parser, var->type, &desc);
  ASSERT(res >= 0);
  if (var->bconst)
    STable_Add_Const(&parser->u->stable, var->id, &desc);
  else
    STable_Add_Var(&parser->u->stable, var->id, &desc);
}

// void symbol_funcdecl_handler(Parser *parser, struct stmt *stmt)
// {
//   ProtoInfo proto;
//   proto.psz = parse_args(parser, stmt, &proto.pdesc);
//   proto.rsz = parse_rets(parser, stmt, &proto.rdesc);
//   Module_Add_Func(parser->module, stmt->funcdecl.id, &proto, NULL);
// }

// void parse_initfunc(Parser *parser)
// {
//   struct stmt *stmt;
//   Vector *vec = &parser->__initstmts__;
//   if (Vector_Size(vec) > 0) {
//     debug("add __init__ function to '%s' module\n",
//           Moudle_Name(parser->module));
//     init_funcdata(&parser->func, parser->module);
//   }

//   for (int i = 0; i < Vector_Size(vec); i++) {
//     stmt = Vector_Get(vec, i);
//     if (stmt->vardecl.exp == NULL) {
//       debug("variable declaration is not need generate code\n");
//       continue;
//     }
//     if (stmt->vardecl.exp->kind == NIL_KIND) {
//       debug("null value\n");
//       continue;
//     }

//     ASSERT_PTR(stmt->vardecl.exp->type);

//     if (!type_check(stmt->vardecl.var->type, stmt->vardecl.exp->type)) {
//       error("typecheck failed\n");
//     } else {
//       //generate code
//       info("generate code\n");
//       generate_initfunc_code(parser, stmt);
//     }
//   }

//   if (Vector_Size(vec) > 0) {
//     ItemTable_Show(parser->func.itable);
//   }
// }

/*--------------------------------------------------------------------------*/

// void parse_symbols(Parser *parser)
// {
//   static stmt_parser_t parsers[] = {
//     NULL, /* INVALID */
//     import_stmt_handler,
//     parse_vardecl_symbol,
//     NULL, //funcdecl_stmt_handler,
//     NULL,
//     NULL,
//   };
//   struct stmt *stmt;
//   Vector *vec = &parser->stmts;
//   for (int i = 0; i < Vector_Size(vec); i++) {
//     stmt = Vector_Get(vec, i);
//     ASSERT(stmt->kind > 0 && stmt->kind < nr_elts(parsers));
//     stmt_parser_t fn_parser = parsers[stmt->kind];
//     ASSERT_PTR(fn_parser);
//     fn_parser(parser, stmt);
//   }
// }

/*--------------------------------------------------------------------------*/
void parser_enter_scope(ParserState *parser)
{
  AtomTable *atable = NULL;
  ParserUnit *u = malloc(sizeof(ParserUnit));
  init_list_head(&u->link);
  if (parser->u != NULL) atable = parser->u->stable.atable;
  STable_Init(&u->stable, atable);
  u->up = parser->u;
  u->block = NULL;
  init_list_head(&u->blocks);

  /* Push the old ParserUnit on the stack. */
  if (parser->u != NULL) {
    list_add(&u->link, &parser->ustack);
  }

  parser->u = u;
  parser->scope++;
}

void parser_exit_scope(ParserState *parser)
{

}

/*--------------------------------------------------------------------------*/

static void parser_body(ParserState *parser, Vector *stmts)
{

}

void parse_module(ParserState *parser, struct mod *mod)
{
  debug("-----------------------");
  parser->package = mod->package;
  parser_body(parser, &mod->stmts);
  printf("package:%s\n", parser->package);
  ASSERT_PTR(parser->u);
  STable_Show(&parser->u->stable);
  debug("-----------------------");
}

static void init_parser(ParserState *parser)
{
  Koala_Init();
  memset(parser, 0, sizeof(ParserState));
  init_imports(parser);
  init_list_head(&parser->ustack);
  Vector_Init(&parser->errors);
  parser_enter_scope(parser);
}

static void fini_parser(ParserState *parser)
{
  parser_exit_scope(parser);
  Koala_Fini();
}

int main(int argc, char *argv[])
{
  extern FILE *yyin;
  int yyparse(void);

  if (argc < 2) {
    printf("error: no input files\n");
    return -1;
  }

  init_parser(&parser);

  yyin = fopen(argv[1], "r");
  yyparse();
  fclose(yyin);

  fini_parser(&parser);

  return 0;
}

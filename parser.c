
#include "parser.h"
#include "koala.h"
#include "hash.h"
#include "ast.h"

extern FILE *yyin;
extern int yyparse(ParserState *ps);
static void parse_body(ParserState *ps, Vector *stmts);
static void parser_visit_expr(ParserState *ps, struct expr *exp);

/*-------------------------------------------------------------------------*/

static void init_imports(ParserState *ps)
{
  STbl_Init(&ps->extstbl, NULL);
  Symbol *sym = parse_import(ps, "lang", "koala/lang");
  sym->refcnt++;
}

static void visit_import(HashNode *hnode, void *arg)
{
  Symbol *sym = container_of(hnode, Symbol, hnode);
  if (sym->refcnt == 0) {
    error("package '%s <- %s' is never used", sym->str, (char *)sym->ptr);
  }
}

static void check_imports(ParserState *ps)
{
  SymTable *stbl = &ps->extstbl;
  HashTable_Traverse(stbl->htbl, visit_import, NULL);
}

/*-------------------------------------------------------------------------*/

char *type_fullpath(ParserState *ps, struct type *type)
{
  Symbol *sym = STbl_Get(&ps->extstbl, type->userdef.mod);
  if (sym == NULL) return NULL;
  ASSERT(sym->kind == SYM_STABLE);
  char *pkg = sym->str;
  sym->refcnt = 1;
  sym = STbl_Get(sym->obj, type->userdef.type);
  if (sym == NULL) {
    error("cannot find type: %s.%s", pkg, type->userdef.type);
    return NULL;
  } else {
    if (sym->kind != SYM_CLASS && sym->kind != SYM_INTF) {
      error("symbol(%d) is not class or interface", sym->kind);
      return NULL;
    }
  }

  int len = strlen(pkg) + strlen(type->userdef.type) + 2;
  char *fullpath = malloc(len);
  sprintf(fullpath, "%s.%s", pkg, type->userdef.type);
  fullpath[len - 1] = '\0';
  return fullpath;
}

int type_to_desc(ParserState *ps, struct type *type, TypeDesc *desc)
{
  ASSERT_PTR(type);
  if (type->kind == PRIMITIVE_KIND) {
    INIT_PRIMITIVE_DESC(desc, type->dims, type->primitive);
  } else if (type->kind == USERDEF_KIND) {
    char *path = type_fullpath(ps, type);
    if (path == NULL) return -1;
    INIT_STRUCTED_DESC(desc, type->dims, path);
  } else {
    ASSERT(0);
  }
  return 0;
}

int type_check_desc(struct type *type, TypeDesc *desc)
{
  ASSERT_PTR(type);
  ASSERT_PTR(desc);
  return 1;
}

static int check_return_types(ParserUnit *u, Vector *vec)
{
  if (vec == NULL) {
    return (u->proto.rsz == 0) ? 1 : 0;
  } else {
    int sz = Vector_Size(vec);
    if (u->proto.rsz != sz) return 0;
    Vector_ForEach(exp, struct expr, vec) {
      if (!type_check_desc(exp->type, u->proto.rdesc + i))
        return 0;
    }
    return 1;
  }
}

/*--------------------------------------------------------------------------*/

#define proto_args(ps, vec, sz, desc) do { \
  sz = Vector_Size(vec); \
  desc = malloc(sizeof(TypeDesc) * sz); \
  ASSERT_PTR(desc); \
  Vector_ForEach(var, struct var, vec) { \
    type_to_desc(ps, var->type, desc + i); \
  } \
} while (0)

#define proto_rets(ps, vec, sz, desc) do { \
  sz = Vector_Size(vec); \
  desc = malloc(sizeof(TypeDesc) * sz); \
  ASSERT_PTR(desc); \
  Vector_ForEach(type, struct type, vec) { \
    type_to_desc(ps, type, desc + i); \
  } \
} while (0)

Symbol *parser_find_symbol(ParserState *ps, char *name)
{
  ParserUnit *u = ps->u;
  Symbol *sym = STbl_Get(&u->stbl, name);
  if (sym != NULL) {
    debug("symbol '%s' is found in current scope", name);
    sym->refcnt++;
    return sym;
  }

  if (!list_empty(&ps->ustack)) {
    list_for_each_entry(u, &ps->ustack, link) {
      sym = STbl_Get(&u->stbl, name);
      if (sym != NULL) {
        debug("symbol '%s' is found in parent scope", name);
        sym->refcnt++;
        return sym;
      }
    }
  }

  sym = STbl_Get(&ps->extstbl, name);
  if (sym != NULL) {
    debug("symbol '%s' is found in external scope", name);
    sym->refcnt++;
    return sym;
  }

  error("cannot find symbol:%s", name);
  return NULL;
}

/*--------------------------------------------------------------------------*/

// int expr_handler(Parser *ps, struct expr *exp);

// int expr_id_handler(Parser *ps, struct expr *exp)
// {
//   char *name = exp->name.id;
//   Symbol *sym = parser_find_symbol(ps, name);
//   if (sym == NULL) {
//     Import k = {.id = name};
//     Import *import = HashTable_FindObject(&ps->imports, &k, Import);
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

// int expr_attr_handler(Parser *ps, struct expr *exp)
// {
  // info("attribute\n");
  // expr_set_ctx(exp->attribute.left, CTX_LOAD);
  // expr_handler(ps, exp->attribute.left);
  // info("%s\n", exp->attribute.id);
  // //OP_GETFIELD
  // return 0;
// }

// int expr_subscribe_handler(Parser *ps, struct expr *exp)
// {
//   info("subscribe\n");
//   expr_handler(ps, exp->subscript.left);
//   expr_handler(ps, exp->subscript.index);
//   return 0;
// }

// int expr_call_handler(Parser *ps, struct expr *exp)
// {
  // info("call\n");
  // expr_handler(ps, exp->call.left);

  // Vector *vec = exp->call.pseq;
  // if (vec == NULL) {
  //   info("no args' function call\n");
  //   return 0;
  // }

  // struct expr *e;
  // int sz = Vector_Size(vec);
  // for (int i = 0; i < sz; i++) {
  //   e = Vector_Get(vec, i);
  // }
//   return 0;
// }

// static expr_handler_t expr_handlers[] = {
//   NULL,
//   expr_id_handler, NULL, NULL, NULL,
//   NULL, NULL, NULL, NULL,
//   NULL, NULL, expr_attr_handler, expr_subscribe_handler,
//   expr_call_handler, NULL, NULL, NULL,
// };

// int expr_handler(Parser *ps, struct expr *exp)
// {
//   ASSERT(exp->kind > 0 && exp->kind < EXPR_KIND_MAX);
//   //printf("expr kind:%d\n", exp->kind);
//   expr_handler_t handler = expr_handlers[exp->kind];
//   ASSERT_PTR(handler);
//   return handler(ps, exp);
// }

// int expr_stmt_handler(Parser *ps, struct stmt *stmt)
// {
//   info("expression\n");
//   struct expr *exp = stmt->expr;
//   return expr_handler(ps, exp);
// }

// int local_vardecl_stmt_handler(Parser *ps, struct stmt *stmt)
// {
//   info("local var decl\n");
//   return 0;
// }

// int expr_assign_handler(Parser *ps, struct stmt *stmt)
// {
//   info("=\n");
//   return 0;
// }

// int ret_stmt_handler(Parser *ps, struct stmt *stmt)
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

// int localstmt_handler(Parser *ps, struct stmt *stmt)
// {
//   ASSERT(stmt->kind > 0 && stmt->kind < STMT_KIND_MAX);
//   //printf("localstmt kind:%d\n", stmt->kind);
//   stmt_handler_t handler = localstmt_handlers[stmt->kind];
//   ASSERT_PTR(handler);
//   return handler(ps, stmt);
// }

void parse_dotacess(ParserState *ps, struct expr *exp)
{
  debug("dot expr");
  struct expr *left = exp->attribute.left;
  left->ctx = CTX_LOAD;
  parser_visit_expr(ps, left);

  debug(".%s", exp->attribute.id);
  if (left->sym == NULL) {
    error("undefined: <symbol-name>");
    return;
  }

  ASSERT(left->sym->kind == SYM_STABLE);
  SymTable *stbl = left->sym->obj;
  Symbol *sym = STbl_Get(stbl, exp->attribute.id);
  if (sym == NULL) {
    error("cannot find symbol '%s' in '%s'",
          exp->attribute.id, left->sym->str);
    exp->sym = NULL;
    return;
  }

  exp->sym = sym;

  //OP_GETFIELD
}

void parse_call(ParserState *ps, struct expr *exp)
{
  debug("call expr");
  struct expr *left = exp->call.left;
  left->ctx = CTX_LOAD;
  parser_visit_expr(ps, left);

  if (left->sym == NULL) {
    error("undefined: <symbol-name>");
    return;
  }

  Symbol *sym = left->sym;
  ASSERT(sym->kind == SYM_PROTO);

  /* check arguments */

  debug("call %s()", left->sym->str);
}

static void parser_visit_expr(ParserState *ps, struct expr *exp)
{
  switch (exp->kind) {
    case NAME_KIND: {
      char *load = exp->ctx == CTX_STORE ? "store": "load";
      debug("name:%s(%s), type:%s",
            exp->name.id, load, type_tostring(exp->type));
      exp->sym = parser_find_symbol(ps, exp->name.id);
      break;
    }
    case INT_KIND: {
      if (exp->ctx == CTX_STORE) {
        error("cannot assign to %lld", exp->ival);
      }
      break;
    }
    case FLOAT_KIND: {
      if (exp->ctx == CTX_STORE) {
        error("cannot assign to %f", exp->fval);
      }
      break;
    }
    case BOOL_KIND: {
      if (exp->ctx == CTX_STORE) {
        error("cannot assign to %s", exp->bval ? "true":"false");
      }
      break;
    }
    case STRING_KIND: {
      if (exp->ctx == CTX_STORE) {
        error("cannot assign to %s", exp->str);
      }
      break;
    }
    case ATTRIBUTE_KIND: {
      parse_dotacess(ps, exp);
      break;
    }
    case CALL_KIND: {
      parse_call(ps, exp);
      break;
    }
    case BINARY_KIND: {
      break;
    }
    default:
      ASSERT_MSG(0, "unknown expression type: %d", exp->kind);
      break;
  }
}

/*--------------------------------------------------------------------------*/

struct type *fill_expr_type(ParserState *ps, struct expr *exp)
{
  struct type *type = NULL;

  switch (exp->kind) {
    case NAME_KIND: {
      if (exp->type == NULL) {

      }
      char *load = exp->ctx == CTX_STORE ? "store": "load";
      debug("name:%s(%s), type:%s",
            exp->name.id, load, type_tostring(exp->type));
      break;
    }
    case INT_KIND: {
      if (exp->ctx == CTX_STORE) {
        error("cannot assign to %lld", exp->ival);
      }
      break;
    }
    case FLOAT_KIND: {
      if (exp->ctx == CTX_STORE) {
        error("cannot assign to %f", exp->fval);
      }
      break;
    }
    case BOOL_KIND: {
      if (exp->ctx == CTX_STORE) {
        error("cannot assign to %s", exp->bval ? "true":"false");
      }
      break;
    }
    case STRING_KIND: {
      if (exp->ctx == CTX_STORE) {
        error("cannot assign to %s", exp->str);
      }
      break;
    }
    case ATTRIBUTE_KIND: {
      break;
    }
    case CALL_KIND: {
      break;
    }
    case BINARY_KIND: {
      exp->type = fill_expr_type(ps, exp->bin_op.left);
      break;
    }
    default:
      ASSERT_MSG(0, "unsupported expression type: %d", exp->kind);
      break;
  }

  return type;
}

/*--------------------------------------------------------------------------*/

// int func_generate_code(Parser *ps, struct stmt *stmt)
// {
//   ASSERT(stmt->kind == FUNCDECL_KIND);
//   info("parsing func %s\n", stmt->funcdecl.id);
//   scope_enter(ps);
//   parse_args(ps, stmt, NULL);
//   parse_body(ps, stmt);
//   scope_exit(ps);
//   return 0;
// }

/*--------------------------------------------------------------------------*/

// void generate_initfunc_code(Parser *ps, struct stmt *stmt)
// {
//   expr_generate_code(&ps->func, stmt->vardecl.exp);
//   FuncData *func = &ps->func;
//   struct var *var = stmt->vardecl.var;
//   Symbol *sym = STbl_Get(Object_STable(func->owner), var->id);
//   ASSERT_PTR(sym); ASSERT(sym->name_index >= 0);
//   int index = ConstItem_Set_String(func->itable, sym->name_index);
//   Buffer_Write_Byte(&func->buf, OP_SETFIELD);
//   Buffer_Write_4Bytes(&func->buf, index);
// }

/*--------------------------------------------------------------------------*/

static void parser_enter_scope(ParserState *ps, int scope)
{
  AtomTable *atbl = NULL;
  ParserUnit *u = calloc(1, sizeof(ParserUnit));
  init_list_head(&u->link);
  if (ps->u != NULL) atbl = ps->u->stbl.atbl;
  STbl_Init(&u->stbl, atbl);
  u->scope = scope;
  init_list_head(&u->blocks);

  /* Push the old ParserUnit on the stack. */
  if (ps->u != NULL) {
    list_add(&ps->u->link, &ps->ustack);
  }

  ps->u = u;
  ps->nestlevel++;
}

static void parser_unit_free(ParserUnit *u)
{
  CodeBlock *b, *n;
  list_for_each_entry_safe(b, n, &u->blocks, link) {
    list_del(&b->link);
    free(b);
  }
  STbl_Fini(&u->stbl);
  free(u);
}

static void parser_exit_scope(ParserState *ps)
{
  STbl_Show_HTable(&ps->u->stbl);

  ps->nestlevel--;
  parser_unit_free(ps->u);
  /* Restore c->u to the parent unit. */
  struct list_head *first = list_first(&ps->ustack);
  if (first != NULL) {
    list_del(first);
    ps->u = container_of(first, ParserUnit, link);
  } else {
    ps->u = NULL;
  }
}

ParserUnit *parent_scope(ParserState *ps)
{
  ASSERT(!list_empty(&ps->ustack));
  return list_first_entry(&ps->ustack, ParserUnit, link);
}

/*--------------------------------------------------------------------------*/

void parse_variable(ParserState *ps, struct var *var, struct expr *exp)
{
  ParserUnit *u = ps->u;
  ASSERT_PTR(u);

  if (u->scope == SCOPE_MODULE || u->scope == SCOPE_CLASS) {
    if (exp == NULL) {
      debug("variable declaration is not need generate code");
      return;
    }

    if (exp->kind == NIL_KIND) {
      debug("nil value");
      return;
    }

    if (exp->kind == SELF_KIND) {
      error("cannot use keyword 'self'");
      return;
    }

    ASSERT_PTR(exp->type);

    if (!type_check(var->type, exp->type)) {
      error("typecheck failed");
    } else {
      // parse exp
      debug("parse exp");
      exp->ctx = CTX_LOAD;  /* rvalue */
      parser_visit_expr(ps, exp);

      //generate code
      debug("generate code");
    }
  } else if (u->scope == SCOPE_FUNCTION) {
    debug("parse func vardecl, '%s'", var->id);
    ASSERT(!list_empty(&ps->ustack));
    TypeDesc desc;
    int res = type_to_desc(ps, var->type, &desc);
    ASSERT(res >= 0);
    ParserUnit *parent = parent_scope(ps);
    ASSERT(parent->scope == SCOPE_MODULE || parent->scope == SCOPE_CLASS);
    STbl_Add_Var(&u->stbl, var->id, &desc, var->bconst);
  } else if (u->scope == SCOPE_BLOCK) {
    debug("parse block vardecl");
    ASSERT(!list_empty(&ps->ustack));
  } else {
    ASSERT_MSG(0, "unknown unit scope:%d", u->scope);
  }
}

void parse_function(ParserState *ps, struct stmt *stmt)
{
  parser_enter_scope(ps, SCOPE_FUNCTION);

  ParserUnit *parent = parent_scope(ps);
  Symbol *sym = STbl_Get(&parent->stbl, stmt->funcdecl.id);
  ASSERT_PTR(sym);
  ps->u->proto = sym->proto;

  if (parent->scope == SCOPE_MODULE) {
    debug("parse function, '%s'", stmt->funcdecl.id);
    Vector_ForEach(var, struct var, stmt->funcdecl.pvec) {
      parse_variable(ps, var, NULL);
    }
    parse_body(ps, stmt->funcdecl.body);
  } else if (parent->scope == SCOPE_CLASS) {
    debug("parse method, '%s'", stmt->funcdecl.id);
  } else {
    ASSERT_MSG(0, "unknown parent scope type:%d", parent->scope);
  }

  parser_exit_scope(ps);
}

void parse_assign(ParserState *ps, struct stmt *stmt)
{
  struct expr *r = stmt->assign.right;
  struct expr *l = stmt->assign.left;
  r->ctx = CTX_LOAD;
  parser_visit_expr(ps, r);
  l->ctx = CTX_STORE;
  parser_visit_expr(ps, l);
}

void paser_return(ParserState *ps, struct stmt *stmt)
{
  ParserUnit *u = ps->u;
  ASSERT_PTR(u);
  if (u->scope == SCOPE_FUNCTION) {
    debug("return in function");
    check_return_types(u, stmt->vec);
  } else if (u->scope == SCOPE_BLOCK) {
    debug("return in some block");
    check_return_types(u, stmt->vec);
  } else {
    ASSERT_MSG(0, "invalid scope:%d", u->scope);
  }

}

void parser_visit_stmt(ParserState *ps, struct stmt *stmt)
{
  switch (stmt->kind) {
    case VARDECL_KIND: {
      parse_variable(ps, stmt->vardecl.var, stmt->vardecl.exp);
      break;
    }
    case FUNCDECL_KIND:
      parse_function(ps, stmt);
      break;
    case CLASS_KIND:
      break;
    case INTF_KIND:
      break;
    case EXPR_KIND: {
      parser_visit_expr(ps, stmt->exp);
      break;
    }
    case ASSIGN_KIND: {
      parse_assign(ps, stmt);
      break;
    }
    case RETURN_KIND: {
      paser_return(ps, stmt);
      break;
    }
    case VARDECL_LIST_KIND: {
      parse_body(ps, stmt->vec);
      break;
    }
    default:
      ASSERT_MSG(0, "unknown statement type: %d", stmt->kind);
      break;
  }
}

/*--------------------------------------------------------------------------*/

/* parse a sequence of statements */
static void parse_body(ParserState *ps, Vector *stmts)
{
  if (stmts == NULL) return;
  Vector_ForEach(stmt, struct stmt, stmts) {
    parser_visit_stmt(ps, stmt);
  }
}

static void init_parser(ParserState *ps)
{
  Koala_Init();
  memset(ps, 0, sizeof(ParserState));
  init_imports(ps);
  init_list_head(&ps->ustack);
  Vector_Init(&ps->errors);
  parser_enter_scope(ps, SCOPE_MODULE);
}

static void fini_parser(ParserState *ps)
{
  parser_exit_scope(ps);
  Koala_Fini();
}

int main(int argc, char *argv[])
{
  ParserState ps;

  if (argc < 2) {
    printf("error: no input files\n");
    return -1;
  }

  init_parser(&ps);

  yyin = fopen(argv[1], "r");
  yyparse(&ps);
  fclose(yyin);

  fini_parser(&ps);

  return 0;
}

/*--------------------------------------------------------------------------*/

static Symbol *add_import(SymTable *stbl, char *id, char *path)
{
  Symbol *sym = STbl_Add_Symbol(stbl, id, SYM_STABLE, 0);
  if (sym == NULL) return NULL;
  idx_t idx = StringItem_Set(stbl->atbl, path);
  ASSERT(idx >= 0);
  sym->desc = idx;
  sym->ptr = path;
  return sym;
}

Symbol *parse_import(ParserState *ps, char *id, char *path)
{
  Object *ob = Koala_Load_Module(path);
  if (ob == NULL) {
    error("load module '%s' failed", path);
    return NULL;
  }
  if (id == NULL) id = Module_Name(ob);
  Symbol *sym = add_import(&ps->extstbl, id, path);
  if (sym == NULL) {
    debug("add import '%s <- %s' failed", id, path);
    return NULL;
  }
  debug("add import '%s <- %s' successful", id, path);
  sym->obj = Module_Get_STable(ob);
  return sym;
}

void parse_vardecl(ParserState *ps, struct stmt *s)
{
  ASSERT(s->kind == VARDECL_LIST_KIND);
  struct var *var;
  TypeDesc desc;
  int res;
  Symbol *sym;

  Vector_ForEach(stmt, struct stmt, s->vec) {
    var = stmt->vardecl.var;
    res = type_to_desc(ps, var->type, &desc);
    ASSERT(res >= 0);
    sym = STbl_Add_Var(&ps->u->stbl, var->id, &desc, var->bconst);
    if (sym != NULL) {
      debug("add '%s %s' successful", var->bconst ? "const":"var", var->id);
    } else {
      debug("add '%s %s' failed", var->bconst ? "const":"var", var->id);
    }
  }
}

void parse_funcdecl(ParserState *ps, struct stmt *stmt)
{
  ProtoInfo proto = {0};
  int sz;
  TypeDesc *desc = NULL;
  Symbol *sym;

  if (stmt->funcdecl.pvec != NULL) {
    proto_args(ps, stmt->funcdecl.pvec, sz, desc);
    proto.psz = sz; proto.pdesc = desc;
  }

  if (stmt->funcdecl.rvec != NULL) {
    proto_rets(ps, stmt->funcdecl.rvec, sz, desc);
    proto.rsz = sz; proto.rdesc = desc;
  }

  sym = STbl_Add_Proto(&ps->u->stbl, stmt->funcdecl.id, &proto);
  if (sym != NULL) {
    debug("add 'func %s' successful", stmt->funcdecl.id);
  } else {
    debug("add 'func %s' failed", stmt->funcdecl.id);
  }
}

void parse_typedecl(ParserState *ps, struct stmt *stmt)
{

}

void parse_module(ParserState *ps, struct mod *mod)
{
  debug("==begin==================");
  ps->package = mod->package;
  parse_body(ps, &mod->stmts);
  debug("==end===================");
  printf("package:%s\n", ps->package);
  STbl_Show(&ps->u->stbl);
  check_imports(ps);
}

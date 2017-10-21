
#include "compile.h"
#include "module_object.h"
#include "string_object.h"
#include "method_object.h"

void init_compiler(struct compiler *c)
{
  c->symtable = new_symtable();
}

static int compiler_import(struct compiler *cp, struct stmt *stmt)
{
  struct symbol *sym = new_symbol(stmt->v.import.alias, MODREF_KIND);
  if (sym != NULL) {
    sym->v.mod_path = stmt->v.import.path;
    return symtable_add(cp->symtable, sym);
  } else {
    return -1;
  }
}

static int compiler_expr(struct compiler *cp, struct expr *expr);

static void call(struct object *ob, struct object *arg)
{
  struct method_object *mo = (struct method_object *)ob;
  cfunc_t fn = *((cfunc_t *)(mo + 1));
  fn(NULL, arg);
}

static int compiler_atom(struct compiler *cp, struct atom *atom)
{
  switch (atom->kind) {
    case NAME_KIND: {
      struct symbol *sym = symtable_find(cp->symtable, atom->v.name.id);
      if (sym == NULL) {
        fprintf(stderr, "[ERROR] cannot find symbol:'%s'\n", atom->v.name.id);
        return -1;
      }

      if (atom->v.name.type == 0) {
        if (sym->kind == MODREF_KIND) {
          cp->obj = find_module(sym->v.mod_path);
        } else if (sym->kind != VAR_KIND) {
          //cp->obj = moduel_get();
        } else {
          fprintf(stderr,
                  "[TypeCheckError] symbol '%s' is not variable or module\n",
                  sym->name);
          return -1;
        }

      } else if (atom->v.name.type == NT_FUNC) {
        if (sym->kind != FUNC_KIND) {
          fprintf(stderr, "[TypeCheckError] symbol '%s' is not func\n",
                  sym->name);
          return -1;
        }
      } else {
        assert(0);
      }
      break;
    }
    case INT_KIND: {
      printf("[integer]\n");
      printf("%lld\n", atom->v.ival);
      break;
    }
    case FLOAT_KIND: {
      printf("[float]\n");
      printf("%f\n", atom->v.fval);
      break;
    }
    case STRING_KIND: {
      cp->arg = string_from_cstr(atom->v.str);
      break;
    }
    case BOOL_KIND: {
      printf("[boolean]\n");
      printf("%s\n", atom->v.bval ? "true" : "false");
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
      compiler_expr(cp, atom->v.exp);
      break;
    }
    case NEW_PRIMITIVE_KIND: {
      printf("[new primitive object]\n");
      break;
    }
    case ATTRIBUTE_KIND: {
      int res = compiler_atom(cp, atom->v.attribute.atom);
      if (res) return res;

      if (OB_KLASS(cp->obj) == &module_klass) {
        struct object *ob = NULL;
        int offset = 0;
        struct namei ni;
        ni.name = atom->v.attribute.id;
        ni.type = atom->v.attribute.type;
        module_get(cp->obj, &ni, &ob, &offset);
        if (ob == NULL) {
          fprintf(stderr, "[ERROR] cannot find %s:'%s'\n",
                  ni_type_string(ni.type), atom->v.attribute.id);
          return -1;
        }

        cp->obj = ob;
      } else {
        /* find symbol from class */
        struct object *ob = NULL;
        int offset = 0;
        struct namei ni;
        ni.name = atom->v.attribute.id;
        ni.type = atom->v.attribute.type;
        klass_get(cp->obj, &ni, &ob, &offset);
        if (ob == NULL) {
          fprintf(stderr, "[ERROR] cannot find %s:'%s'\n",
                  ni_type_string(ni.type), atom->v.attribute.id);
          return -1;
        }

        cp->obj = ob;
      }
      break;
    }
    case CALL_KIND: {
      int res = compiler_atom(cp, atom->v.call.atom);
      if (res) return res;

      struct expr *expr;
      clist_foreach(expr, atom->v.call.list) {
        compiler_expr(cp, expr);
      }
      assert(OB_KLASS(cp->obj) == &method_klass);
      call(cp->obj, cp->arg);
      break;
    }
    default: {
      assert(0);
      break;
    }
  }

  return 0;
}

static int compiler_expr(struct compiler *cp, struct expr *expr)
{
  int res = -1;
  switch (expr->kind) {
    case ATOM_KIND: {
      res = compiler_atom(cp, expr->v.atom);
      break;
    }
    default: {
      assert(0);
      break;
    }
  }

  return res;
}

void stmt_call_test(struct compiler *cp)
{
  if (OB_KLASS(cp->obj) == &method_klass) {

  }
}

static int compiler_stmt(struct compiler *cp, struct stmt *stmt)
{
  int res = -1;
  switch (stmt->kind) {
    case IMPORT_KIND: {
      res = compiler_import(cp, stmt);
      break;
    }
    case EXPR_KIND: {
      res = compiler_expr(cp, stmt->v.expr);
      break;
    }
    default:{
      assert(0);
    }
  }

  stmt_call_test(cp);
  return res;
}

int compiler_module(struct mod *mod)
{
  struct compiler c;
  init_compiler(&c);

  symtable_begin_scope(c.symtable);

  /**
   * handle imports
   */
  struct stmt *stmt;
  clist_foreach(stmt, mod->imports) {
    compiler_stmt(&c, stmt);
  }

  /**
   * handle statements
   */
  clist_foreach(stmt, mod->stmts) {
    compiler_stmt(&c, stmt);
  }

  symtable_end_scope(c.symtable);

  return 0;
}

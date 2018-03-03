
#include "parser.h"
#include "koala_state.h"
#include "checker.h"
#include "codegen.h"
#include "opcode.h"
#include "moduleobject.h"
#include "log.h"

void visit_expr(ParserState *ps, struct expr *exp);
void parse_statements(ParserState *ps, Vector *stmts);

/*-------------------------------------------------------------------------*/
// Symbol

static Symbol *find_id_symbol(ParserState *ps, char *id)
{
	ParserUnit *u = ps->u;
	Symbol *sym = STbl_Get(&u->stbl, id);
	if (sym) {
		debug("symbol '%s' is found in current scope", id);
		sym->refcnt++;
		return sym;
	}

	if (!list_empty(&ps->ustack)) {
		list_for_each_entry(u, &ps->ustack, link) {
			sym = STbl_Get(&u->stbl, id);
			if (sym) {
				debug("symbol '%s' is found in parent scope", id);
				sym->refcnt++;
				return sym;
			}
		}
	}

	sym = STbl_Get(&ps->extstbl, id);
	if (sym) {
		debug("symbol '%s' is found in external scope", id);
		assert(sym->kind == SYM_STABLE);
		sym->refcnt++;
		return sym;
	}

	error("cannot find symbol:%s", id);
	return NULL;
}

static Symbol *find_userdef_symbol(ParserState *ps, TypeDesc *desc)
{
	if (desc->kind != TYPE_USERDEF) {
		error("type(%s) is not class or interface", TypeDesc_ToString(desc));
		return NULL;
	}

	// find in current module

	// find in external imported module
	Import key = {.path = desc->path};
	Import *import = HTable_FindObject(&ps->imports, &key, Import);
	if (!import) {
		error("cannot find '%s.%s'", desc->path, desc->type);
		return NULL;
	}

	Symbol *sym = import->sym;
	assert(sym->kind == SYM_STABLE);
	sym = STbl_Get(sym->ptr, desc->type);
	if (sym) {
		debug("find '%s.%s'", desc->path, desc->type);
		sym->refcnt++;
		return sym;
	}

	error("cannot find '%s.%s'", desc->path, desc->type);
	return NULL;
}

/*-------------------------------------------------------------------------*/
// External imported modules management

static Import *import_new(char *path)
{
	Import *import = malloc(sizeof(Import));
	import->path = strdup(path);
	Init_HashNode(&import->hnode, import);
	return import;
}

static void import_free(Import *import)
{
	free(import);
}

static uint32 import_hash(void *k)
{
	Import *import = k;
	return hash_string(import->path);
}

static int import_equal(void *k1, void *k2)
{
	Import *import1 = k1;
	Import *import2 = k2;
	return !strcmp(import1->path, import2->path);
}

static void init_imports(ParserState *ps)
{
	HashInfo hashinfo;
	Init_HashInfo(&hashinfo, import_hash, import_equal);
	HTable_Init(&ps->imports, &hashinfo);
	STbl_Init(&ps->extstbl, NULL);
	Symbol *sym = Parse_Import(ps, "lang", "koala/lang");
	sym->refcnt++;
}

static void __import_free_fn(HashNode *hnode, void *arg)
{
	UNUSED_PARAMETER(arg);
	Import *import = container_of(hnode, Import, hnode);
	//free(import->path);
	import_free(import);
}

static void fini_imports(ParserState *ps)
{
	HTable_Fini(&ps->imports, __import_free_fn, NULL);
	STbl_Fini(&ps->extstbl);
}

/*-------------------------------------------------------------------------*/

static ParserUnit *parent_scope(ParserState *ps)
{
	if (list_empty(&ps->ustack)) return NULL;
	return list_first_entry(&ps->ustack, ParserUnit, link);
}

/*--------------------------------------------------------------------------*/
// API used by yacc

static Symbol *add_import(STable *stbl, char *id, char *path)
{
	Symbol *sym = STbl_Add_Symbol(stbl, id, SYM_STABLE, 0);
	if (!sym) return NULL;
	sym->path = strdup(path);
	return sym;
}

Symbol *Parse_Import(ParserState *ps, char *id, char *path)
{
	Import key = {.path = path};
	Import *import = HTable_FindObject(&ps->imports, &key, Import);
	Symbol *sym;
	if (import) {
		sym = import->sym;
		if (sym && sym->refcnt > 0) {
			warn("find auto imported module '%s'", path);
			if (id) {
				if (strcmp(id, sym->name)) {
					warn("imported as '%s' is different with auto imported as '%s'",
								id, sym->name);
				} else {
					warn("imported as '%s' is the same with auto imported as '%s'",
								id, sym->name);
				}
			}
			return sym;
		}
	}

	import = import_new(path);
	if (HTable_Insert(&ps->imports, &import->hnode) < 0) {
		error("module '%s' is imported duplicated", path);
		return NULL;
	}
	Object *ob = Koala_Load_Module(path);
	if (!ob) {
		error("load module '%s' failed", path);
		HTable_Remove(&ps->imports, &import->hnode);
		import_free(import);
		return NULL;
	}
	if (!id) id = Module_Name(ob);
	sym = add_import(&ps->extstbl, id, path);
	if (!sym) {
		debug("add import '%s <- %s' failed", id, path);
		HTable_Remove(&ps->imports, &import->hnode);
		import_free(import);
		return NULL;
	}

	sym->ptr = Module_To_STable(ob, ps->extstbl.atbl);
	import->sym = sym;
	debug("add import '%s <- %s' successful", id, path);
	return sym;
}

char *Import_Get_Path(ParserState *ps, char *id)
{
	Symbol *sym = STbl_Get(&ps->extstbl, id);
	if (!sym) {
		error("cannot find module:%s", id);
		return NULL;
	}
	assert(sym->kind == SYM_STABLE);
	sym->refcnt = 1;
	return sym->desc->path;
}

static inline void __add_stmt(ParserState *ps, struct stmt *stmt)
{
	Vector_Append(&(ps)->stmts, stmt);
}

static void parse_vardecl(ParserState *ps, struct stmt *stmt)
{
	__add_stmt(ps, stmt);

	struct var *var = stmt->vardecl.var;
	if (!var->desc) {
		debug("'%s %s' type is not set", var->bconst ? "const" : "var", var->id);
	} else {
		TypeDesc *desc = var->desc;
		if (desc->kind == TYPE_USERDEF) {
			if (!find_userdef_symbol(ps, desc)) {
				char *typestr = TypeDesc_ToString(desc);
				warn("cannot find type:'%s'", typestr);
				free(typestr);
			}
		} else if (desc->kind == TYPE_PROTO) {
			debug("var's type is proto");
		} else {
			assert(desc->kind == TYPE_PRIMITIVE);
		}
	}

	Symbol *sym = STbl_Add_Var(&ps->u->stbl, var->id, var->desc, var->bconst);
	if (sym) {
		debug("add %s '%s' successful", var->bconst ? "const":"var", var->id);
		sym->up = ps->u->sym;
	} else {
		error("add %s '%s' failed, it'name is duplicated",
					var->bconst ? "const":"var", var->id);
	}

	if (!stmt->vardecl.exp) {
		debug("There is not a initial expr for var '%s'", var->id);
	}
}

void Parse_VarDecls(ParserState *ps, struct stmt *stmt)
{
	if (stmt->kind == VARDECL_LIST_KIND) {
		struct stmt *s;
		Vector_ForEach(s, stmt->vec) {
			parse_vardecl(ps, s);
		}
	} else {
		assert(stmt->kind == VARDECL_KIND);
		parse_vardecl(ps, stmt);
	}
}

static int var_vec_to_arr(Vector *vec, TypeDesc **arr)
{
	int sz = 0;
	TypeDesc *desc = NULL;
	if (vec && Vector_Size(vec) != 0) {
		sz = Vector_Size(vec);
		desc = malloc(sizeof(TypeDesc) * sz);
		struct var *var;
		Vector_ForEach(var, vec)
			memcpy(desc + i, var->desc, sizeof(TypeDesc));
	}

	*arr = desc;
	return sz;
}

static Proto *funcdecl_to_proto(struct stmt *stmt)
{
	Proto *proto = malloc(sizeof(Proto));
	int sz;
	TypeDesc *desc;

	sz = var_vec_to_arr(stmt->funcdecl.pvec, &desc);
	proto->psz = sz;
	proto->pdesc = desc;

	sz = TypeVec_ToTypeArray(stmt->funcdecl.rvec, &desc);
	proto->rsz = sz;
	proto->rdesc = desc;

	return proto;
}

void Parse_Proto(ParserState *ps, struct stmt *stmt)
{
	__add_stmt(ps, stmt);

	Proto *proto;
	Symbol *sym;
	proto = funcdecl_to_proto(stmt);
	sym = STbl_Add_Proto(&ps->u->stbl, stmt->funcdecl.id, proto);
	if (sym) {
		debug("add func '%s' successful", stmt->funcdecl.id);
		sym->up = ps->u->sym;
	} else {
		debug("add func '%s' failed", stmt->funcdecl.id);
	}
}

void Parse_UserDef(ParserState *ps, struct stmt *stmt)
{
	UNUSED_PARAMETER(ps);
	UNUSED_PARAMETER(stmt);
}

/*-------------------------------------------------------------------------*/

static int check_return_types(ParserUnit *u, Vector *vec)
{
	Proto *proto = u->sym->desc->proto;;
	if (!vec) {
		return (proto->rsz == 0) ? 1 : 0;
	} else {
		int sz = Vector_Size(vec);
		if (proto->rsz != sz) return 0;
		struct expr *exp;
		Vector_ForEach(exp, vec) {
			if (!TypeDesc_Check(exp->desc, proto->rdesc + i)) {
				error("type check failed");
				return 0;
			}
		}
		return 1;
	}
}

/*--------------------------------------------------------------------------*/

#if 0
static struct expr *optimize_binary_add(struct expr *l, struct expr *r)
{
	struct expr *e;
	if (l->kind == INT_KIND) {
		int64 val;
		if (r->kind == INT_KIND) {
			val = l->ival + r->ival;
		} else if (r->kind == FLOAT_KIND) {
			val = l->ival + r->fval;
		} else {
			assert(0);
		}
		e = expr_from_int(val);
	} else if (l->kind == FLOAT_KIND) {
		float64 val;
		if (r->kind == INT_KIND) {
			val = l->fval + r->ival;
		} else if (r->kind == FLOAT_KIND) {
			val = l->fval + r->fval;
		} else {
			assert(0);
		}
		e = expr_from_float(val);
	} else {
		assertm(0, "unsupported optimized type:%d", l->kind);
	}
	return e;
}

static struct expr *optimize_binary_sub(struct expr *l, struct expr *r)
{
	struct expr *e;
	if (l->kind == INT_KIND) {
		int64 val;
		if (r->kind == INT_KIND) {
			val = l->ival - r->ival;
		} else if (r->kind == FLOAT_KIND) {
			val = l->ival - r->fval;
		} else {
			assert(0);
		}
		e = expr_from_int(val);
	} else if (l->kind == FLOAT_KIND) {
		float64 val;
		if (r->kind == INT_KIND) {
			val = l->fval - r->ival;
		} else if (r->kind == FLOAT_KIND) {
			val = l->fval - r->fval;
		} else {
			assert(0);
		}
		e = expr_from_float(val);
	} else {
		assertm(0, "unsupported optimized type:%d", l->kind);
	}
	return e;
}

static int optimize_binary_expr(ParserState *ps, struct expr **exp)
{
	if (ps->olevel <= 0) return 0;

	int ret = 0;
	struct expr *origin = *exp;
	struct expr *left = origin->binary.left;
	struct expr *right = origin->binary.right;
	if (left->bconst && right->bconst) {
		ret = 1;
		struct expr *e = NULL;
		switch (origin->binary.op) {
			case BINARY_ADD: {
				debug("optimize add");
				e = optimize_binary_add(left, right);
				break;
			}
			case BINARY_SUB: {
				debug("optimize sub");
				e = optimize_binary_sub(left, right);
				break;
			}
			default: {
				assert(0);
			}
		}
		//free origin, left and right expression
		*exp = e;
	}

	return ret;
}

#endif

/*--------------------------------------------------------------------------*/

static void parser_finish_unit(ParserUnit *u)
{
	// save code to symbol
	if (u->scope == SCOPE_FUNCTION) {
		debug("save code to function '%s'", u->sym->name);
		u->sym->ptr = u->block;
		u->block = NULL;
		u->sym->locvars = u->stbl.varcnt;
	} else if (u->scope == SCOPE_BLOCK) {
		debug("merge code to parent's block");
	} else if (u->scope == SCOPE_MODULE) {
		debug("save code to module __init__ function");
		if (u->block) {
			Symbol *sym = STbl_Get(&u->stbl, "__init__");
			if (!sym) {
				Proto *proto = Proto_New(0, NULL, 0, NULL);
				sym = STbl_Add_Proto(&u->stbl, "__init__", proto);
				assert(sym);
			}

			sym->ptr = u->block;
			sym->locvars = u->stbl.varcnt;
			u->sym = sym;
			assert(list_empty(&u->blocks));
			u->block = NULL;
		}
	} else {
		assertm(0, "no codes in scope:%d", u->scope);
	}
}

static void init_parser_unit(ParserUnit *u, AtomTable *atbl, int scope)
{
	init_list_head(&u->link);
	init_list_head(&u->blocks);
	STbl_Init(&u->stbl, atbl);
	u->sym = NULL;
	u->block = NULL;
	u->scope = scope;
}

static void fini_parser_unit(ParserUnit *u)
{
	parser_finish_unit(u);
	// free symbol table
	STbl_Fini(&u->stbl);
}

static ParserUnit *parser_enter_unit(AtomTable *atbl, int scope)
{
	ParserUnit *u = calloc(1, sizeof(ParserUnit));
	init_parser_unit(u, atbl, scope);
	return u;
}

static void parser_exit_unit(ParserUnit *u)
{
	fini_parser_unit(u);
	CodeBlock_Free(u->block);
	free(u);
}

static void parser_enter_scope(ParserState *ps, int scope)
{
	AtomTable *atbl = NULL;
	if (ps->u) atbl = ps->u->stbl.atbl;
	ParserUnit *u = parser_enter_unit(atbl, scope);

	/* Push the old ParserUnit on the stack. */
	if (ps->u) {
		list_add(&ps->u->link, &ps->ustack);
	}
	ps->u = u;
	ps->nestlevel++;
}

static void parser_exit_scope(ParserState *ps)
{
	printf("-------------------------\n");
	printf("scope-%d symbols:\n", ps->nestlevel);
	STbl_Show(&ps->u->stbl, 0);
	CodeBlock_Show(ps->u->block);
	printf("-------------------------\n");
	check_unused_symbols(ps);

	parser_exit_unit(ps->u);
	ps->nestlevel--;

	/* Restore c->u to the parent unit. */
	struct list_head *first = list_first(&ps->ustack);
	if (first) {
		list_del(first);
		ps->u = container_of(first, ParserUnit, link);
	} else {
		ps->u = NULL;
	}
}

static void parser_enter_codeblock(ParserUnit *u)
{
	CodeBlock *block = CodeBlock_New(u->stbl.atbl);
	if (u->block) list_add(&u->block->link, &u->blocks);
	u->block = block;
}

static void parser_exit_codeblock(ParserUnit *u)
{
	CodeBlock *b = u->block;

	if (list_empty(&u->blocks)) {
		debug("no up code block");
		return;
	}

	debug("merge code block up");

	CodeBlock *nxt = list_first_entry(&u->blocks, CodeBlock, link);
	list_del(&nxt->link);
	u->block = nxt;

	Inst *i, *n;
	list_for_each_entry_safe(i, n, &b->insts, link) {
		list_del(&i->link);
		list_add_tail(&i->link, &u->block->insts);
	}

	CodeBlock_Free(b);
}

/*--------------------------------------------------------------------------*/

static void expr_parse_id(ParserState *ps, struct expr *exp)
{
	Symbol *sym = find_id_symbol(ps, exp->id);
	if (!sym) {
		error("cannot find ident:%s", exp->id);
		return;
	}

	exp->sym = sym;
	if (!exp->desc) {
		char *typestr = TypeDesc_ToString(sym->desc);
		debug("ident '%s' is as '%s'", exp->id, typestr);
		free(typestr);
		exp->desc = sym->desc;
	}

	// generate code
	ParserUnit *u = ps->u;

	if (sym->kind == SYM_STABLE) {
		// reference a module, it's not check id's scope
		debug("symbol '%s' is module", sym->name);
		TValue val = CSTR_VALUE_INIT(sym->path);
		Inst_Append(u->block, OP_LOADM, &val);
		return;
	}

	switch (u->scope) {
		case SCOPE_MODULE:
		case SCOPE_CLASS: {
			assert(exp->ctx == EXPR_LOAD);
			if (sym->kind == SYM_VAR) {
				debug("symbol '%s' is variable", sym->name);
				TValue val = INT_VALUE_INIT(0);
				Inst_Append(u->block, OP_LOAD, &val);
				setcstrvalue(&val, sym->name);
				Inst_Append(u->block, OP_GETFIELD, &val);
			} else if (sym->kind == SYM_PROTO) {
				debug("symbol '%s' is function", sym->name);
				// self object
				TValue val = INT_VALUE_INIT(0);
				Inst_Append(u->block, OP_LOAD, &val);
				setcstrvalue(&val, sym->name);
				Inst_Append(u->block, OP_CALL, &val);
			} else {
				assert(0);
			}
			break;
		}
		case SCOPE_FUNCTION:
		case SCOPE_METHOD: {
			if (sym->kind == SYM_VAR) {
				debug("symbol '%s' is variable", sym->name);
				int opcode;
				if (sym->up == u->sym) {
					// local's variable
					opcode = (exp->ctx == EXPR_LOAD) ? OP_LOAD : OP_STORE;
					TValue val = INT_VALUE_INIT(sym->index);
					Inst_Append(u->block, opcode, &val);
				} else {
					// up's variable(module or class)
					TValue val = INT_VALUE_INIT(0);
					Inst_Append(u->block, OP_LOAD, &val);

					opcode = (exp->ctx == EXPR_LOAD) ? OP_GETFIELD : OP_SETFIELD;
					setcstrvalue(&val, sym->name);
					Inst_Append(u->block, opcode, &val);
				}
			} else if (sym->kind == SYM_PROTO) {
				debug("symbol '%s' is function", sym->name);
				// self object
				TValue val = INT_VALUE_INIT(0);
				Inst_Append(u->block, OP_LOAD, &val);
				setcstrvalue(&val, sym->name);
				Inst_Append(u->block, OP_CALL, &val);
			} else {
				assert(0);
			}
			break;
		}
		case SCOPE_CLOSURE: {
			break;
		}
		case SCOPE_BLOCK: {
			break;
		}
		default: {
			assert(0);
			break;
		}
	}
}

static void expr_parse_int(ParserState *ps, struct expr *exp)
{
	if (exp->ctx == EXPR_STORE) {
		error("cannot assign to %lld", exp->ival);
		return;
	}
	TValue val = INT_VALUE_INIT(exp->ival);
	Inst_Append(ps->u->block, OP_LOADK, &val);
}

static void expr_parse_float(ParserState *ps, struct expr *exp)
{
	UNUSED_PARAMETER(ps);
	if (exp->ctx == EXPR_STORE) {
		error("cannot assign to %f", exp->fval);
	}
}

static void expr_parse_bool(ParserState *ps, struct expr *exp)
{
	UNUSED_PARAMETER(ps);
	if (exp->ctx == EXPR_STORE) {
		error("cannot assign to %s", exp->bval ? "true":"false");
	}
}

static void expr_parse_string(ParserState *ps, struct expr *exp)
{
	if (exp->ctx == EXPR_STORE) {
		error("cannot assign to %s", exp->str);
		return;
	}
	TValue val = CSTR_VALUE_INIT(exp->str);
	Inst_Append(ps->u->block, OP_LOADK, &val);
}

void expr_parse_attribute(ParserState *ps, struct expr *exp)
{
	struct expr *left = exp->attribute.left;
	left->ctx = EXPR_LOAD;
	visit_expr(ps, left);

	debug(".%s", exp->attribute.id);

	Symbol *leftsym = left->sym;
	if (!leftsym) {
		//FIXME:
		error("cannot find '%s' in '%s'", exp->attribute.id, left->str);
		return;
	}

	Symbol *sym = NULL;
	if (leftsym->kind == SYM_STABLE) {
		debug("symbol '%s' is a module", leftsym->name);
		sym = STbl_Get(leftsym->ptr, exp->attribute.id);
		if (!sym) {
			//FIXME:
			error("cannot find '%s' in '%s'", exp->attribute.id, left->str);
			exp->sym = NULL;
			return;
		}
	} else if (leftsym->kind == SYM_VAR) {
		debug("symbol '%s' is a variable", leftsym->name);
		assert(leftsym->desc);
		sym = find_userdef_symbol(ps, leftsym->desc);
		if (!sym) {
			char *typestr = TypeDesc_ToString(leftsym->desc);
			error("cannot find '%s' in '%s'", exp->attribute.id, typestr);
			return;
		}
		assert(sym->kind == SYM_STABLE);
		char *typename = sym->name;
		sym = STbl_Get(sym->ptr, exp->attribute.id);
		if (!sym) {
			error("cannot find '%s' in '%s'", exp->attribute.id, typename);
			return;
		}
	} else {
		assert(0);
	}

	// set expression's symbol
	exp->sym = sym;

	// generate code
	ParserUnit *u = ps->u;
	//parser_enter_codeblock(u);
	if (sym->kind == SYM_VAR) {
		assert(0);
	} else if (sym->kind == SYM_PROTO) {
		TValue val = CSTR_VALUE_INIT(exp->attribute.id);
		Inst_Append(u->block, OP_CALL, &val);
	} else {
		assert(0);
	}
	//parser_exit_codeblock(u);
}

static void expr_parse_call(ParserState *ps, struct expr *exp)
{
	if (exp->call.args) {
		struct expr *e;
		Vector_ForEach_Reverse(e, exp->call.args) {
			e->ctx = EXPR_LOAD;
			visit_expr(ps, e);
		}
	}

	struct expr *left = exp->call.left;
	left->ctx = EXPR_LOAD;
	visit_expr(ps, left);
	if (!left->sym) {
		error("func is not found");
		return;
	}

	Symbol *sym = left->sym;
	assert(sym && sym->kind == SYM_PROTO);
	debug("call %s()", sym->name);

	/* function type */
	exp->desc = sym->desc;

	/* check arguments */
	if (!check_call_args(exp->desc->proto, exp->call.args)) {
		error("arguments are not matched.");
	}
}

static void expr_parse_binary(ParserState *ps, struct expr *exp)
{
	debug("binary_op:%d", exp->binary.op);
	exp->binary.right->ctx = EXPR_LOAD;
	visit_expr(ps, exp->binary.right);
	exp->binary.left->ctx = EXPR_LOAD;
	visit_expr(ps, exp->binary.left);
	exp->desc = exp->binary.left->desc;

	// generate code
	if (exp->binary.op == BINARY_ADD) {
		debug("add 'OP_ADD'");
		Inst_Append(ps->u->block, OP_ADD, NULL);
	} else if (exp->binary.op == BINARY_SUB) {
		debug("add 'OP_SUB'");
		Inst_Append(ps->u->block, OP_SUB, NULL);
	}
	// if (optimize_binary_expr(ps, &exp)) {
	//   exp->gencode = 1;
	//   parser_visit_expr(ps, exp);
	// } else {
	//   exp->binary.left->gencode = 1;
	//   exp->binary.right->gencode = 1;
	//   parser_visit_expr(ps, exp->binary.right);
	//   parser_visit_expr(ps, exp->binary.left);
	//   // generate code
	//   debug("add 'OP_ADD' inst");
	//   Inst_Append(ps->u->block, OP_ADD, NULL);
	// }
}

typedef void (*visit_expr_fn)(ParserState *ps, struct expr *exp);

static visit_expr_fn exprfuncs[] = {
	NULL,  //INVALID
	expr_parse_id,     //ID_KIND
	expr_parse_int,    //INT_KIND
	expr_parse_float,  //FLOAT_KIND
	expr_parse_bool,   //BOOL_KIND
	expr_parse_string, //STRING_KIND
	NULL,  //SELF_KIND
	NULL,  //NIL_KIND
	NULL,  //EXP_KIND
	NULL,  //ARRAY_KIND
	NULL,  //ANONYOUS_FUNC_KIND
	expr_parse_attribute,  //ATTRIBUTE_KIND
	NULL,  //SUBSCRIPT_KIND
	expr_parse_call,  //CALL_KIND
	NULL,  //UNARY_KIND
	expr_parse_binary,  //BINARY_KIND
};

void visit_expr(ParserState *ps, struct expr *exp)
{
	int kind = exp->kind;
	assert(kind > 0 && kind < EXPR_KIND_MAX);
	visit_expr_fn fn = exprfuncs[kind];
	assert(fn);
	//parser_enter_codeblock(ps->u);
	fn(ps, exp);
	//parser_exit_codeblock(ps->u);
}

/*--------------------------------------------------------------------------*/

static void parse_variable(ParserState *ps, struct var *var, struct expr *exp)
{
	ParserUnit *u = ps->u;

	debug("parse variable '%s' declaration", var->id);

	if (exp) {
		visit_expr(ps, exp);
		if (!exp->desc) {
			error("cannot resolve var '%s' type", var->id);
			return;
		}

		if (!var->desc) {
			var->desc = exp->desc;
		}

		if (!TypeDesc_Check(var->desc, exp->desc)) {
			error("typecheck failed");
			return;
		}
	}

	Symbol *sym;
	if (u->scope == SCOPE_MODULE || u->scope == SCOPE_CLASS) {
		debug("var '%s' decl in module or class", var->id);
		sym = STbl_Get(&u->stbl, var->id);
		assert(sym);
		if (sym->kind == SYM_VAR && !sym->desc) {
			debug("update symbol '%s' type", var->id);
			STbl_Update_Symbol(&u->stbl, sym, var->desc);
		}
	} else if (u->scope == SCOPE_FUNCTION) {
		debug("var '%s' decl in function", var->id);
		sym = STbl_Add_Var(&u->stbl, var->id, var->desc, var->bconst);
		sym->up = u->sym;
	} else {
		assertm(0, "unknown unit scope:%d", u->scope);
	}

	//generate code
	if (exp) {
		//parser_enter_codeblock(u);
		if (u->scope == SCOPE_MODULE || u->scope == SCOPE_CLASS) {
			// module's or class's variable
			TValue val = INT_VALUE_INIT(0);
			Inst_Append(ps->u->block, OP_LOAD, &val);
			setcstrvalue(&val, var->id);
			Inst_Append(ps->u->block, OP_SETFIELD, &val);
		} else if (u->scope == SCOPE_FUNCTION) {
			// local's variable
			TValue val = INT_VALUE_INIT(sym->index);
			Inst_Append(ps->u->block, OP_STORE, &val);
		} else {
			assert(0);
		}
		//parser_exit_codeblock(u);
	}
}

static void stmt_parse_variable(ParserState *ps, struct stmt *stmt)
{
	struct var *var = stmt->vardecl.var;
	struct expr *exp = stmt->vardecl.exp;
	parse_variable(ps, var, exp);
}

void stmt_parse_function(ParserState *ps, struct stmt *stmt)
{
	parser_enter_scope(ps, SCOPE_FUNCTION);

	ParserUnit *parent = parent_scope(ps);
	Symbol *sym = STbl_Get(&parent->stbl, stmt->funcdecl.id);
	assert(sym);
	ps->u->sym = sym;

	if (parent->scope == SCOPE_MODULE) {
		debug("parse function '%s'", stmt->funcdecl.id);
		if (stmt->funcdecl.pvec) {
			struct var *var;
			Vector_ForEach(var, stmt->funcdecl.pvec)
				parse_variable(ps, var, NULL);
		}
		parse_statements(ps, stmt->funcdecl.body);
		debug("end function '%s'", stmt->funcdecl.id);
	} else if (parent->scope == SCOPE_CLASS) {
		debug("parse method '%s'", stmt->funcdecl.id);
	} else {
		assertm(0, "unknown parent scope: %d", parent->scope);
	}

	parser_exit_scope(ps);
}

static void stmt_parse_assign(ParserState *ps, struct stmt *stmt)
{
	struct expr *r = stmt->assign.right;
	struct expr *l = stmt->assign.left;
	//check_assignment(l, r);
	r->ctx = EXPR_LOAD;
	visit_expr(ps, r);
	l->ctx = EXPR_STORE;
	visit_expr(ps, l);
}

static void stmt_parse_return(ParserState *ps, struct stmt *stmt)
{
	ParserUnit *u = ps->u;
	if (u->scope == SCOPE_FUNCTION) {
		debug("return in function");
		if (stmt->vec) {
			struct expr *e;
			Vector_ForEach(e, stmt->vec) {
				e->ctx = EXPR_LOAD;
				visit_expr(ps, e);
			}
		}
		check_return_types(u, stmt->vec);
	} else {
		assertm(0, "invalid scope:%d", u->scope);
	}
}

void stmt_parse_expr(ParserState *ps, struct stmt *stmt)
{
	assert(stmt->kind == EXPR_KIND);
	visit_expr(ps, stmt->exp);
}

typedef void (*visit_stmt_fn)(ParserState *ps, struct stmt *stmt);

static visit_stmt_fn stmtfuncs[] = {
	NULL,  //INVALID
	NULL,  //IMPORT_KIND
	stmt_parse_variable,  //VARDECL_KIND
	stmt_parse_function,  //FUNCDECL_KIND
	NULL,  //CLASS_KIND
	NULL,  //INTF_KIND
	stmt_parse_expr,  //EXPR_KIND
	stmt_parse_assign,  //ASSIGN_KIND
	NULL,  //COMPOUND_ASSIGN_KIND
	stmt_parse_return,  //RETURN_KIND
};

void parse_statements(ParserState *ps, Vector *stmts)
{
	struct stmt *s;
	Vector_ForEach(s, stmts) {
		int kind = s->kind;
		assert(kind > IMPORT_KIND && kind < STMT_KIND_MAX);
		visit_stmt_fn fn = stmtfuncs[kind];
		assert(fn);
		parser_enter_codeblock(ps->u);
		fn(ps, s);
		parser_exit_codeblock(ps->u);
	}
}

/*--------------------------------------------------------------------------*/

void init_parser(ParserState *ps)
{
	memset(ps, 0, sizeof(ParserState));
	Vector_Init(&ps->stmts);
	init_imports(ps);
	init_list_head(&ps->ustack);
	Vector_Init(&ps->errors);
	init_parser_unit(&ps->mu, NULL, SCOPE_MODULE);
	ps->u = &ps->mu;
}

void fini_parser(ParserState *ps)
{
	vec_stmt_fini(&ps->stmts);
	fini_imports(ps);
	Vector_Fini(&ps->errors, NULL, NULL);
	fini_parser_unit(&ps->mu);
	free(ps->package);
}

void parse(ParserState *ps, FILE *in)
{
	extern FILE *yyin;
	extern int yyparse(ParserState *ps);

	yyin = in;
	yyparse(ps);
	fclose(yyin);

	printf("-------parse-------\n");
	parse_statements(ps, &ps->stmts);
	printf("-------parse End---\n");

	printf("-------------------------\n");
	printf("scope-%d symbols:\n", ps->nestlevel);
	STbl_Show(&ps->u->stbl, 0);
	printf("-------------------------\n");

	check_unused_imports(ps);
	check_unused_symbols(ps);

	parser_finish_unit(ps->u);
}

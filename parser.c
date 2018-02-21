
#include "parser.h"
#include "analyser.h"

extern FILE *yyin;
extern int yyparse(ParserState *ps);
static void parse_statements(ParserState *ps, Vector *stmts);
static void parser_visit_expr(ParserState *ps, struct expr *exp);
static void parser_visit_stmt(ParserState *ps, struct stmt *stmt);
static void enter_codeblock(ParserState *ps);
static void merge_codeblock(ParserState *ps);

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

	sz = TypeDesc_Vec_To_Arr(stmt->funcdecl.rvec, &desc);
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

static void check_symbol(Symbol *sym, void *arg)
{
	UNUSED_PARAMETER(arg);

	if ((sym->access == ACCESS_PRIVATE) && (sym->refcnt == 0)) {
		if (sym->kind == SYM_VAR) {
			warn("variable '%s' is never used", sym->name);
		} else if (sym->kind == SYM_PROTO) {
			warn("function '%s' is never used", sym->name);
		}
	}
}

static void check_variables(ParserState *ps)
{
	ParserUnit *u = ps->u;
	assert(u);
	STbl_Traverse(&u->stbl, check_symbol, NULL);
}

static void check_unused_symbols(ParserState *ps)
{
	Check_Imports(ps);
	check_variables(ps);
}

/*-------------------------------------------------------------------------*/

char *UserDef_Get_Path(ParserState *ps, char *mod)
{
	Symbol *sym = STbl_Get(&ps->extstbl, mod);
	if (!sym) {
		error("cannot find module:%s", mod);
		return NULL;
	}
	assert(sym->kind == SYM_STABLE);
	sym->refcnt = 1;
	return sym->desc->path;
}

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



static void sym_gen_code(Symbol *sym, void *arg)
{
	KImage *image = arg;
	switch (sym->kind) {
		case SYM_VAR: {
			debug("%s %s:", sym->access & ACCESS_CONST ? "const" : "var", sym->name);
			if (sym->access & ACCESS_CONST)
				KImage_Add_Const(image, sym->name, sym->desc);
			else
				KImage_Add_Var(image, sym->name, sym->desc);
			break;
		}
		case SYM_PROTO: {
			debug("func %s:", sym->name);
			CodeBlock *b = sym->ptr;
			int locvars = sym->locvars;
			Inst_Append(b, OP_RET, NULL);
			AtomTable *atbl = image->table;
			Buffer buf;
			Buffer_Init(&buf, 32);
			Inst *i;
			list_for_each_entry(i, &b->insts, link) {
				Inst_Gen(atbl, &buf, i);
			}
			uint8 *data = Buffer_RawData(&buf);
			int size = Buffer_Size(&buf);
			Code_Show(data, size);
			Buffer_Fini(&buf);
			KImage_Add_Func(image, sym->name, sym->desc->proto, locvars, data, size);
			break;
		}
		default: {
			assertm(0, "unknown symbol kind:%d", sym->kind);
		}
	}
}

static void generate_image(ParserState *ps)
{
	printf("----------generate_image--------------------\n");
	ParserUnit *u = ps->u;
	KImage *image = KImage_New(ps->package);
	STbl_Traverse(&u->stbl, sym_gen_code, image);
	KImage_Finish(image);
	KImage_Show(image);
	KImage_Write_File(image, ps->outfile);
	image = KImage_Read_File(ps->outfile);
	KImage_Show(image);
	printf("----------generate_image end----------------\n");
}

/*--------------------------------------------------------------------------*/

static void parse_dotaccess(ParserState *ps, struct expr *exp)
{
	struct expr *left = exp->attribute.left;
	parser_visit_expr(ps, left);
	left->ctx = EXPR_LOAD;
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
		}
	} else {
		assert(0);
	}

	// set expression's symbol
	exp->sym = sym;

	// generate code
	if (ps->gencode) {
		if (sym->kind == SYM_VAR) {
			assert(0);
		} else if (sym->kind == SYM_PROTO) {
			TValue val = CSTR_VALUE_INIT(exp->attribute.id);
			Inst_Append(ps->u->block, OP_CALL, &val);
		} else {
			assert(0);
		}
	}
}

static int check_call_varg(Proto *proto, Vector *vec)
{
	int sz = !vec ? 0: Vector_Size(vec);
	if (proto->psz -1 > sz) {
			return 0;
	} else {
		TypeDesc *desc;
		struct expr *exp;
		Vector_ForEach(exp, vec) {
			if (i < proto->psz - 1)
				desc = proto->pdesc + i;
			else
				desc = proto->pdesc + proto->psz - 1;

			TypeDesc *d = exp->desc;
			if (!d) {
				error("expr's type is null");
				return 0;
			}

			if (d->kind == TYPE_PROTO) {
				Proto *p = d->proto;
				/* allow only one return value as function argument */
				if (p->rsz != 1) return 0;
				if (!TypeDesc_Check(p->rdesc, desc)) return 0;
			} else {
				assert(d->kind == TYPE_PRIMITIVE || d->kind == TYPE_USERDEF);
				if (!TypeDesc_Check(d, desc)) return 0;
			}
		}
		return 1;
	}
}

static int check_call_args(Proto *proto, Vector *vec)
{
	if (Proto_Has_Vargs(proto))
		return check_call_varg(proto, vec);

	int sz = !vec ? 0: Vector_Size(vec);
	if (proto->psz != sz)
		return 0;

	TypeDesc *d;
	struct expr *exp;
	Vector_ForEach(exp, vec) {
		d = exp->desc;
		if (!d) {
			error("expr's type is null");
			return 0;
		}
		if (d->kind == TYPE_PROTO) {
			Proto *p = d->proto;
			/* allow only one return value as function argument */
			if (p->rsz != 1) return 0;
			if (!TypeDesc_Check(p->rdesc, proto->pdesc + i)) return 0;
		} else {
			assert(d->kind == TYPE_PRIMITIVE || d->kind == TYPE_USERDEF);
			if (!TypeDesc_Check(d, proto->pdesc + i)) return 0;
		}
	}

	return 1;
}

static void parse_call(ParserState *ps, struct expr *exp)
{
	if (ps->gencode) {
		if (exp->call.args) {
			struct expr *e;
			Vector_ForEach_Reverse(e, exp->call.args) {
				enter_codeblock(ps);
				parser_visit_expr(ps, e);
				merge_codeblock(ps);
			}
		}
		parser_visit_expr(ps, exp->call.left);
	} else {

		if (exp->call.args) {
			struct expr *e;
			Vector_ForEach(e, exp->call.args) {
				parser_visit_expr(ps, e);
				e->ctx = EXPR_LOAD;
			}
		}

		struct expr *left = exp->call.left;
		parser_visit_expr(ps, left);
		if (!left->sym) {
			error("func is not found");
			return;
		}
		left->ctx = EXPR_LOAD;

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

static void parser_visit_expr(ParserState *ps, struct expr *exp)
{
	switch (exp->kind) {
		case NAME_KIND: {
			Symbol *sym = find_id_symbol(ps, exp->id);
			if (!sym) {
				break;
			}

			if (ps->gencode) {
				if (sym->kind == SYM_STABLE) {
					TValue val = CSTR_VALUE_INIT(sym->path);
					Inst_Append(ps->u->block, OP_LOADM, &val);
				} else if (sym->kind == SYM_VAR) {
					debug("symbol '%s' is variable", exp->id);
					ParserUnit *u = ps->u;
					Symbol *cur = u->sym;
					if (exp->ctx == EXPR_LOAD) {
						debug("load var's index:%d", sym->index);
						if (!sym->up) {
							debug("id '%s' is module's variable", exp->id);
							if (!cur) {
								debug("current scope is module");
								TValue val = INT_VALUE_INIT(0);
								Inst_Append(ps->u->block, OP_LOAD, &val);
								setcstrvalue(&val, sym->name);
								Inst_Append(ps->u->block, OP_GETFIELD, &val);
							} else if (cur->kind == SYM_PROTO) {
								if (!cur->up) {
									debug("id '%s' in function '%s'", exp->id, cur->name);
									TValue val = INT_VALUE_INIT(0);
									Inst_Append(ps->u->block, OP_LOAD, &val);
									setcstrvalue(&val, sym->name);
									Inst_Append(ps->u->block, OP_GETFIELD, &val);
								} else {
									Symbol *parent = cur->up;
									assert(parent->kind == SYM_CLASS);
									debug("id '%s' is referenced in method '%s'",
												exp->id, cur->name);
								}
							} else {
								assert(0);
								TValue val = INT_VALUE_INIT(sym->index);
								Inst_Append(ps->u->block, OP_LOAD, &val);
							}
						} else {
							TValue val = INT_VALUE_INIT(sym->index);
							Inst_Append(ps->u->block, OP_LOAD, &val);
						}
					} else if (exp->ctx == EXPR_STORE) {
						debug("store var's index:%d", sym->index);
						TValue val = INT_VALUE_INIT(sym->index);
						Inst_Append(ps->u->block, OP_STORE, &val);
					} else {
						assertm(0, "unknown ctx:%d", exp->ctx);
					}
				} else if (sym->kind == SYM_PROTO) {
					debug("symbol '%s' is function", exp->id);
					// self object
					TValue val = INT_VALUE_INIT(0);
					Inst_Append(ps->u->block, OP_LOAD, &val);
					setcstrvalue(&val, exp->id);
					Inst_Append(ps->u->block, OP_CALL, &val);
				} else {
					assert(0);
				}
			} else {
				exp->sym = sym;
				if (!exp->desc) {
					char *typestr = TypeDesc_ToString(sym->desc);
					debug("id '%s' is as '%s'", exp->id, typestr);
					free(typestr);
					exp->desc = sym->desc;
				}
			}
			break;
		}
		case INT_KIND: {
			if (exp->ctx == EXPR_STORE) {
				error("cannot assign to %lld", exp->ival);
			}
			if (ps->gencode) {
				//printf("ival=%lld\n", exp->ival);
				TValue val = INT_VALUE_INIT(exp->ival);
				Inst_Append(ps->u->block, OP_LOADK, &val);
			}
			break;
		}
		case FLOAT_KIND: {
			if (exp->ctx == EXPR_STORE) {
				error("cannot assign to %f", exp->fval);
			}
			break;
		}
		case BOOL_KIND: {
			if (exp->ctx == EXPR_STORE) {
				error("cannot assign to %s", exp->bval ? "true":"false");
			}
			break;
		}
		case STRING_KIND: {
			if (exp->ctx == EXPR_STORE) {
				error("cannot assign to %s", exp->str);
				break;
			}
			if (ps->gencode) {
				TValue val = CSTR_VALUE_INIT(exp->str);
				Inst_Append(ps->u->block, OP_LOADK, &val);
			}
			break;
		}
		case ATTRIBUTE_KIND: {
			parse_dotaccess(ps, exp);
			break;
		}
		case CALL_KIND: {
			parse_call(ps, exp);
			break;
		}
		case BINARY_KIND: {
			debug("binary_op:%d", exp->binary.op);
			exp->binary.right->ctx = EXPR_LOAD;
			parser_visit_expr(ps, exp->binary.right);
			exp->binary.left->ctx = EXPR_LOAD;
			parser_visit_expr(ps, exp->binary.left);
			if (ps->gencode) {
				if (exp->binary.op == BINARY_ADD) {
					debug("add 'OP_ADD' inst");
					Inst_Append(ps->u->block, OP_ADD, NULL);
				} else if (exp->binary.op == BINARY_SUB) {
					debug("add 'OP_SUB' inst");
					Inst_Append(ps->u->block, OP_SUB, NULL);
				}
			} else {
				exp->desc = exp->binary.left->desc;
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
			break;
		}
		default:
			assertm(0, "unknown expression type: %d", exp->kind);
			break;
	}
}

/*--------------------------------------------------------------------------*/

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
	STbl_Fini(&u->stbl);
}

static ParserUnit *parser_unit_new(AtomTable *atbl, int scope)
{
	ParserUnit *u = calloc(1, sizeof(ParserUnit));
	init_parser_unit(u, atbl, scope);
	return u;
}

static void parser_unit_free(ParserUnit *u)
{
	STbl_Fini(&u->stbl);
	CodeBlock_Free(u->block);
	free(u);
}

static void parser_enter_scope(ParserState *ps, int scope)
{
	AtomTable *atbl = NULL;
	if (ps->u) atbl = ps->u->stbl.atbl;
	ParserUnit *u = parser_unit_new(atbl, scope);

	/* Push the old ParserUnit on the stack. */
	if (ps->u) {
		list_add(&ps->u->link, &ps->ustack);
	}
	ps->u = u;
	ps->nestlevel++;
}

static void enter_codeblock(ParserState *ps)
{
	ParserUnit *u = ps->u;
	CodeBlock *block = CodeBlock_New(u->stbl.atbl);
	if (u->block) list_add(&u->block->link, &u->blocks);
	u->block = block;
}

static void merge_codeblock(ParserState *ps)
{
	ParserUnit *u = ps->u;
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

static void save_code(ParserState *ps)
{
	if (!ps->gencode) return;

	ParserUnit *u = ps->u;
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
			assert(u == &ps->mu);
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
		}
	} else {
		assertm(0, "no codes in scope:%d", u->scope);
	}
}

static void parser_exit_scope(ParserState *ps)
{
	printf("-------------------------\n");
	printf("scope-%d symbols:\n", ps->nestlevel);
	STbl_Show(&ps->u->stbl, 0);
	CodeBlock_Show(ps->u->block);
	printf("-------------------------\n");

	check_unused_symbols(ps);

	save_code(ps);

	ps->nestlevel--;
	parser_unit_free(ps->u);

	/* Restore c->u to the parent unit. */
	struct list_head *first = list_first(&ps->ustack);
	if (first) {
		list_del(first);
		ps->u = container_of(first, ParserUnit, link);
	} else {
		ps->u = NULL;
	}
}

/*--------------------------------------------------------------------------*/

static void parse_variable(ParserState *ps, struct var *var, struct expr *exp)
{
	ParserUnit *u = ps->u;
	if (ps->gencode) {
		if (exp) {
			enter_codeblock(ps);
			parser_visit_expr(ps, exp);
			if ((u->scope == SCOPE_MODULE || u->scope == SCOPE_CLASS)) {
				// module's or class's variable
				TValue val = INT_VALUE_INIT(0);
				Inst_Append(ps->u->block, OP_LOAD, &val);
				setcstrvalue(&val, var->id);
				Inst_Append(ps->u->block, OP_SETFIELD, &val);
			} else if (u->scope == SCOPE_FUNCTION) {
				Symbol *sym = STbl_Add_Var(&u->stbl, var->id, var->desc, var->bconst);
				sym->up = u->sym;
				// local variable
				TValue val = INT_VALUE_INIT(sym->index);
				Inst_Append(ps->u->block, OP_STORE, &val);
			} else {
				assert(0);
			}
			merge_codeblock(ps);
		} else {
			if (u->scope == SCOPE_MODULE || u->scope == SCOPE_CLASS) {
				debug("parse variable '%s' declaration in module or class", var->id);
			} else if (u->scope == SCOPE_FUNCTION) {
				debug("parse variable '%s' declaration in function", var->id);
				assert(!list_empty(&ps->ustack));
				ParserUnit *parent = parent_scope(ps);
				assert(parent->scope == SCOPE_MODULE || parent->scope == SCOPE_CLASS);
				Symbol *sym = STbl_Add_Var(&u->stbl, var->id, var->desc, var->bconst);
				sym->up = u->sym;
			} else {
				assertm(0, "unknown unit scope:%d", u->scope);
			}
		}
	} else {
		if (!exp) {
			assert(var->desc);
		} else {
			if (exp->kind == NIL_KIND) {
				debug("nil value");
			}

			if (exp->kind == SELF_KIND) {
				error("cannot use keyword 'self'");
			}

			if (!exp->desc) {
				debug("parse right expr, and set its type");
				parser_visit_expr(ps, exp);
			}

			if (!exp->desc) {
				error("cannot resolve var '%s' type", var->id);
			} else {
				if (!var->desc) {
					debug("using its right type as var '%s' type", var->id);
					var->desc = exp->desc;
					if (!var->desc) {
						error("var '%s' type is null", var->id);
					}
				} else {
					if (!TypeDesc_Check(var->desc, exp->desc)) {
						error("typecheck failed");
					}
				}
			}
		}

		debug("parse variable '%s' declaration", var->id);

		if (u->scope == SCOPE_MODULE || u->scope == SCOPE_CLASS) {
			debug("in module or class");
			Symbol *sym = STbl_Get(&u->stbl, var->id);
			assert(sym);
			if (sym->kind == SYM_VAR) {
				if (!sym->desc) {
					debug("update symbol '%s' type", var->id);
					STbl_Update_Symbol(&u->stbl, sym, var->desc);
				} else {
					debug("no need update symbol '%s'", var->id);
				}
			}
		} else if (u->scope == SCOPE_FUNCTION) {
			debug("in function");
			assert(!list_empty(&ps->ustack));
			ParserUnit *parent = parent_scope(ps);
			assert(parent->scope == SCOPE_MODULE || parent->scope == SCOPE_CLASS);
			Symbol *sym = STbl_Add_Var(&u->stbl, var->id, var->desc, var->bconst);
			sym->up = u->sym;
		} else {
			assertm(0, "unknown unit scope:%d", u->scope);
		}
	}
}

static void parse_function(ParserState *ps, struct stmt *stmt)
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
		assertm(0, "unknown parent scope type:%d", parent->scope);
	}

	parser_exit_scope(ps);
}

static void parse_assign(ParserState *ps, struct stmt *stmt)
{
	struct expr *r = stmt->assign.right;
	struct expr *l = stmt->assign.left;
	r->ctx = EXPR_LOAD;
	parser_visit_expr(ps, r);
	l->ctx = EXPR_STORE;
	parser_visit_expr(ps, l);
}

static void paser_return(ParserState *ps, struct stmt *stmt)
{
	ParserUnit *u = ps->u;
	if (u->scope == SCOPE_FUNCTION) {
		debug("return in function");
		if (stmt->vec) {
			struct expr *e;
			Vector_ForEach(e, stmt->vec) {
				e->ctx = EXPR_LOAD;
				parser_visit_expr(ps, e);
			}
		}

		if (!ps->gencode) {
			check_return_types(u, stmt->vec);
		}
	} else {
		assertm(0, "invalid scope:%d", u->scope);
	}
}

static void parser_visit_stmt(ParserState *ps, struct stmt *stmt)
{
	if (ps->gencode) enter_codeblock(ps);

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
		default:
			assertm(0, "unknown statement type: %d", stmt->kind);
			break;
	}

	if (ps->gencode) merge_codeblock(ps);
}

/*--------------------------------------------------------------------------*/

static void parse_statements(ParserState *ps, Vector *stmts)
{
	struct stmt *s;
	Vector_ForEach(s, stmts) {
		parser_visit_stmt(ps, s);
	}
}

static void generate_code(ParserState *ps)
{
	printf("-------code generator----------\n");
	ps->gencode = 1;
	parse_statements(ps, &ps->stmts);
	CodeBlock_Show(ps->u->block);
	printf("-------code generator end------\n");
}

static void init_parser(ParserState *ps)
{
	Koala_Init();
	memset(ps, 0, sizeof(ParserState));
	Vector_Init(&ps->stmts);
	init_imports(ps);
	init_list_head(&ps->ustack);
	Vector_Init(&ps->errors);
	init_parser_unit(&ps->mu, NULL, SCOPE_MODULE);
	ps->u = &ps->mu;
}

static void fini_parser(ParserState *ps)
{
	vec_stmt_fini(&ps->stmts);
	fini_imports(ps);
	Vector_Fini(&ps->errors, NULL, NULL);
	fini_parser_unit(&ps->mu);
	free(ps->package);
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

	char *srcfile = argv[1];
	int len = strlen(srcfile);
	char *outfile = malloc(len + 4 + 1);
	char *tmp = strrchr(srcfile, '.');
	if (tmp) {
		memcpy(outfile, srcfile, tmp - srcfile);
		outfile[tmp - srcfile] = 0;
	} else {
		strcpy(outfile, srcfile);
	}
	strcat(outfile, ".klc");
	ps.outfile = outfile;

	yyin = fopen(argv[1], "r");
	yyparse(&ps);
	fclose(yyin);

	parse_statements(&ps, &ps.stmts);

	printf("-------------------------\n");
	printf("scope-%d symbols:\n", ps.nestlevel);
	STbl_Show(&ps.u->stbl, 0);
	printf("-------------------------\n");
	check_unused_symbols(&ps);

	generate_code(&ps);
	save_code(&ps);
	generate_image(&ps);

	printf("package:%s\n", ps.package);
	STbl_Show(&ps.u->stbl, 1);
	Check_Imports(&ps);

	fini_parser(&ps);

	return 0;
}

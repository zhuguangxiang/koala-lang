
#include "parser.h"
#include "codegen.h"
#include "koala_state.h"
#include "checker.h"
#include "codegen.h"
#include "opcode.h"
#include "moduleobject.h"
#include "log.h"

static JmpInst *JmpInst_New(Inst *inst, int type)
{
	JmpInst *jmp = malloc(sizeof(JmpInst));
	jmp->inst = inst;
	jmp->type = type;
	return jmp;
}

static void JmpInst_Free(JmpInst *jmp)
{
	free(jmp);
}

static CodeBlock *codeblock_new(void)
{
	CodeBlock *b = calloc(1, sizeof(CodeBlock));
	//init_list_head(&b->link);
	//STable_Init(&b->stbl, atbl);
	init_list_head(&b->insts);
	return b;
}

void codeblock_free(CodeBlock *b)
{
	if (!b) return;
	//assert(list_unlinked(&b->link));
	//STable_Fini(&b->stbl);

	Inst *i, *n;
	list_for_each_entry_safe(i, n, &b->insts, link) {
		list_del(&i->link);
		Inst_Free(i);
	}

	free(b);
}

static void codeblock_merge(CodeBlock *from, CodeBlock *to)
{
	Inst *i, *n;
	list_for_each_entry_safe(i, n, &from->insts, link) {
		list_del(&i->link);
		from->bytes -= i->bytes;
		list_add_tail(&i->link, &to->insts);
		to->bytes += i->bytes;
		i->upbytes = to->bytes;
	}
	assert(!from->bytes);

	CodeBlock *b = from->next;
	while (b) {
		list_for_each_entry_safe(i, n, &b->insts, link) {
			list_del(&i->link);
			b->bytes -= i->bytes;
			list_add_tail(&i->link, &to->insts);
			to->bytes += i->bytes;
			i->upbytes = to->bytes;
		}
		assert(!b->bytes);
		//FIXME: codeblock_free(b);
		b = b->next;
	}
}

#if SHOW_ENABLED
static void codeblock_show(CodeBlock *block)
{
	if (!block) return;

	char buf[64];

	debug("---------CodeBlock-------");
	debug("insts:%d", block->bytes);
	if (!list_empty(&block->insts)) {
		int cnt = 0;
		Inst *i;
		list_for_each_entry(i, &block->insts, link) {
			printf("[%d]:\n", cnt++);
			printf("  opcode:%s\n", opcode_string(i->op));
			TValue_Print(buf, sizeof(buf), &i->arg, 0);
			printf("  arg:%s\n", buf);
			printf("  bytes:%d\n", i->bytes);
			printf("-----------------\n");
		}
	}
	debug("--------CodeBlock End----");
}
#endif

/*-------------------------------------------------------------------------*/
static void parser_enter_scope(ParserState *ps, STable *stbl, int scope);
static void parser_exit_scope(ParserState *ps);
static void parser_visit_expr(ParserState *ps, struct expr *exp);
static void parser_body(ParserState *ps, Vector *stmts);
static ParserUnit *get_class_unit(ParserState *ps);

/*-------------------------------------------------------------------------*/

static Symbol *find_external_package(ParserState *ps, char *path)
{
	Import key = {.path = path};
	Import *import = HashTable_Find(&ps->imports, &key);
	if (!import) {
		error("cannot find '%s' package", path);
		return NULL;
	}
	return import->sym;
}

static Symbol *find_userdef_symbol(ParserState *ps, TypeDesc *desc)
{
	if (desc->kind != TYPE_USERDEF) {
		error("type(%s) is not class or interface", TypeDesc_ToString(desc));
		return NULL;
	}

	// find in current module
	Symbol *sym;
	if (!desc->path) {
		debug("type '%s' maybe in current module", desc->type);
		sym = STable_Get(ps->sym->ptr, desc->type);
		if (!sym) {
			error("cannot find %s", desc->type);
			return NULL;
		}
		sym->refcnt++;
		return sym;
	}

	// find in external imported module
	sym = find_external_package(ps, desc->path);
	if (!sym) {
		error("cannot find '%s' package", desc->path);
		return NULL;
	}

	assert(sym->kind == SYM_STABLE);
	sym = STable_Get(sym->ptr, desc->type);
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
	HashTable_Init(&ps->imports, &hashinfo);
	ps->extstbl = STable_New(NULL);
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
	HashTable_Fini(&ps->imports, __import_free_fn, NULL);
	STable_Free(ps->extstbl);
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
	Symbol *sym = STable_Add_Symbol(stbl, id, SYM_STABLE, 0);
	if (!sym) return NULL;
	sym->path = strdup(path);
	return sym;
}

Symbol *Parse_Import(ParserState *ps, char *id, char *path)
{
	Import key = {.path = path};
	Import *import = HashTable_Find(&ps->imports, &key);
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
	if (HashTable_Insert(&ps->imports, &import->hnode) < 0) {
		error("module '%s' is imported duplicated", path);
		return NULL;
	}
	Object *ob = Koala_Load_Module(path);
	if (!ob) {
		error("load module '%s' failed", path);
		HashTable_Remove(&ps->imports, &import->hnode);
		import_free(import);
		return NULL;
	}
	if (!id) id = Module_Name(ob);
	sym = add_import(ps->extstbl, id, path);
	if (!sym) {
		debug("add import '%s <- %s' failed", id, path);
		HashTable_Remove(&ps->imports, &import->hnode);
		import_free(import);
		return NULL;
	}

	sym->ptr = Module_To_STable(ob, ps->extstbl->atbl, path);
	import->sym = sym;
	debug("add import '%s <- %s' successful", id, path);
	return sym;
}

char *Import_Get_Path(ParserState *ps, char *id)
{
	Symbol *sym = STable_Get(ps->extstbl, id);
	if (!sym) {
		error("cannot find module:%s", id);
		return NULL;
	}
	assert(sym->kind == SYM_STABLE);
	sym->refcnt = 1;
	return sym->path;
}

static inline void __add_stmt(ParserState *ps, struct stmt *stmt)
{
	Vector_Append(&(ps)->stmts, stmt);
}

static void parse_vardecl(ParserState *ps, struct stmt *stmt)
{
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

	Symbol *sym = STable_Add_Var(ps->u->stbl, var->id, var->desc, var->bconst);
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
			__add_stmt(ps, s);
			parse_vardecl(ps, s);
		}
	} else {
		assert(stmt->kind == VARDECL_KIND);
		__add_stmt(ps, stmt);
		parse_vardecl(ps, stmt);
	}
}

static int var_vec_to_arr(Vector *vec, TypeDesc **arr)
{
	if (!vec) {
		*arr = NULL;
		return 0;
	}
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

static void parse_funcdecl(ParserState *ps, struct stmt *stmt)
{
	Proto *proto;
	Symbol *sym;
	proto = funcdecl_to_proto(stmt);
	sym = STable_Add_Proto(ps->u->stbl, stmt->funcdecl.id, proto);
	if (sym) {
		debug("add func '%s' successful", stmt->funcdecl.id);
		sym->up = ps->u->sym;
	} else {
		debug("add func '%s' failed", stmt->funcdecl.id);
	}
}

void Parse_Function(ParserState *ps, struct stmt *stmt)
{
	__add_stmt(ps, stmt);
	parse_funcdecl(ps, stmt);
}

static Proto *funcproto_to_proto(struct stmt *stmt)
{
	Proto *proto = malloc(sizeof(Proto));
	int sz;
	TypeDesc *desc;

	sz = TypeVec_ToTypeArray(stmt->funcproto.pvec, &desc);
	proto->psz = sz;
	proto->pdesc = desc;

	sz = TypeVec_ToTypeArray(stmt->funcproto.rvec, &desc);
	proto->rsz = sz;
	proto->rdesc = desc;

	return proto;
}

static void parse_funcproto(ParserState *ps, struct stmt *stmt)
{
	Proto *proto;
	Symbol *sym;
	proto = funcproto_to_proto(stmt);
	sym = STable_Add_IProto(ps->u->stbl, stmt->funcproto.id, proto);
	if (sym) {
		debug("add abstract func '%s' successful", stmt->funcproto.id);
		sym->up = ps->u->sym;
	} else {
		debug("add abstract func '%s' failed", stmt->funcproto.id);
	}
}

void Parse_UserDef(ParserState *ps, struct stmt *stmt)
{
	Symbol *sym = NULL;
	__add_stmt(ps, stmt);

	if (stmt->kind == CLASS_KIND) {
		sym = STable_Add_Class(ps->u->stbl, stmt->class_type.id);
	} else if (stmt->kind == TRAIT_KIND) {
		sym = STable_Add_Trait(ps->u->stbl, stmt->class_type.id);
	} else {
		assert(0);
	}

	sym->up = ps->u->sym;
	sym->ptr = STable_New(ps->u->stbl->atbl);
	sym->desc = TypeDesc_From_UserDef(NULL, stmt->class_type.id);
	debug("add class(trait) '%s' successful", sym->name);
	struct stmt *s;
	parser_enter_scope(ps, sym->ptr, SCOPE_CLASS);
	ps->u->sym = sym;
	Vector_ForEach(s, stmt->class_type.body) {
		if (s->kind == VARDECL_KIND) {
			parse_vardecl(ps, s);
		} else if (s->kind == FUNCDECL_KIND) {
			parse_funcdecl(ps, s);
		} else {
			assert(s->kind == FUNCPROTO_KIND);
			parse_funcproto(ps, s);
		}
	}
	parser_exit_scope(ps);
	debug("end class(trait) '%s'", sym->name);
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

static void parser_show_scope(ParserState *ps)
{
#if SHOW_ENABLED
	debug("------scope show-------------");
	debug("scope-%d symbols:", ps->nestlevel);
	STable_Show(ps->u->stbl, 0);
	codeblock_show(ps->u->block);
	debug("-----scope show end----------");
#else
	UNUSED_PARAMETER(ps);
#endif
}

static void parser_new_block(ParserUnit *u)
{
	CodeBlock *block = codeblock_new();
	assert(!u->block);
	u->block = block;
}

/*--------------------------------------------------------------------------*/

static void parser_merge(ParserState *ps)
{
	ParserUnit *u = ps->u;
	// save code to symbol
	if (u->scope == SCOPE_FUNCTION || u->scope == SCOPE_METHOD) {
		if (u->scope == SCOPE_METHOD && !strcmp(u->sym->name, "__init__")) {
			debug("class __init__ function");
			TValue val = INT_VALUE_INIT(0);
			Inst_Append(u->block, OP_LOAD, &val);
		}
		if (!u->block->bret) {
			debug("add 'return' to function '%s'", u->sym->name);
			Inst_Append(u->block, OP_RET, NULL);
		}
		u->sym->ptr = u->block;
		u->block = NULL;
		u->sym->locvars = u->stbl->varcnt;
		u->stbl = NULL;
		debug("save code to function '%s'", u->sym->name);
	} else if (u->scope == SCOPE_BLOCK) {
		if (u->loop) {
			// loop-statement check break or continue statement
			int offset;
			TValue val;
			JmpInst *jmp;
			Vector_ForEach(jmp, &u->jmps) {
				if (jmp->type == JMP_BREAK) {
					offset = u->block->bytes - jmp->inst->upbytes;
				} else {
					assert(jmp->type == JMP_CONTINUE);
					offset = 0 - jmp->inst->upbytes;
				}
				setivalue(&val, offset);
				jmp->inst->arg = val;
			}
			u->merge = 1;
		}

		ParserUnit *parent = parent_scope(ps);
		debug("merge code to parent's block(%d)", parent->scope);
		assert(parent->block);
		if (u->merge || parent->scope == SCOPE_FUNCTION ||
			u->scope == SCOPE_METHOD) {
			debug("merge code to up block");
			codeblock_merge(u->block, parent->block);
		} else {
			debug("set block as up block's next");
			assert(parent->scope == SCOPE_BLOCK);
			parent->block->next = u->block;
			u->block = NULL;
		}
	} else if (u->scope == SCOPE_MODULE) {
		Proto *proto = Proto_New(0, NULL, 0, NULL);
		Symbol *sym = STable_Add_Proto(u->stbl, "__init__", proto);
		assert(sym);
		debug("add 'return' to function '__init__'");
		Inst_Append(u->block, OP_RET, NULL);
		if (u->block && u->block->bytes > 0) {
			sym->ptr = u->block;
			//sym->stbl = u->stbl;
			sym->locvars = u->stbl->varcnt;
			//u->stbl = NULL;
		}
		u->sym = sym;
		u->block = NULL;
		debug("save code to module '__init__' function");
	} else if (u->scope == SCOPE_CLASS) {
		debug(">>>> class or trait");
	} else {
		assertm(0, "no codes in scope:%d", u->scope);
	}
}

static void parser_init_unit(ParserUnit *u, STable *stbl, int scope)
{
	init_list_head(&u->link);
	u->stbl = stbl;
	u->sym = NULL;
	u->block = NULL;
	u->scope = scope;
	Vector_Init(&u->jmps);
}

static void vec_jmpinst_free_fn(void *item, void *arg)
{
	UNUSED_PARAMETER(arg);
	JmpInst_Free(item);
}

static void parser_fini_unit(ParserUnit *u)
{
	list_del(&u->link);
	CodeBlock *b = u->block;
	//FIXME:
	if (b) {
		assert(list_empty(&b->insts));
	}

	Vector_Fini(&u->jmps, vec_jmpinst_free_fn, NULL);
	//FIXME
	//assert(!u->stbl);
	//STable_Free(u->stbl);
}

static ParserUnit *parser_new_unit(STable *stbl, int scope)
{
	ParserUnit *u = calloc(1, sizeof(ParserUnit));
	parser_init_unit(u, stbl, scope);
	return u;
}

static void parser_free_unit(ParserUnit *u)
{
	parser_fini_unit(u);
	free(u);
}

static void parser_enter_scope(ParserState *ps, STable *stbl, int scope)
{
	AtomTable *atbl = NULL;
	if (ps->u) atbl = ps->u->stbl->atbl;
	if (!stbl) stbl = STable_New(atbl);
	ParserUnit *u = parser_new_unit(stbl, scope);

	/* Push the old ParserUnit on the stack. */
	if (ps->u) {
		list_add(&ps->u->link, &ps->ustack);
	}
	ps->u = u;
	ps->nestlevel++;
	parser_new_block(ps->u);
}

static void parser_exit_scope(ParserState *ps)
{
	parser_show_scope(ps);

	check_unused_symbols(ps);

	parser_merge(ps);

	ParserUnit *u = ps->u;

	if (u->scope == SCOPE_MODULE) {
		//save module's symbol to ps
		//assert(ps->sym->ptr == u->stbl);
		u->stbl = NULL;
	} else if (u->scope == SCOPE_CLASS) {
		assert(!list_empty(&ps->ustack));
		ParserUnit *parent = list_first_entry(&ps->ustack, ParserUnit, link);
		assert(parent->scope == SCOPE_MODULE);
		// class symbol table is stored in class symbol
		u->stbl = NULL;
	} else {
		debug(">>>> exit scope:%d", u->scope);
	}

	parser_free_unit(u);

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

/*--------------------------------------------------------------------------*/

static void parser_expr_desc(ParserState *ps, struct expr *exp, Symbol *sym)
{
	exp->sym = sym;
	if (!exp->desc) {
		if (sym->kind == SYM_MODULE) {
			debug("ident '%s' is as Module", ps->package);
		} else {
			char *typestr = TypeDesc_ToString(sym->desc);
			debug("ident '%s' is as '%s'", exp->id, typestr);
			free(typestr);
			exp->desc = sym->desc;
		}
	}
}

static void parser_curscope_ident(ParserState *ps, Symbol *sym,
	struct expr *exp)
{
	ParserUnit *u = ps->u;
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
				TValue val = INT_VALUE_INIT(0);
				Inst_Append(u->block, OP_LOAD, &val);
				setcstrvalue(&val, sym->name);
				Inst *i = Inst_Append(u->block, OP_CALL, &val);
				i->argc = exp->argc;
			} else {
				assertm(0, "invalid symbol kind :%d", sym->kind);
			}
			break;
		}
		case SCOPE_FUNCTION:
		case SCOPE_METHOD:
		case SCOPE_BLOCK: {
			if (sym->kind == SYM_VAR) {
				debug("symbol '%s' is variable", sym->name);
				int opcode;
				debug("local's variable");
				opcode = (exp->ctx == EXPR_LOAD) ? OP_LOAD : OP_STORE;
				TValue val = INT_VALUE_INIT(sym->index);
				Inst_Append(u->block, opcode, &val);
			} else {
				assertm(0, "invalid symbol kind :%d", sym->kind);
			}
			break;
		}
		default: {
			assertm(0, "invalid scope:%d", u->scope);
			break;
		}
	}
}

static void parser_upscope_ident(ParserState *ps, Symbol *sym,
	struct expr *exp)
{
	ParserUnit *u = ps->u;
	switch (u->scope) {
		case SCOPE_CLASS: {
			assert(sym->up->kind == SYM_MODULE);
			debug("symbol '%s' in module '%s'", sym->name, ps->package);
			if (sym->kind == SYM_VAR) {
				debug("symbol '%s' is variable", sym->name);
				TValue val = INT_VALUE_INIT(0);
				Inst_Append(u->block, OP_LOAD, &val);
				Inst_Append(u->block, OP_GETM, NULL);
				setcstrvalue(&val, sym->name);
				Inst_Append(u->block, OP_GETFIELD, &val);
			} else if (sym->kind == SYM_PROTO) {
				debug("symbol '%s' is function", sym->name);
				TValue val = INT_VALUE_INIT(0);
				Inst_Append(u->block, OP_LOAD, &val);
				Inst_Append(u->block, OP_GETM, NULL);
				setcstrvalue(&val, sym->name);
				Inst *i = Inst_Append(u->block, OP_CALL, &val);
				i->argc = exp->argc;
			} else {
				assertm(0, "invalid symbol kind :%d", sym->kind);
			}
			break;
		}
		case SCOPE_FUNCTION: {
			assert(sym->up->kind == SYM_MODULE);
			debug("symbol '%s' in module '%s'", sym->name, ps->package);
			if (sym->kind == SYM_VAR) {
				debug("symbol '%s' is variable", sym->name);
				int opcode;
				TValue val = INT_VALUE_INIT(0);
				Inst_Append(u->block, OP_LOAD, &val);
				opcode = (exp->ctx == EXPR_LOAD) ? OP_GETFIELD : OP_SETFIELD;
				setcstrvalue(&val, sym->name);
				Inst_Append(u->block, opcode, &val);
			} else if (sym->kind == SYM_PROTO) {
				debug("symbol '%s' is function", sym->name);
				TValue val = INT_VALUE_INIT(0);
				Inst_Append(u->block, OP_LOAD, &val);
				setcstrvalue(&val, sym->name);
				Inst *i = Inst_Append(u->block, OP_CALL, &val);
				i->argc = exp->argc;
			} else if (sym->kind == SYM_CLASS) {
				debug("symbol '%s' is class", sym->name);
				if (exp->right && exp->right->kind == CALL_KIND) {
					struct expr *right = exp->right;
					if (!right->right) {
						debug("new object :%s", exp->sym->name);
						TValue val = INT_VALUE_INIT(0);
						Inst_Append(u->block, OP_LOAD, &val);
						Inst_Append(u->block, OP_GETM, NULL);
						setcstrvalue(&val, sym->name);
						Inst *i = Inst_Append(u->block, OP_NEW, &val);
						i->argc = exp->argc;
					} else {
						struct expr *rr = right->right;
						assert(rr->kind == WITH_KIND);
						debug("class '%s' with trait '%s'", exp->sym->name, rr->sym->name);
					}
				} else {
					if (!exp->right) {
						debug("'%s' is a class", sym->name);
						TValue val = INT_VALUE_INIT(0);
						Inst_Append(u->block, OP_LOAD, &val);
						Inst_Append(u->block, OP_GETM, NULL);
					} else {
						assert(0);
					}
				}
			} else {
				assertm(0, "invalid symbol kind :%d", sym->kind);
			}
			break;
		}
		case SCOPE_METHOD: {
			if (sym->up->kind == SYM_CLASS) {
				debug("symbol '%s' in class '%s'", sym->name, sym->up->name);
				debug("symbol '%s' is inherited ? %s", sym->name,
					sym->inherited ? "true" : "false");
				if (sym->kind == SYM_VAR) {
					debug("symbol '%s' is variable", sym->name);
					int opcode;
					TValue val = INT_VALUE_INIT(0);
					Inst_Append(u->block, OP_LOAD, &val);
					opcode = (exp->ctx == EXPR_LOAD) ? OP_GETFIELD : OP_SETFIELD;
					setcstrvalue(&val, sym->name);
					Inst_Append(u->block, opcode, &val);
				} else if (sym->kind == SYM_PROTO) {
					debug("symbol '%s' is function", sym->name);
					TValue val = INT_VALUE_INIT(0);
					Inst_Append(u->block, OP_LOAD, &val);
					setcstrvalue(&val, sym->name);
					Inst *i = Inst_Append(u->block, OP_CALL, &val);
					i->argc = exp->argc;
				} else {
					assertm(0, "invalid symbol kind :%d", sym->kind);
				}
			} else {
				assert(sym->up->kind == SYM_MODULE);
				debug("symbol '%s' in module '%s'", sym->name, ps->package);
				if (sym->kind == SYM_VAR) {
					debug("symbol '%s' is variable", sym->name);
					int opcode;
					TValue val = INT_VALUE_INIT(0);
					Inst_Append(u->block, OP_LOAD, &val);
					opcode = (exp->ctx == EXPR_LOAD) ? OP_GETFIELD : OP_SETFIELD;
					Inst_Append(u->block, OP_GETM, NULL);
					setcstrvalue(&val, sym->name);
					Inst_Append(u->block, opcode, &val);
				} else if (sym->kind == SYM_PROTO) {
					debug("symbol '%s' is function", sym->name);
					TValue val = INT_VALUE_INIT(0);
					Inst_Append(u->block, OP_LOAD, &val);
					Inst_Append(u->block, OP_GETM, NULL);
					setcstrvalue(&val, sym->name);
					Inst *i = Inst_Append(u->block, OP_CALL, &val);
					i->argc = exp->argc;
				} else if (sym->kind == SYM_CLASS) {
					debug("symbol '%s' is class", sym->name);
					TValue val = INT_VALUE_INIT(0);
					Inst_Append(u->block, OP_LOAD, &val);
					Inst_Append(u->block, OP_GETM, NULL);
					setcstrvalue(&val, sym->name);
					Inst *i = Inst_Append(u->block, OP_NEW, &val);
					i->argc = exp->argc;
				} else {
					assertm(0, "invalid symbol kind :%d", sym->kind);
				}
			}
			break;
		}
		case SCOPE_BLOCK: {
			Symbol *up = sym->up;
			if (!up || up->kind == SYM_PROTO) {
				debug("symbol '%s' is local variable", sym->name);
				// local's variable
				int opcode = (exp->ctx == EXPR_LOAD) ? OP_LOAD : OP_STORE;
				TValue val = INT_VALUE_INIT(sym->index);
				Inst_Append(u->block, opcode, &val);
			} else if (up->kind == SYM_CLASS) {
				debug("symbol '%s' is class variable", sym->name);
				debug("symbol '%s' is inherited ? %s", sym->name,
					sym->inherited ? "true" : "false");
				TValue val = INT_VALUE_INIT(0);
				Inst_Append(u->block, OP_LOAD, &val);
				int opcode = (exp->ctx == EXPR_LOAD) ? OP_GETFIELD : OP_SETFIELD;
				setcstrvalue(&val, sym->name);
				Inst_Append(u->block, opcode, &val);
			} else if (up->kind == SYM_MODULE) {
				debug("symbol '%s' is module variable", sym->name);
				TValue val = INT_VALUE_INIT(0);
				Inst_Append(u->block, OP_LOAD, &val);
				Inst_Append(u->block, OP_GETM, NULL);
				int opcode = (exp->ctx == EXPR_LOAD) ? OP_GETFIELD : OP_SETFIELD;
				setcstrvalue(&val, sym->name);
				Inst_Append(u->block, opcode, &val);
			} else {
				assertm(0, "invalid up symbol kind :%d", up->kind);
			}
			break;
		}
		default: {
			assertm(0, "invalid scope:%d", u->scope);
			break;
		}
	}
}

#if 0
static void parser_superclass_ident(ParserState *ps, Symbol *sym,
	struct expr *exp)
{
	ParserUnit *u = ps->u;
	switch (u->scope) {
		case SCOPE_METHOD:
		case SCOPE_BLOCK: {
			ParserUnit *pu = get_class_unit(ps);
			assert(ps);
			break;
		}
		default: {
			assertm(0, "invalid scope:%d", u->scope);
			break;
		}
	}
}
#endif

static void parser_ident(ParserState *ps, struct expr *exp)
{
	ParserUnit *u = ps->u;
	char *id = exp->id;

	// find ident from local scope
	Symbol *sym = STable_Get(u->stbl, id);
	if (sym) {
		debug("symbol '%s' is found in local scope", id);
		sym->refcnt++;
		parser_expr_desc(ps, exp, sym);
		parser_curscope_ident(ps, sym, exp);
		return;
	}

	// find ident from parent scope
	ParserUnit *up;
	list_for_each_entry(up, &ps->ustack, link) {
		sym = STable_Get(up->stbl, id);
		if (sym) {
			debug("symbol '%s' is found in up scope", id);
			sym->refcnt++;
			parser_expr_desc(ps, exp, sym);
			parser_upscope_ident(ps, sym, exp);
			return;
		}
	}

	// ident is in super class
	Symbol *super = NULL;
	ParserUnit *cu = get_class_unit(ps);
	if (cu) {
		super = cu->sym->super;
	}

	if (super) {
		sym = STable_Get(super->ptr, id);
		if (sym) {
			debug("symbol '%s' is found in super '%s' class", id, super->name);
			sym->refcnt++;
			parser_expr_desc(ps, exp, sym);

			TValue val;
			setcstrvalue(&val, id);

			if (sym->kind == SYM_VAR) {
				if (exp->ctx == EXPR_LOAD) {
					debug("load:%s", val.cstr);
					TValue val2 = INT_VALUE_INIT(0);
					Inst_Append(u->block, OP_LOAD, &val2);
					Inst_Append(ps->u->block, OP_GETFIELD, &val);
				} else {
					assert(exp->ctx == EXPR_STORE);
					debug("store:%s", val.cstr);
					TValue val2 = INT_VALUE_INIT(0);
					Inst_Append(u->block, OP_LOAD, &val2);
					Inst_Append(ps->u->block, OP_SETFIELD, &val);
				}
			} else if (sym->kind == SYM_PROTO) {
				TValue val2 = INT_VALUE_INIT(0);
				Inst_Append(u->block, OP_LOAD, &val2);
				Inst *i = Inst_Append(u->block, OP_CALL, &val);
				i->argc = exp->argc;
			} else {
				assert(0);
			}
			return;
		}
	}

	// ident is current module's name
	if (!strcmp(id, ps->package)) {
		sym = ps->sym;
		assert(sym);
		debug("symbol '%s' is current module", id);
		sym->refcnt++;
		parser_expr_desc(ps, exp, sym);
		assert(exp->ctx == EXPR_LOAD);
		TValue val = INT_VALUE_INIT(0);
		Inst_Append(u->block, OP_LOAD, &val);
		Inst_Append(u->block, OP_GETM, NULL);
		return;
	}

	// find ident from external module
	sym = STable_Get(ps->extstbl, id);
	if (sym) {
		debug("symbol '%s' is found in external scope", id);
		assert(sym->kind == SYM_STABLE);
		sym->refcnt++;
		parser_expr_desc(ps, exp, sym);
		assert(exp->ctx == EXPR_LOAD);
		debug("symbol '%s' is module", sym->name);
		TValue val = CSTR_VALUE_INIT(sym->path);
		Inst_Append(u->block, OP_LOADM, &val);
		return;
	}

	error("cannot find symbol:%s", id);
}

static void parser_attribute(ParserState *ps, struct expr *exp)
{
	struct expr *left = exp->attribute.left;
	left->right = exp;
	left->ctx = EXPR_LOAD;
	parser_visit_expr(ps, left);

	if (exp->supername)
		debug(">>>(%s).%s", exp->supername, exp->attribute.id);
	else
		debug(">>>.%s", exp->attribute.id);

	Symbol *leftsym = left->sym;
	if (!leftsym) {
		//FIXME:
		error("cannot find '%s' in '%s'", exp->attribute.id, left->str);
		return;
	}

	Symbol *sym = NULL;
	if (leftsym->kind == SYM_STABLE) {
		debug(">>>>symbol '%s' is a module", leftsym->name);
		sym = STable_Get(leftsym->ptr, exp->attribute.id);
		if (!sym) {
			//FIXME:
			error("cannot find '%s' in '%s'", exp->attribute.id, left->str);
			exp->sym = NULL;
			return;
		}
		if (sym->kind == SYM_STABLE) {
			if (!sym->desc) {
				warn("symbol '%s' desc is null", sym->name);
				sym->desc = TypeDesc_From_UserDef(leftsym->path, sym->name);
			}
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
		assert(sym->kind == SYM_STABLE ||
			sym->kind == SYM_CLASS || sym->kind == SYM_INTF);
		char *typename = sym->name;
		sym = STable_Get(sym->ptr, exp->attribute.id);
		if (!sym) {
			error("cannot find '%s' in '%s'", exp->attribute.id, typename);
			return;
		}
	} else if (leftsym->kind == SYM_CLASS) {
		debug("symbol '%s' is a class", leftsym->name);
		sym = STable_Get(leftsym->ptr, exp->attribute.id);
		if (!sym) {
			if (!leftsym->super) {
				error("cannot find '%s' in '%s' class",
					exp->attribute.id, leftsym->name);
				return;
			}

			sym = STable_Get(leftsym->super->ptr, exp->attribute.id);
			if (!sym) {
				error("cannot find '%s' in super '%s' class",
					exp->attribute.id, leftsym->super->name);
				return;
			}

			debug("find '%s' in super '%s' class",
				exp->attribute.id, leftsym->super->name);
		} else {
			debug("find '%s' in '%s' class", exp->attribute.id, leftsym->name);
		}
	} else if (leftsym->kind == SYM_MODULE) {
		debug("symbol '%s' is a module", leftsym->name);
		char *typename = leftsym->name;
		sym = STable_Get(leftsym->ptr, exp->attribute.id);
		if (!sym) {
			error("cannot find '%s' in '%s'", exp->attribute.id, typename);
			return;
		}
	} else if (leftsym->kind == SYM_TRAIT) {
		debug("symbol '%s' is a trait", leftsym->name);
		char *typename = leftsym->name;
		sym = STable_Get(leftsym->ptr, exp->attribute.id);
		if (!sym) {
			error("cannot find '%s' in '%s'", exp->attribute.id, typename);
			return;
		}
	} else {
		assert(0);
	}

	// set expression's symbol
	exp->sym = sym;
	exp->desc = sym->desc;

	// generate code
	ParserUnit *u = ps->u;
	if (sym->kind == SYM_VAR) {
		TValue val;
		if (exp->supername) {
			char *namebuf = malloc(strlen(exp->supername) + 1 +
				strlen(exp->attribute.id) + 1);
			assert(namebuf);
			sprintf(namebuf, "%s.%s", exp->supername, exp->attribute.id);
			setcstrvalue(&val, namebuf);
		} else {
			setcstrvalue(&val, exp->attribute.id);
		}

		if (exp->ctx == EXPR_LOAD) {
			debug("load:%s", val.cstr);
			Inst_Append(ps->u->block, OP_GETFIELD, &val);
		} else {
			assert(exp->ctx == EXPR_STORE);
			debug("store:%s", val.cstr);
			Inst_Append(ps->u->block, OP_SETFIELD, &val);
		}
	} else if (sym->kind == SYM_PROTO) {
		TValue val;
		if (exp->supername) {
			char *namebuf = malloc(strlen(exp->supername) + 1 +
				strlen(exp->attribute.id) + 1);
			assert(namebuf);
			sprintf(namebuf, "%s.%s", exp->supername, exp->attribute.id);
			setcstrvalue(&val, namebuf);
		} else {
			setcstrvalue(&val, exp->attribute.id);
		}
		Inst *i = Inst_Append(u->block, OP_CALL, &val);
		i->argc = exp->argc;
	} else if (sym->kind == SYM_STABLE) {
		// new object
		TValue val;
		setcstrvalue(&val, sym->name);
		Inst *i = Inst_Append(u->block, OP_NEW, &val);
		i->argc = exp->argc;
	} else if (sym->kind == SYM_IPROTO) {
		TValue val = CSTR_VALUE_INIT(sym->name);
		Inst *i = Inst_Append(u->block, OP_CALL, &val);
		i->argc = exp->argc;
	} else {
		assert(0);
	}
}

static void parser_call(ParserState *ps, struct expr *exp)
{
	int argc = 0;

	//check super()
	if (exp->call.left->kind == SUPER_KIND) {
		assert(ps->u->scope == SCOPE_METHOD);
		assert(!strcmp(ps->u->sym->name, "__init__"));
		assert(list_empty(&ps->u->block->insts));
	}

	if (exp->call.args) {
		struct expr *e;
		Vector_ForEach_Reverse(e, exp->call.args) {
			e->ctx = EXPR_LOAD;
			parser_visit_expr(ps, e);
		}
		argc = Vector_Size(exp->call.args);
	}

	struct expr *left = exp->call.left;
	left->right = exp;
	left->ctx = EXPR_LOAD;
	left->argc = argc;
	parser_visit_expr(ps, left);
	if (!left->sym) {
		error("left symbol is not found in call");
		return;
	}

	Symbol *sym = left->sym;
	assert(sym);
	if (sym->kind == SYM_PROTO) {
		debug("call %s()", sym->name);
		/* function type */
		exp->sym = sym;
		exp->desc = sym->desc;
		/* check arguments */
		if (!check_call_args(exp->desc->proto, exp->call.args)) {
			error("arguments are not matched.");
			return;
		}
	} else if (sym->kind == SYM_CLASS || sym->kind == SYM_STABLE) {
		debug("class '%s'", sym->name);
		/* class type */
		exp->sym = sym;
		exp->desc = sym->desc;
		/* get __init__ function */
		Symbol *__init__ = STable_Get(sym->ptr, "__init__");
		if (__init__) {
			Proto *proto = __init__->desc->proto;
			/* check arguments and returns(no returns) */
			if (!check_call_args(proto, exp->call.args)) {
				error("Arguments of __init__ are not matched.");
				return;
			}
			/* check no returns */
			if (proto->rsz) {
				error("Returns of __init__ must be 0");
				return;
			}
		} else {
			debug("'%s' class is not defined __init__", sym->name);
			if (argc) {
				error("There is no __init__, but pass %d arguments", argc);
				return;
			}
		}
	} else if (sym->kind == SYM_IPROTO) {
		debug("call abstract method %s()", sym->name);
		/* function type */
		exp->sym = sym;
		exp->desc = sym->desc;
		/* check arguments */
		if (!check_call_args(exp->desc->proto, exp->call.args)) {
			error("abstract method arguments are not matched.");
			return;
		}
	} else {
		assert(0);
	}
}

static void parser_with(ParserState *ps, struct expr *exp)
{
	debug("with %s.%s", exp->with.desc->path, exp->with.desc->type);
	Symbol *sym = find_userdef_symbol(ps, exp->with.desc);
	assert(sym);
	exp->sym = sym;
	exp->desc = sym->desc;

	struct expr *left = exp->with.left;
	left->right = exp;
	left->ctx = EXPR_LOAD;
	parser_visit_expr(ps, left);

	Symbol *leftsym = left->sym;
	if (!leftsym) {
		error("cannot find '%s' in '%s'", exp->with.desc->type, left->str);
		return;
	}
}

static ParserUnit *get_class_unit(ParserState *ps)
{
	if (ps->u->scope != SCOPE_METHOD) return NULL;
	ParserUnit *u;
	list_for_each_entry(u, &ps->ustack, link) {
		if (u->scope == SCOPE_CLASS) return u;
	}
	return NULL;
}

static void parser_super(ParserState *ps, struct expr *exp)
{
	ParserUnit *u = ps->u;
	ParserUnit *cu = get_class_unit(ps);
	assert(cu->sym->super);
	exp->sym = cu->sym->super;
	Symbol *super = cu->sym->super;

	struct expr *r = exp->right;
	r->supername = super->name;
	if (r->kind == CALL_KIND) {
		assert(u->scope == SCOPE_METHOD);
		assert(!strcmp(u->sym->name, "__init__"));
		char *namebuf = malloc(strlen(super->name) + 1 + strlen(u->sym->name) + 1);
		assert(namebuf);
		sprintf(namebuf, "%s.%s", super->name, u->sym->name);
		TValue val = INT_VALUE_INIT(0);
		Inst_Append(u->block, OP_LOAD, &val);
		setcstrvalue(&val, namebuf);
		Inst *i = Inst_Append(u->block, OP_CALL, &val);
		i->argc = exp->argc;
	} else if (r->kind == ATTRIBUTE_KIND) {
		TValue val = INT_VALUE_INIT(0);
		Inst_Append(ps->u->block, OP_LOAD, &val);
	} else {
		assert(0);
	}
}

/*
static void parser_binary(ParserState *ps, struct expr *exp)
{
	debug("binary_op:%d", exp->binary.op);
	exp->binary.right->ctx = EXPR_LOAD;
	parser_visit_expr(ps, exp->binary.right);
	exp->binary.left->ctx = EXPR_LOAD;
	parser_visit_expr(ps, exp->binary.left);
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
*/

static void parser_visit_expr(ParserState *ps, struct expr *exp)
{
	switch (exp->kind) {
		case ID_KIND: {
			parser_ident(ps, exp);
			break;
		}
		case INT_KIND: {
			assert(exp->ctx == EXPR_LOAD);
			TValue val = INT_VALUE_INIT(exp->ival);
			Inst_Append(ps->u->block, OP_LOADK, &val);
			break;
		}
		case FLOAT_KIND: {
			assert(exp->ctx == EXPR_LOAD);
			break;
		}
		case BOOL_KIND: {
			assert(exp->ctx == EXPR_LOAD);
			break;
		}
		case STRING_KIND: {
			assert(exp->ctx == EXPR_LOAD);
			TValue val = CSTR_VALUE_INIT(exp->str);
			Inst_Append(ps->u->block, OP_LOADK, &val);
			break;
		}
		case SELF_KIND: {
			assert(exp->ctx == EXPR_LOAD);
			debug("load self");
			ParserUnit *cu = get_class_unit(ps);
			exp->sym = cu->sym;
			TValue val = INT_VALUE_INIT(0);
			Inst_Append(ps->u->block, OP_LOAD, &val);
			break;
		}
		case SUPER_KIND: {
			assert(exp->ctx == EXPR_LOAD);
			debug("load super");
			parser_super(ps, exp);
			break;
		}
		case TYPEOF_KIND: {
			assert(exp->ctx == EXPR_LOAD);
			debug("typeof");
			assert(exp->right->kind == CALL_KIND);
			Symbol *sym = find_external_package(ps, "koala/lang");
			assert(sym);
			sym = STable_Get(sym->ptr, "TypeOf");
			assert(sym);
			exp->sym = sym;

			TValue val = CSTR_VALUE_INIT("koala/lang");
			Inst_Append(ps->u->block, OP_LOADM, &val);
			setcstrvalue(&val, "TypeOf");
			Inst *i = Inst_Append(ps->u->block, OP_CALL, &val);
			i->argc = exp->argc;
			break;
		}
		case ATTRIBUTE_KIND: {
			parser_attribute(ps, exp);
			break;
		}
		case CALL_KIND: {
			parser_call(ps, exp);
			break;
		}
		case WITH_KIND: {
			parser_with(ps, exp);
			break;
		}
		case BINARY_KIND: {
			debug("binary_op:%d", exp->binary.op);
			exp->binary.right->ctx = EXPR_LOAD;
			parser_visit_expr(ps, exp->binary.right);
			exp->binary.left->ctx = EXPR_LOAD;
			parser_visit_expr(ps, exp->binary.left);
			if (binop_relation(exp->binary.op)) {
				exp->desc = TypeDesc_From_Primitive(PRIMITIVE_BOOL);
			} else {
				exp->desc = exp->binary.left->desc;
			}
			codegen_binary(ps, exp->binary.op);
			break;
		}
		default: {
			assert(0);
			break;
		}
	}
}

/*--------------------------------------------------------------------------*/

static ParserUnit *get_function_unit(ParserState *ps)
{
	ParserUnit *u;
	list_for_each_entry(u, &ps->ustack, link) {
		if (u->scope == SCOPE_FUNCTION || u->scope == SCOPE_METHOD) return u;
	}
	return NULL;
}

static int __intf_cls_check(Symbol *intf, Symbol *cls)
{
	int result = -1;
	Symbol *method;
	Symbol *base = cls;
	while (base) {
		method = STable_Get(base->ptr, intf->name);
		if (!method) {
			if (!base->super) {
				error("Is '%s' in '%s'? no", intf->name, base->name);
				result = -1;
				break;
			} else {
				debug("go super class to check '%s' is in '%s'",
					intf->name, base->name);
			}
		} else {
			debug("Is '%s' in '%s'? yes", intf->name, base->name);
			if (!Proto_IsEqual(method->desc->proto, intf->desc->proto)) {
				error("abstract func check failed");
				result = -1;
			} else {
				debug("abstract func check ok");
				result = 0;
			}
			break;
		}
		base = base->super;
	}
	return result;
}

struct intf_inherit_struct {
	Symbol *sym;
	int result;
};

static void __intf_func_fn(Symbol *sym, void *arg)
{
	struct intf_inherit_struct *temp = arg;
	if (temp->result) return;

	Symbol *sub = temp->sym;
	assert(sym->kind == SYM_IPROTO);
	assert(sub->kind == SYM_CLASS);
	temp->result = __intf_cls_check(sym, sub);
}

static int check_intf_cls_inherit(Symbol *base, Symbol *sub)
{
	struct intf_inherit_struct temp = {sub, 0};
	STable_Traverse(base->ptr, __intf_func_fn, &temp);
	return temp.result;
}

static int check_intf_intf_inherit(Symbol *base, Symbol *sub)
{
	if (!sub->table) return -1;
	BaseSymbol basesym = {.sym = base};
	BaseSymbol *temp = HashTable_Find(sub->table, &basesym);
	return temp ? 0 : -1;
}

static int check_symbol_inherit(Symbol *base, Symbol *sub)
{
	Symbol *up = sub->super;
	while (up) {
		if (up == base) return 0;
		up = up->super;
	}
	return -1;
}

static void parser_variable(ParserState *ps, struct var *var, struct expr *exp)
{
	ParserUnit *u = ps->u;

	debug("parse variable '%s' declaration", var->id);

	if (exp) {
		exp->ctx = EXPR_LOAD;
		parser_visit_expr(ps, exp);
		if (!exp->desc) {
			error("cannot resolve var '%s' type", var->id);
			return;
		}

		if (exp->desc->kind == TYPE_PROTO) {
			Proto *proto = exp->desc->proto;
			if (proto->rsz != 1) {
				//FIXME
				error("function's returns is not 1:%d", proto->rsz);
				return;
			}

			if (!var->desc) {
				var->desc = proto->rdesc;
			}

			if (!TypeDesc_Check(var->desc, proto->rdesc)) {
				error("proto typecheck failed");
				return;
			}
		} else {
			if (!var->desc) {
				var->desc = exp->desc;
			}

			if (!TypeDesc_Check(var->desc, exp->desc)) {
				//literal check failed
				if (var->desc->kind == TYPE_USERDEF &&
					exp->desc->kind == TYPE_USERDEF) {
					Symbol *s1 = find_userdef_symbol(ps, var->desc);
					Symbol *s2 = find_userdef_symbol(ps, exp->desc);
					if (s1->kind == SYM_INTF) {
						if (s2->kind == SYM_CLASS) {
							if (check_intf_cls_inherit(s1, s2)) {
								error("check intf <- class failed");
								return;
							}
						} else if (s2->kind == SYM_INTF) {
							if (check_intf_intf_inherit(s1, s2)) {
								error("check intf <- intf failed");
								return;
							}
						} else {
							assertm(0, "invalid:%d", s2->kind);
						}
					} else {
						if (check_symbol_inherit(s1, s2)) {
							error("check_symbol_inherit(class <- class) failed");
							return;
						}
					}
				} else {
					error("typecheck failed");
					return;
				}
			}
		}
	}

	Symbol *sym;
	if (u->scope == SCOPE_MODULE || u->scope == SCOPE_CLASS) {
		debug("var '%s' decl in module or class", var->id);
		sym = STable_Get(u->stbl, var->id);
		assert(sym);
		if (sym->kind == SYM_VAR && !sym->desc) {
			debug("update symbol '%s' type", var->id);
			STable_Update_Symbol(u->stbl, sym, var->desc);
		}
	} else if (u->scope == SCOPE_FUNCTION || u->scope == SCOPE_METHOD) {
		debug("var '%s' decl in function", var->id);
		sym = STable_Add_Var(u->stbl, var->id, var->desc, var->bconst);
		sym->up = u->sym;
		Vector_Append(&u->sym->locvec, sym);
	} else if (u->scope == SCOPE_BLOCK) {
		debug("var '%s' decl in block", var->id);
		ParserUnit *pu = get_function_unit(ps);
		assert(pu);
		sym = STable_Get(pu->stbl, var->id);
		if (sym) {
			error("var '%s' is already defined in this scope", var->id);
			return;
		}
		//FIXME: need add to function's symbol table? how to handle different block has the same variable
		sym = STable_Add_Var(u->stbl, var->id, var->desc, var->bconst);
		assert(sym);
		sym->index = pu->stbl->varcnt++;
		sym->up = NULL;
		Vector_Append(&pu->sym->locvec, sym);
	} else {
		assertm(0, "unknown unit scope:%d", u->scope);
	}

	//generate code
	if (exp) {
		if (u->scope == SCOPE_MODULE || u->scope == SCOPE_CLASS) {
			// module's or class's variable
			TValue val = INT_VALUE_INIT(0);
			Inst_Append(ps->u->block, OP_LOAD, &val);
			setcstrvalue(&val, var->id);
			Inst_Append(ps->u->block, OP_SETFIELD, &val);
		} else if (u->scope == SCOPE_FUNCTION || u->scope == SCOPE_METHOD) {
			// local's variable
			TValue val = INT_VALUE_INIT(sym->index);
			Inst_Append(ps->u->block, OP_STORE, &val);
		} else if (u->scope == SCOPE_BLOCK) {
			// local's variable in block
			TValue val = INT_VALUE_INIT(sym->index);
			Inst_Append(ps->u->block, OP_STORE, &val);
		} else {
			assert(0);
		}
	}
}

static void parser_function(ParserState *ps, struct stmt *stmt, int scope)
{
	parser_enter_scope(ps, NULL, scope);

	ParserUnit *parent = parent_scope(ps);
	Symbol *sym = STable_Get(parent->stbl, stmt->funcdecl.id);
	assert(sym);
	ps->u->sym = sym;

	if (parent->scope == SCOPE_MODULE || parent->scope == SCOPE_CLASS) {
		debug("------parse function '%s'--------", stmt->funcdecl.id);
		if (stmt->funcdecl.pvec) {
			struct var *var;
			Vector_ForEach(var, stmt->funcdecl.pvec)
				parser_variable(ps, var, NULL);
		}
		parser_body(ps, stmt->funcdecl.body);
		debug("------parse function '%s' end----", stmt->funcdecl.id);
	} else {
		assertm(0, "unknown parent scope: %d", parent->scope);
	}

	parser_exit_scope(ps);
}

void sym_inherit_fn(Symbol *sym, void *arg)
{
	Symbol *subsym = arg;
	Symbol *supersym = subsym->super;
	STable *stbl = subsym->ptr;
	Symbol *s;

	char namebuf[strlen(supersym->name) + 1 + strlen(sym->name) + 1];
	sprintf(namebuf, "%s.%s", supersym->name, sym->name);

	if (STable_Get(stbl, sym->name)) {
		debug("symbol '%s' exist in subclass '%s'", sym->name, subsym->name);
	}

	if (sym->kind == SYM_VAR) {
		debug("inherit variable '%s' from super class", namebuf);
		s = STable_Add_Var(stbl, namebuf, sym->desc, sym->access & ACCESS_CONST);
		if (s) {
			s->up = subsym;
			s->inherited = 1;
			s->super = sym;
		}
	} else if (sym->kind == SYM_PROTO) {
		debug("inherit function '%s' from super class", namebuf);
		s = STable_Add_Proto(stbl, namebuf, sym->desc->proto);
		if (s) {
			s->up = subsym;
			s->inherited = 1;
			s->super = sym;
		}
	} else {
		assertm(0, "invalid symbol kind:%d", sym->kind);
	}
}

void parser_class_inheritance(Symbol *sym)
{
	STable_Traverse(sym->super->ptr, sym_inherit_fn, sym);
}

static void parser_class(ParserState *ps, struct stmt *stmt)
{
	debug("------parse class or trait -------");
	debug("class(trait):%s", stmt->class_type.id);

	ParserUnit *u = ps->u;
	Symbol *sym = STable_Get(u->stbl, stmt->class_type.id);
	assert(sym && sym->ptr);

	if (stmt->class_type.parent) {
		TypeDesc *desc;
		Symbol *super;
		Vector_ForEach(desc, stmt->class_type.parent) {
			//FIXME
			char *typestr = TypeDesc_ToString(desc);
			debug("class or trait with super '%s'", typestr);
			super = find_userdef_symbol(ps, desc);
			if (!super) {
				error("cannot find super '%s'", typestr);
				free(typestr);
				return;
			}
			free(typestr);
			assert(super->ptr);
			Vector_Append(&sym->supers, super);
		}
	}

	parser_enter_scope(ps, sym->ptr, SCOPE_CLASS);
	ps->u->sym = sym;

	struct stmt *s;
	Vector_ForEach(s, stmt->class_type.body) {
		if (s->kind == VARDECL_KIND) {
			parser_variable(ps, s->vardecl.var, s->vardecl.exp);
		} else if (s->kind == FUNCDECL_KIND) {
			parser_function(ps, s, SCOPE_METHOD);
		} else if (s->kind == FUNCPROTO_KIND) {
			debug("func %s is abstract", s->funcproto.id);
		} else {
			assertm(0, "invalid symbol kind:%d", s->kind);
		}
	}

	parser_exit_scope(ps);

	debug("------parse class or trait end---");
}

// static void __inherit_imethod_fn(Symbol *sym, void *arg)
// {
// 	struct intf_inherit_struct *temp = arg;
// 	if (temp->result) return;
// 	Symbol *intf = temp->sym;
// 	assert(sym->kind == SYM_IPROTO);
// 	assert(intf->kind == SYM_INTF);
// 	Symbol *imeth = STable_Add_IProto(intf->ptr, sym->name, sym->desc->proto);
// 	if (!imeth)
// 		debug("The same imethod '%s' in '%s.'", sym->name, intf->name);
// 	else
// 		debug("Inherit imethod '%s' to '%s' successfully.", sym->name, intf->name);
// 	temp->result = 0;
// }

// static int inherit_imethods(Symbol *sym, Symbol *basesym)
// {
// 	struct intf_inherit_struct temp = {sym, 0};
// 	STable_Traverse(basesym->ptr, __inherit_imethod_fn, &temp);
// 	return temp.result;
// }

// static void __inherit_intf_fn(HashNode *hnode, void *arg)
// {
// 	Symbol *sym = arg;
// 	BaseSymbol *basesym = container_of(hnode, BaseSymbol, hnode);
// 	if (Symbol_Add_Base(sym, basesym->sym)) {
// 		debug("The same intf '%s' in '%s'.", basesym->sym->name, sym->name);
// 	} else {
// 		debug("Inherit intf '%s' to '%s' successfully.",
// 			basesym->sym->name, sym->name);
// 	}
// }

// static int inherit_interfaces(Symbol *sym, Symbol *basesym)
// {
// 	assert(sym->kind == SYM_INTF);
// 	assert(basesym->kind == SYM_INTF);
// 	Symbol_Add_Base(sym, basesym);
// 	HashTable_Traverse(basesym->table, __inherit_intf_fn, sym);
// 	return 0;
// }

// static void parser_interface(ParserState *ps, struct stmt *stmt)
// {
// 	debug("------parse interface-------");
// 	debug("interface:%s", stmt->intf_type.id);

// 	ParserUnit *u = ps->u;
// 	Symbol *sym = STable_Get(u->stbl, stmt->intf_type.id);
// 	assert(sym && sym->ptr);

// 	if (!stmt->intf_type.bases) {
// 		debug("------parse interface end---");
// 		return;
// 	}

// 	TypeDesc *desc;
// 	Symbol *basesym;
// 	char *typestr;
// 	Vector_ForEach(desc, stmt->intf_type.bases) {
// 		typestr = TypeDesc_ToString(desc);
// 		debug("interface with base '%s'", typestr);
// 		basesym = find_userdef_symbol(ps, desc);
// 		if (!basesym) {
// 			error("cannot find base interface '%s'", typestr);
// 			free(typestr);
// 			return;
// 		}

// 		if (basesym->kind != SYM_INTF) {
// 			error("base '%s' is not an interface", typestr);
// 			free(typestr);
// 			return;
// 		}

// 		free(typestr);

// 		inherit_imethods(sym, basesym);
// 		inherit_interfaces(sym, basesym);
// 	}

// 	debug("------parse interface end---");
// }

static void parser_assign(ParserState *ps, struct stmt *stmt)
{
	struct expr *r = stmt->assign.right;
	struct expr *l = stmt->assign.left;
	//check_assignment(l, r);
	r->ctx = EXPR_LOAD;
	parser_visit_expr(ps, r);
	l->ctx = EXPR_STORE;
	parser_visit_expr(ps, l);
}

static void parser_return(ParserState *ps, struct stmt *stmt)
{
	ParserUnit *u = ps->u;
	Symbol *sym = NULL;
	if (u->scope == SCOPE_FUNCTION || u->scope == SCOPE_METHOD) {
		debug("return in function");
		sym = u->sym;
	} else if (u->scope == SCOPE_BLOCK) {
		debug("return in block");
		ParserUnit *parent = get_function_unit(ps);
		assert(parent);
		sym = parent->sym;
	} else {
		assertm(0, "invalid scope:%d", u->scope);
	}

	if (stmt->vec) {
		struct expr *e;
		Vector_ForEach(e, stmt->vec) {
			e->ctx = EXPR_LOAD;
			parser_visit_expr(ps, e);
		}
	}
	check_return_types(sym, stmt->vec);

	Inst_Append(u->block, OP_RET, NULL);
	u->block->bret = 1;
}

static void parser_if(ParserState *ps, struct stmt *stmt)
{
	parser_enter_scope(ps, NULL, SCOPE_BLOCK);
	int testsize = 0;
	struct expr *test = stmt->if_stmt.test;
	if (test) {
		// ELSE branch has not 'test'
		parser_visit_expr(ps, test);
		assert(test->desc);
		if (!TypeDesc_IsBool(test->desc)) {
			error("if-stmt condition is not bool");
			return;
		}
		testsize = ps->u->block->bytes;
	}

	CodeBlock *b = ps->u->block;
	Inst *jumpfalse = NULL;
	if (test) {
		jumpfalse = Inst_Append(b, OP_JUMP_FALSE, NULL);
	}

	parser_body(ps, stmt->if_stmt.body);

	if (test) {
		// ELSE branch has not 'test'
		int offset = b->bytes;
		debug("offset:%d", offset);
		if (stmt->if_stmt.orelse) {
			offset -= testsize;
			assert(offset >= 0);
			debug("offset2:%d", offset);
		} else {
			offset -= testsize + 1 + opcode_argsize(OP_JUMP_FALSE);
			debug("offset3:%d", offset);
			assert(offset >= 0);
		}
		TValue val = INT_VALUE_INIT(offset);
		jumpfalse->arg = val;
	}

	if (stmt->if_stmt.orelse) {
		parser_if(ps, stmt->if_stmt.orelse);
		assert(b->next);
		CodeBlock *nb = b->next;
		int offset = 0;
		while (nb) {
			offset += nb->bytes;
			nb = nb->next;
		}
		TValue val = INT_VALUE_INIT(offset);
		Inst_Append(b, OP_JUMP, &val);
	}

	if (stmt->if_stmt.belse)
		ps->u->merge = 0;
	else
		ps->u->merge = 1;

	parser_exit_scope(ps);
}

static void parser_while(ParserState *ps, struct stmt *stmt)
{
	parser_enter_scope(ps, NULL, SCOPE_BLOCK);
	ParserUnit *u = ps->u;
	u->loop = 1;
	CodeBlock *b = u->block;
	Inst *jmp;
	int jmpsize = 0;
	int bodysize = 0;

	if (stmt->while_stmt.btest) {
		jmp = Inst_Append(u->block, OP_JUMP, NULL);
		jmpsize = 1 + opcode_argsize(OP_JUMP);
	}

	parser_body(ps, stmt->while_stmt.body);
	bodysize = u->block->bytes - jmpsize;

	struct expr *test = stmt->while_stmt.test;
	parser_visit_expr(ps, test);
	assert(test->desc);
	if (!TypeDesc_IsBool(test->desc)) {
		error("while-stmt condition is not bool");
	}

	int offset = 0 - (u->block->bytes - jmpsize +
		1 + opcode_argsize(OP_JUMP_TRUE));
	TValue val = INT_VALUE_INIT(offset);
	Inst_Append(b, OP_JUMP_TRUE, &val);

	if (stmt->while_stmt.btest) {
		TValue val = INT_VALUE_INIT(bodysize);
		jmp->arg = val;
	}

	parser_exit_scope(ps);
}

static void parser_break(ParserState *ps, struct stmt *stmt)
{
	UNUSED_PARAMETER(stmt);

	ParserUnit *u = ps->u;
	if (u->scope != SCOPE_BLOCK) {
		error("break statement not within a loop or switch");
		return;
	}

	CodeBlock *b = u->block;
	// current block is a loop
	if (u->loop) {
		Inst *i = Inst_Append(b, OP_JUMP, NULL);
		JmpInst *jmp = JmpInst_New(i, JMP_BREAK);
		Vector_Append(&u->jmps, jmp);
	} else {
		ParserUnit *parent;
		list_for_each_entry(parent, &ps->ustack, link) {
			if (parent->scope != SCOPE_BLOCK) {
				error("break statement is not within a block");
				break;
			}

			if (parent->loop) {
				Inst *i = Inst_Append(b, OP_JUMP, NULL);
				//FIXME:
				JmpInst *jmp = JmpInst_New(i, JMP_BREAK);
				Vector_Append(&parent->jmps, jmp);
				return;
			}
		}

		error("break statement not within a loop or switch");
	}
}

static void parser_continue(ParserState *ps, struct stmt *stmt)
{
	UNUSED_PARAMETER(stmt);

	ParserUnit *u = ps->u;
	if (u->scope != SCOPE_BLOCK) {
		error("continue statement not within a loop");
		return;
	}

	CodeBlock *b = u->block;
	// current block is a loop
	if (u->loop) {
		Inst *i = Inst_Append(b, OP_JUMP, NULL);
		JmpInst *jmp = JmpInst_New(i, JMP_CONTINUE);
		Vector_Append(&u->jmps, jmp);
	} else {
		ParserUnit *parent;
		list_for_each_entry(parent, &ps->ustack, link) {
			if (parent->scope != SCOPE_BLOCK) {
				error("break statement is not within a block");
				break;
			}

			if (parent->loop) {
				Inst *i = Inst_Append(b, OP_JUMP, NULL);
				//FIXME:
				JmpInst *jmp = JmpInst_New(i, JMP_CONTINUE);
				Vector_Append(&parent->jmps, jmp);
				return;
			}
		}

		error("continue statement not within a loop");
	}
}

static void parser_vist_stmt(ParserState *ps, struct stmt *stmt)
{
	switch (stmt->kind) {
		case VARDECL_KIND: {
			struct var *var = stmt->vardecl.var;
			struct expr *exp = stmt->vardecl.exp;
			parser_variable(ps, var, exp);
			break;
		}
		case FUNCDECL_KIND: {
			parser_function(ps, stmt, SCOPE_FUNCTION);
			break;
		}
		case CLASS_KIND:
		case TRAIT_KIND: {
			parser_class(ps, stmt);
			break;
		}
		case EXPR_KIND: {
			parser_visit_expr(ps, stmt->exp);
			break;
		}
		case ASSIGN_KIND: {
			parser_assign(ps, stmt);
			break;
		}
		case RETURN_KIND: {
			parser_return(ps, stmt);
			break;
		}
		case IF_KIND: {
			parser_if(ps, stmt);
			break;
		}
		case WHILE_KIND: {
			parser_while(ps, stmt);
			break;
		}
		case BREAK_KIND: {
			parser_break(ps, stmt);
			break;
		}
		case CONTINUE_KIND: {
			parser_continue(ps, stmt);
			break;
		}
		case VARDECL_LIST_KIND: {
			assert(0);
			break;
		}
		default: {
			assert(0);
			break;
		}
	}
}

static void parser_body(ParserState *ps, Vector *stmts)
{
	debug("------parse body-------");
	if (stmts) {
		struct stmt *s;
		Vector_ForEach(s, stmts) {
			parser_vist_stmt(ps, s);
		}
	}
	debug("------parse body end---");
}

/*--------------------------------------------------------------------------*/

void init_parser(ParserState *ps)
{
	memset(ps, 0, sizeof(ParserState));
	Vector_Init(&ps->stmts);
	init_imports(ps);
	Symbol *sym = Symbol_New(SYM_MODULE);
	sym->ptr = STable_New(NULL);
	ps->sym = sym;
	init_list_head(&ps->ustack);
	Vector_Init(&ps->errors);
}

void fini_parser(ParserState *ps)
{
	vec_stmt_fini(&ps->stmts);
	fini_imports(ps);
	Vector_Fini(&ps->errors, NULL, NULL);
	free(ps->package);
}

void parser_module(ParserState *ps, FILE *in)
{
	extern FILE *yyin;
	extern int yyparse(ParserState *ps);

	parser_enter_scope(ps, ps->sym->ptr, SCOPE_MODULE);
	ps->u->sym = ps->sym;

	yyin = in;
	yyparse(ps);
	fclose(yyin);

	parser_body(ps, &ps->stmts);

	check_unused_imports(ps);

	parser_exit_scope(ps);
}

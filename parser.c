
#include "parser.h"
#include "codegen.h"
#include "kstate.h"
#include "checker.h"
#include "codegen.h"
#include "opcode.h"
#include "moduleobject.h"
#include "log.h"
#include "koala_yacc.h"
#include "koala_lex.h"

int Lexer_DoYYInput(ParserState *ps, char *buf, int size, FILE *in)
{
	LineBuffer *lb = &ps->line;

	if (lb->lineleft <= 0) {
		if (!fgets(lb->line, LINE_MAX_LEN, in)) {
			if (ferror(in)) clearerr(in);
			return 0;
		} else {
			lb->lineleft = lb->linelen = strlen(lb->line);
			lb->row++;
			lb->lastlen = 0;
			lb->col = 0;
			lb->print = 0;
		}
	}

	int sz = min(lb->lineleft, size);
	memcpy(buf, lb->line, sz);
	lb->lineleft -= sz;
	return sz;
}

void Lexer_DoUserAction(ParserState *ps, char *text)
{
	LineBuffer *lb = &ps->line;
	lb->col += lb->lastlen;
	strncpy(lb->lasttoken, text, TOKEN_MAX_LEN);
	lb->lastlen = strlen(text);
}

void Parser_SetLine(ParserState *ps, struct expr *exp)
{
	if (exp) {
		Line *l = &exp->line;
		LineBuffer *lb = &ps->line;
		l->line = strdup(lb->line);
		l->row = lb->row;
		l->col = lb->col;
	}
}

void Parser_PrintError(ParserState *ps, Line *l, char *fmt, ...)
{
	if (++ps->errnum >= MAX_ERRORS) {
		exit(-1);
	}

	fprintf(stderr, "%s:%d:%d: ", ps->filename, l->row, l->col);

	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	fprintf(stderr, "\n%s", l->line);
	char ch;
	for (int i = 0; i < l->col - 1; i++) {
		ch = l->line[i];
		if (!isspace(ch)) putchar(' '); else putchar(ch);
	}
	puts("^");
}

/*---------------------------------------------------------------------------*/

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

static void arg_print(char *buf, int sz, Argument *val)
{
	if (!val) {
		buf[0] = '\0';
		return;
	}

	switch (val->kind) {
		case ARG_NIL: {
			snprintf(buf, sz, "(nil)");
			break;
		}
		case ARG_INT: {
			snprintf(buf, sz, "%lld", val->ival);
			break;
		}
		case ARG_FLOAT: {
			snprintf(buf, sz, "%.16lf", val->fval);
			break;
		}
		case ARG_BOOL: {
			snprintf(buf, sz, "%s", val->bval ? "true" : "false");
			break;
		}
		case ARG_STR: {
			snprintf(buf, sz, "%s", val->str);
			break;
		}
		default: {
			assert(0);
			break;
		}
	}
}

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
			arg_print(buf, sizeof(buf), &i->arg);
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
static void parser_vist_stmt(ParserState *ps, struct stmt *stmt);

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
		debug("type '%s' in current module", desc->type);
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

static Symbol *find_symbol_linear_order(Symbol *clssym, char *name)
{
	Symbol *sym;
	debug(">>>>find_symbol_linear_order<<<<");
	//find in current class
	debug("finding '%s' in '%s'", name, clssym->name);
	sym = STable_Get(clssym->ptr, name);
	if (sym) {
		debug("'%s' is found in '%s'", name, clssym->name);
		return sym;
	}
	debug("'%s' is not in '%s'", name, clssym->name);

	//find in linear-order traits
	Symbol *s;
	Vector_ForEach_Reverse(s, &clssym->traits) {
		sym = find_symbol_linear_order(s, name);
		if (sym) {
			return sym;
		}
	}

	// //find in super class
	// if (clssym->super)
	// 	return find_symbol_linear_order(clssym->super, name);
	// else
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

struct path_stbl_struct {
	char *path;
	STable *stbl;
};

static void __to_stbl_fn(HashNode *hnode, void *arg)
{
	MemberDef *member = container_of(hnode, MemberDef, hnode);
	struct path_stbl_struct *path_struct = arg;
	STable *stbl = path_struct->stbl;
	char *path = path_struct->path;

	if (member->kind == MEMBER_CLASS || member->kind == MEMBER_TRAIT) {
		Symbol *s = STable_Add_Symbol(stbl, member->name, SYM_STABLE, 0);
		s->desc = TypeDesc_From_UserDef(strdup(path), member->name);
		s->ptr = STable_New(stbl->atbl);
		struct path_stbl_struct tmp = {path, s->ptr};
		HashTable_Traverse(member->klazz->table, __to_stbl_fn, &tmp);
	} else if (member->kind == MEMBER_VAR) {
		STable_Add_Var(stbl, member->name, member->desc, member->bconst);
	} else if (member->kind == MEMBER_CODE) {
		//FIXME
		STable_Add_Proto(stbl, member->name, member->desc);
	} else if (member->kind == MEMBER_PROTO) {
		//FIXME
		STable_Add_IProto(stbl, member->name, member->desc);
	} else {
		assert(0);
	}
}

/* for compiler only */
STable *Module_To_STable(Object *ob, AtomTable *atbl, char *path)
{
	ModuleObject *m = OBJ_TO_MOD(ob);
	STable *stbl = STable_New(atbl);
	struct path_stbl_struct path_struct = {path, stbl};
	HashTable_Traverse(m->table, __to_stbl_fn, &path_struct);
	return stbl;
}

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
	Line *l = &import->line;
	LineBuffer *lb = &ps->line;
	l->line = strdup(lb->line);
	l->row = lb->row;
	l->col = 1;
	sym->import = import;
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

static void parse_funcdecl(ParserState *ps, struct stmt *stmt)
{
	TypeDesc *proto;
	Symbol *sym;
	Vector *pdesc = NULL;
	if (stmt->funcdecl.pvec) {
		pdesc = Vector_New();
		struct var *var;
		Vector_ForEach(var, stmt->funcdecl.pvec) {
			Vector_Append(pdesc, var->desc);
		}
	}
	proto = TypeDesc_From_Proto(stmt->funcdecl.rvec, pdesc);
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
	if (stmt) {
		__add_stmt(ps, stmt);
		parse_funcdecl(ps, stmt);
	}
}

static void parse_funcproto(ParserState *ps, struct stmt *stmt)
{
	TypeDesc *proto;
	Symbol *sym;
	proto = TypeDesc_From_Proto(stmt->funcproto.rvec, stmt->funcproto.pvec);
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
		sym = STable_Add_Class(ps->u->stbl, stmt->class_info.id);
	} else if (stmt->kind == TRAIT_KIND) {
		sym = STable_Add_Trait(ps->u->stbl, stmt->class_info.id);
	} else {
		assert(0);
	}

	sym->up = ps->u->sym;
	sym->ptr = STable_New(ps->u->stbl->atbl);
	sym->desc = TypeDesc_From_UserDef(NULL, stmt->class_info.id);
	debug(">>>>add class(trait) '%s' successfully", sym->name);

	parser_enter_scope(ps, sym->ptr, SCOPE_CLASS);
	ps->u->sym = sym;
	if (stmt->class_info.body) {
		struct stmt *s;
		Vector_ForEach(s, stmt->class_info.body) {
			if (s->kind == VARDECL_KIND) {
				parse_vardecl(ps, s);
			} else if (s->kind == FUNCDECL_KIND) {
				parse_funcdecl(ps, s);
			} else {
				assert(s->kind == FUNCPROTO_KIND);
				parse_funcproto(ps, s);
			}
		}
	}
	parser_exit_scope(ps);

	debug(">>>>end class(trait) '%s'", sym->name);
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

int check_npr_func(Symbol *sym)
{
	TypeDesc *proto = sym->desc;
	int rsz = Vector_Size(proto->rdesc);
	int psz = Vector_Size(proto->pdesc);
	return (rsz == 0 && psz == 0) ? 0 : -1;
}

static void parser_merge(ParserState *ps)
{
	ParserUnit *u = ps->u;
	// save code to symbol
	if (u->scope == SCOPE_FUNCTION || u->scope == SCOPE_METHOD) {
		if (u->scope == SCOPE_METHOD && !strcmp(u->sym->name, "__init__")) {
			debug("class __init__ function");
			Inst_Append_NoArg(u->block, OP_LOAD0);
		}
		if (!u->block->bret) {
			debug("add 'return' to function '%s'", u->sym->name);
			Inst_Append_NoArg(u->block, OP_RET);
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
			JmpInst *jmp;
			Vector_ForEach(jmp, &u->jmps) {
				if (jmp->type == JMP_BREAK) {
					offset = u->block->bytes - jmp->inst->upbytes;
				} else {
					assert(jmp->type == JMP_CONTINUE);
					offset = 0 - jmp->inst->upbytes;
				}
				jmp->inst->arg.kind = ARG_INT;
				jmp->inst->arg.ival = offset;
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
		TypeDesc *proto = TypeDesc_From_Proto(NULL, NULL);
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

		if (u->block && u->block->bytes > 0) {
			debug("merge code into class or trait '%s' __init__ function",
				u->sym->name);
			Symbol *sym = STable_Get(u->sym->ptr, "__init__");
			if (!sym) {
				TypeDesc *proto = TypeDesc_From_Proto(NULL, NULL);
				sym = STable_Add_Proto(u->sym->ptr, "__init__", proto);
				assert(sym);
				Inst_Append_NoArg(u->block, OP_LOAD0);
				debug("add 'return' to function '__init__'");
				Inst_Append_NoArg(u->block, OP_RET);
			}

			if (u->sym->kind == SYM_TRAIT) {
				if(check_npr_func(sym)) {
					error("trait cannot have __init__ function");
					return;
				}
			} else {
				assert(u->sym->kind == SYM_CLASS);
			}

			codeblock_merge(sym->ptr, u->block);
			CodeBlock *b = sym->ptr;
			assert(list_empty(&b->insts));
			assert(!b->next);
			codeblock_free(b);
			sym->ptr = u->block;
			sym->locvars = u->stbl->varcnt;
			debug("save code to class '__init__'(defined) function");
		}	else {
			Symbol *sym = STable_Get(u->sym->ptr, "__init__");
			if (!sym) {
				TypeDesc *proto = TypeDesc_From_Proto(NULL, NULL);
				sym = STable_Add_Proto(u->sym->ptr, "__init__", proto);
				assert(sym);
				Inst_Append_NoArg(u->block, OP_LOAD0);
				debug("add 'return' to function '__init__'");
				Inst_Append_NoArg(u->block, OP_RET);
				sym->ptr = u->block;
				sym->locvars = 2; //FIXME
				debug("save code to class '__init__' function");
			}
		}

		u->block = NULL;
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
				Inst_Append_NoArg(u->block, OP_LOAD0);
				Argument val = {.kind = ARG_STR, .str = sym->name};
				Inst_Append(u->block, OP_GETFIELD, &val);
			} else if (sym->kind == SYM_PROTO) {
				debug("symbol '%s' is function", sym->name);
				Inst_Append_NoArg(u->block, OP_LOAD0);
				Argument val = {.kind = ARG_STR, .str = sym->name};
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
				Argument val = {.kind = ARG_INT, .ival = sym->index};
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
				Inst_Append_NoArg(u->block, OP_LOAD0);
				Inst_Append_NoArg(u->block, OP_GETM);
				Argument val = {.kind = ARG_STR, .str = sym->name};
				Inst_Append(u->block, OP_GETFIELD, &val);
			} else if (sym->kind == SYM_PROTO) {
				debug("symbol '%s' is function", sym->name);
				Inst_Append_NoArg(u->block, OP_LOAD0);
				Inst_Append_NoArg(u->block, OP_GETM);
				Argument val = {.kind = ARG_STR, .str = sym->name};
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
				Inst_Append_NoArg(u->block, OP_LOAD0);
				opcode = (exp->ctx == EXPR_LOAD) ? OP_GETFIELD : OP_SETFIELD;
				Argument val = {.kind = ARG_STR, .str = sym->name};
				Inst_Append(u->block, opcode, &val);
			} else if (sym->kind == SYM_PROTO) {
				debug("symbol '%s' is function", sym->name);
				Inst_Append_NoArg(u->block, OP_LOAD0);
				Argument val = {.kind = ARG_STR, .str = sym->name};
				Inst *i = Inst_Append(u->block, OP_CALL, &val);
				i->argc = exp->argc;
			} else if (sym->kind == SYM_CLASS) {
				debug("symbol '%s' is class", sym->name);
				if (exp->right && exp->right->kind == CALL_KIND) {
					debug("new object :%s", exp->sym->name);
					Inst_Append_NoArg(u->block, OP_LOAD0);
					Inst_Append_NoArg(u->block, OP_GETM);
					Argument val = {.kind = ARG_STR, .str = sym->name};
					Inst *i = Inst_Append(u->block, OP_NEW, &val);
					i->argc = exp->argc;
					val.str = "__init__";
					i = Inst_Append(u->block, OP_CALL, &val);
					i->argc = exp->argc;
				} else {
					if (!exp->right) {
						debug("'%s' is a class", sym->name);
						Inst_Append_NoArg(u->block, OP_LOAD0);
						Inst_Append_NoArg(u->block, OP_GETM);
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
			if (sym->up->kind == SYM_CLASS || sym->up->kind == SYM_TRAIT) {
				debug("symbol '%s' in class '%s'", sym->name, sym->up->name);
				debug("symbol '%s' is inherited ? %s", sym->name,
					sym->inherited ? "true" : "false");
				if (sym->kind == SYM_VAR) {
					debug("symbol '%s' is variable", sym->name);
					Inst_Append_NoArg(u->block, OP_LOAD0);
					int opcode = (exp->ctx == EXPR_LOAD) ? OP_GETFIELD : OP_SETFIELD;
					Argument val = {.kind = ARG_STR, .str = sym->name};
					Inst_Append(u->block, opcode, &val);
				} else if (sym->kind == SYM_PROTO) {
					debug("symbol '%s' is function", sym->name);
					Inst_Append_NoArg(u->block, OP_LOAD0);
					Argument val = {.kind = ARG_STR, .str = sym->name};
					Inst *i = Inst_Append(u->block, OP_CALL, &val);
					i->argc = exp->argc;
				} else if (sym->kind == SYM_IPROTO) {
					debug("symbol '%s' is abstract function", sym->name);
					Inst_Append_NoArg(u->block, OP_LOAD0);
					Argument val = {.kind = ARG_STR, .str = sym->name};
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
					Inst_Append_NoArg(u->block, OP_LOAD0);
					Inst_Append_NoArg(u->block, OP_GETM);
					int opcode = (exp->ctx == EXPR_LOAD) ? OP_GETFIELD : OP_SETFIELD;
					Argument val = {.kind = ARG_STR, .str = sym->name};
					Inst_Append(u->block, opcode, &val);
				} else if (sym->kind == SYM_PROTO) {
					debug("symbol '%s' is function", sym->name);
					Inst_Append_NoArg(u->block, OP_LOAD0);
					Inst_Append_NoArg(u->block, OP_GETM);
					Argument val = {.kind = ARG_STR, .str = sym->name};
					Inst *i = Inst_Append(u->block, OP_CALL, &val);
					i->argc = exp->argc;
				} else if (sym->kind == SYM_CLASS) {
					debug("symbol '%s' is class", sym->name);
					Inst_Append_NoArg(u->block, OP_LOAD0);
					Inst_Append_NoArg(u->block, OP_GETM);
					Argument val = {.kind = ARG_STR, .str = sym->name};
					Inst *i = Inst_Append(u->block, OP_NEW, &val);
					i->argc = exp->argc;
					val.str = "__init__";
					i = Inst_Append(u->block, OP_CALL, &val);
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
				Argument val = {.kind = ARG_INT, .ival = sym->index};
				Inst_Append(u->block, opcode, &val);
			} else if (up->kind == SYM_CLASS) {
				debug("symbol '%s' is class variable", sym->name);
				debug("symbol '%s' is inherited ? %s", sym->name,
					sym->inherited ? "true" : "false");
				Inst_Append_NoArg(u->block, OP_LOAD0);
				int opcode = (exp->ctx == EXPR_LOAD) ? OP_GETFIELD : OP_SETFIELD;
				Argument val = {.kind = ARG_STR, .str = sym->name};
				Inst_Append(u->block, opcode, &val);
			} else if (up->kind == SYM_MODULE) {
				debug("symbol '%s' is module variable", sym->name);
				Inst_Append_NoArg(u->block, OP_LOAD0);
				Inst_Append_NoArg(u->block, OP_GETM);
				int opcode = (exp->ctx == EXPR_LOAD) ? OP_GETFIELD : OP_SETFIELD;
				Argument val = {.kind = ARG_STR, .str = sym->name};
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

static void gencode_id_in_super(ParserState *ps, struct expr *exp, Symbol *sym)
{
	ParserUnit *u = ps->u;
	char *id = exp->id;

	sym->refcnt++;
	parser_expr_desc(ps, exp, sym);

	Argument val = {.kind = ARG_STR, .str = id};

	if (sym->kind == SYM_VAR) {
		if (exp->ctx == EXPR_LOAD) {
			debug("load:%s", val.str);
			Inst_Append_NoArg(u->block, OP_LOAD0);
			Inst_Append(ps->u->block, OP_GETFIELD, &val);
		} else {
			assert(exp->ctx == EXPR_STORE);
			debug("store:%s", val.str);
			Inst_Append_NoArg(u->block, OP_LOAD0);
			Inst_Append(ps->u->block, OP_SETFIELD, &val);
		}
	} else if (sym->kind == SYM_PROTO) {
		Inst_Append_NoArg(u->block, OP_LOAD0);
		Inst *i = Inst_Append(u->block, OP_CALL, &val);
		i->argc = exp->argc;
	} else {
		assert(0);
	}
}

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

	// ident is in traits ?
	ParserUnit *cu = get_class_unit(ps);
	if (cu && Vector_Size(&cu->sym->traits) > 0) {
		Symbol *trait;
		Vector_ForEach(trait, &cu->sym->traits) {
			sym = STable_Get(trait->ptr, id);
			if (sym) {
				debug("symbol '%s' is found in trait '%s'", id, trait->name);
				gencode_id_in_super(ps, exp, sym);
				return;
			}
		}
	}

	// ident is in super ?
	Symbol *super = NULL;
	if (cu && cu->sym->super) {
		super = cu->sym->super;
		sym = STable_Get(super->ptr, id);
		if (sym) {
			debug("symbol '%s' is found in super '%s'", id, super->name);
			gencode_id_in_super(ps, exp, sym);
			return;
		}
	}

	// ident is current module's name
	if (ps->package && !strcmp(id, ps->package)) {
		sym = ps->sym;
		assert(sym);
		debug("symbol '%s' is current module", id);
		sym->refcnt++;
		parser_expr_desc(ps, exp, sym);
		assert(exp->ctx == EXPR_LOAD);
		Inst_Append_NoArg(u->block, OP_LOAD0);
		Inst_Append_NoArg(u->block, OP_GETM);
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
		Argument val = {.kind = ARG_STR, .str = sym->path};
		Inst_Append(u->block, OP_LOADM, &val);
		return;
	}

	Parser_PrintError(ps, &exp->line, "cannot find '%s'", id);
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
		return;
	}

	Symbol *sym = NULL;
	if (leftsym->kind == SYM_STABLE) {
		debug(">>>>symbol '%s' is a module", leftsym->name);
		sym = STable_Get(leftsym->ptr, exp->attribute.id);
		if (!sym) {
			Parser_PrintError(ps, &exp->line, "cannot find '%s' in '%s'",
				exp->attribute.id, left->str);
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
			//FIXME: free(typestr);
			char *typestr = TypeDesc_ToString(leftsym->desc);
			error("cannot find '%s' in '%s'", exp->attribute.id, typestr);
			free(typestr);
			return;
		}
		assert(sym->kind == SYM_STABLE ||
			sym->kind == SYM_CLASS || sym->kind == SYM_TRAIT);
		char *typename = sym->name;
		sym = find_symbol_linear_order(sym, exp->attribute.id);
		if (!sym) {
			error("cannot find '%s' in '%s'", exp->attribute.id, typename);
			return;
		}
	} else if (leftsym->kind == SYM_CLASS) {
		debug("symbol '%s' is a class", leftsym->name);
		sym = find_symbol_linear_order(leftsym, exp->attribute.id);
		//sym = STable_Get(leftsym->ptr, exp->attribute.id);
		if (!sym) {
			error("cannot find '%s' in '%s'", exp->attribute.id, leftsym->name);
			return;
			// if (!leftsym->super) {
			// 	error("cannot find '%s' in '%s' class",
			// 		exp->attribute.id, leftsym->name);
			// 	return;
			// }

			// sym = STable_Get(leftsym->super->ptr, exp->attribute.id);
			// if (!sym) {
			// 	error("cannot find '%s' in super '%s' class",
			// 		exp->attribute.id, leftsym->super->name);
			// 	return;
			// }

			// debug("find '%s' in super '%s' class",
			// 	exp->attribute.id, leftsym->super->name);
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
	Argument val = {.kind = ARG_STR, .str = NULL};

	if (sym->kind == SYM_VAR) {
		if (exp->supername) {
			char *namebuf = malloc(strlen(exp->supername) + 1 +
				strlen(exp->attribute.id) + 1);
			assert(namebuf);
			sprintf(namebuf, "%s.%s", exp->supername, exp->attribute.id);
			val.str = namebuf;
		} else {
			val.str = exp->attribute.id;
		}

		if (exp->ctx == EXPR_LOAD) {
			debug("load:%s", val.str);
			Inst_Append(ps->u->block, OP_GETFIELD, &val);
		} else {
			assert(exp->ctx == EXPR_STORE);
			debug("store:%s", val.str);
			Inst_Append(ps->u->block, OP_SETFIELD, &val);
		}
	} else if (sym->kind == SYM_PROTO) {
		if (exp->supername) {
			char *namebuf = malloc(strlen(exp->supername) + 1 +
				strlen(exp->attribute.id) + 1);
			assert(namebuf);
			sprintf(namebuf, "%s.%s", exp->supername, exp->attribute.id);
			val.str = namebuf;
		} else {
			val.str = exp->attribute.id;
		}
		Inst *i = Inst_Append(u->block, OP_CALL, &val);
		i->argc = exp->argc;
	} else if (sym->kind == SYM_STABLE) {
		// new object
		val.str = sym->name;
		Inst *i = Inst_Append(u->block, OP_NEW, &val);
		i->argc = exp->argc;
		val.str = "__init__";
		i = Inst_Append(u->block, OP_CALL, &val);
		i->argc = exp->argc;
	} else if (sym->kind == SYM_IPROTO) {
		if (left->kind == SUPER_KIND) {
			//for super.xyz(), super is abstract method and not impl in traits
			char *namebuf = malloc(strlen("super") + 1 +
				strlen(exp->attribute.id) + 1);
			assert(namebuf);
			sprintf(namebuf, "super.%s", exp->attribute.id);
			val.str = namebuf;
			Inst *i = Inst_Append(u->block, OP_CALL, &val);
			i->argc = exp->argc;
		} else {
			val.str = sym->name;
			Inst *i = Inst_Append(u->block, OP_CALL, &val);
			i->argc = exp->argc;
		}
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
		if (!check_call_args(exp->desc, exp->call.args)) {
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
			TypeDesc *proto = __init__->desc;
			/* check arguments and returns(no returns) */
			if (!check_call_args(proto, exp->call.args)) {
				error("Arguments of __init__ are not matched.");
				return;
			}
			/* check no returns */
			if (Vector_Size(proto->rdesc) > 0) {
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
		if (!check_call_args(exp->desc, exp->call.args)) {
			error("abstract method arguments are not matched.");
			return;
		}
	} else {
		assert(0);
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

static void handle_traits(ParserState *ps, Symbol *sym)
{
	ParserUnit *u = ps->u;
	Symbol *trait;
	Argument val = {.kind = ARG_STR};

	Vector_ForEach(trait, &sym->traits) {
		char *namebuf = malloc(strlen(trait->name) + 1 + strlen("__init__") + 1);
		assert(namebuf);
		sprintf(namebuf, "%s.__init__", trait->name);
		debug("call trait '%s' __init__ function", trait->name);
		Inst_Append_NoArg(u->block, OP_LOAD0);
		val.str = namebuf;
		Inst *i = Inst_Append(u->block, OP_CALL, &val);
		i->argc = 0;
	}
}

static void parser_super(ParserState *ps, struct expr *exp)
{
	ParserUnit *u = ps->u;
	ParserUnit *cu = get_class_unit(ps);

	//NOTES: no need find traits, find in super class only
	Symbol *super;
	struct expr *r = exp->right;
	if (r->kind == CALL_KIND) {
		assert(u->scope == SCOPE_METHOD);
		assert(!strcmp(u->sym->name, "__init__"));
		assert(cu->sym->kind == SYM_CLASS);
		assert(cu->sym->super);
		exp->sym = cu->sym->super;
		super = cu->sym->super;
		char *namebuf = malloc(strlen(super->name) + 1 + strlen(u->sym->name) + 1);
		assert(namebuf);
		sprintf(namebuf, "%s.%s", super->name, u->sym->name);
		Inst_Append_NoArg(u->block, OP_LOAD0);
		Argument val = {.kind = ARG_STR, .str = namebuf};
		Inst *i = Inst_Append(u->block, OP_CALL, &val);
		i->argc = exp->argc;
		handle_traits(ps, cu->sym);
	} else if (r->kind == ATTRIBUTE_KIND) {
		Inst_Append_NoArg(u->block, OP_LOAD0);
		//find in linear-order traits
		Symbol *s;
		Symbol *sym = NULL;
		Vector_ForEach_Reverse(s, &cu->sym->traits) {
			sym = STable_Get(s->ptr, exp->right->attribute.id);
			if (sym) break;
		}
		if (sym) {
			exp->sym = s;
			super = s;
		} else {
			assert(cu->sym->super);
			exp->sym = cu->sym->super;
			super = cu->sym->super;
		}
	} else {
		assert(0);
	}

	r->supername = super->name;
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
			Argument val = {.kind = ARG_INT, .ival = exp->ival};
			Inst_Append(ps->u->block, OP_LOADK, &val);
			break;
		}
		case FLOAT_KIND: {
			assert(exp->ctx == EXPR_LOAD);
			Argument val = {.kind = ARG_FLOAT, .fval = exp->fval};
			Inst_Append(ps->u->block, OP_LOADK, &val);
			break;
		}
		case BOOL_KIND: {
			assert(exp->ctx == EXPR_LOAD);
			Argument val = {.kind = ARG_BOOL, .fval = exp->bval};
			Inst_Append(ps->u->block, OP_LOADK, &val);
			break;
		}
		case STRING_KIND: {
			assert(exp->ctx == EXPR_LOAD);
			Argument val = {.kind = ARG_STR, .str = exp->str};
			Inst_Append(ps->u->block, OP_LOADK, &val);
			break;
		}
		case SELF_KIND: {
			assert(exp->ctx == EXPR_LOAD);
			debug("load self");
			ParserUnit *cu = get_class_unit(ps);
			exp->sym = cu->sym;
			Inst_Append_NoArg(ps->u->block, OP_LOAD0);
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

			Argument val = {.kind = ARG_STR, .str = "koala/lang"};
			Inst_Append(ps->u->block, OP_LOADM, &val);
			val.str = "TypeOf";
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
		case UNARY_KIND: {
			debug("unary_op:%d", exp->unary.op);
			exp->unary.operand->ctx = EXPR_LOAD;
			parser_visit_expr(ps, exp->unary.operand);
			codegen_unary(ps, exp->unary.op);
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

#if 0
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
#endif

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
			TypeDesc *proto = exp->desc;
			int rsize = Vector_Size(proto->rdesc);
			if (rsize != 1) {
				//FIXME
				error("function's returns is not 1:%d", rsize);
				return;
			}

			if (!var->desc) {
				var->desc = Vector_Get(proto->rdesc, 0);
			}

			if (!TypeDesc_Check(var->desc, Vector_Get(proto->rdesc, 0))) {
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
					//Symbol *s1 = find_userdef_symbol(ps, var->desc);
					//Symbol *s2 = find_userdef_symbol(ps, exp->desc);
					// if (s1->kind == SYM_INTF) {
					// 	if (s2->kind == SYM_CLASS) {
					// 		if (check_intf_cls_inherit(s1, s2)) {
					// 			error("check intf <- class failed");
					// 			return;
					// 		}
					// 	} else if (s2->kind == SYM_INTF) {
					// 		if (check_intf_intf_inherit(s1, s2)) {
					// 			error("check intf <- intf failed");
					// 			return;
					// 		}
					// 	} else {
					// 		assertm(0, "invalid:%d", s2->kind);
					// 	}
					// } else {
					// 	if (check_symbol_inherit(s1, s2)) {
					// 		error("check_symbol_inherit(class <- class) failed");
					// 		return;
					// 	}
					// }
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
			STable_Update_Symbol(u->stbl, sym, var->desc);
			char *typestr = TypeDesc_ToString(var->desc);
			debug("update symbol '%s' type as '%s'", var->id, typestr);
			free(typestr);
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
			Inst_Append_NoArg(ps->u->block, OP_LOAD0);
			Argument val = {.kind = ARG_STR, .str = var->id};
			Inst_Append(ps->u->block, OP_SETFIELD, &val);
		} else if (u->scope == SCOPE_FUNCTION || u->scope == SCOPE_METHOD) {
			// local's variable
			Argument val = {.kind = ARG_INT, .ival = sym->index};
			Inst_Append(ps->u->block, OP_STORE, &val);
		} else if (u->scope == SCOPE_BLOCK) {
			// local's variable in block
			Argument val = {.kind = ARG_INT, .ival = sym->index};
			Inst_Append(ps->u->block, OP_STORE, &val);
		} else {
			assert(0);
		}
	}
}

static void parser_init_function(ParserState *ps, Vector *stmts)
{
	debug("------parser_init_function-------");
	if (stmts) {
		struct stmt *s;
		Vector_ForEach(s, stmts) {
			if (i == 0) {
				struct expr *exp = s->exp;
				if (exp->kind != SUPER_KIND) {
					ParserUnit *cu = get_class_unit(ps);
					if (cu->sym->kind == SYM_CLASS) {
						debug("class hanlde traits init");
						handle_traits(ps, cu->sym);
					} else {
						assert(cu->sym->kind == SYM_TRAIT);
						debug("trait needs not handle traits init");
					}
				}
			}
			parser_vist_stmt(ps, s);
		}
	}
	debug("------parser_init_function end---");
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
		if (!strcmp(stmt->funcdecl.id, "__init__"))
			parser_init_function(ps, stmt->funcdecl.body);
		else
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
		s = STable_Add_Proto(stbl, namebuf, sym->desc);
		if (s) {
			s->up = subsym;
			s->inherited = 1;
			s->super = sym;
		}
	} else {
		assertm(0, "invalid symbol kind:%d", sym->kind);
	}
}

static int trait_in_super(Symbol *trait, Symbol *sym)
{
	if (!sym) return 0;

	Symbol *t;
	Vector_ForEach_Reverse(t, &sym->traits) {
		if (t == trait) return 1;
	}

	return trait_in_super(trait, sym->super);
}

static void line_trait(Symbol *trait, Symbol *to)
{
	assert(trait->kind == SYM_TRAIT);
	Symbol *s;
	Vector_ForEach(s, &to->traits) {
		if (s == trait) {
			debug("trait '%s' is already in liner order", trait->name);
			return;
		}
	}

	if (trait_in_super(trait, to->super)) {
		debug("trait '%s' is already in super class '%s'",
			trait->name, to->super->name);
		return;
	}

	debug("add trait '%s' to liner order", trait->name);
	Vector_Append(&to->traits, trait);
}

static void parser_order(Vector *vec, Symbol *to)
{
	debug("------parse order -------");
	Symbol *s;
	Vector_ForEach(s, vec) {
		parser_order(&s->traits, to);
		line_trait(s, to);
	}

	debug("------parse order end-------");
}

struct check_trait_s {
	int index;
	Symbol *sym;
};

static void __check_trait_fn(Symbol *sym, void *arg)
{
	if (sym->kind != SYM_IPROTO) return;

	struct check_trait_s *temp = arg;
	Symbol *s;
	if (temp->sym->super) {
		s = STable_Get(temp->sym->super->ptr, sym->name);
		if (s) {
			debug("'%s' has impl in super class '%s'",
				sym->name, temp->sym->super->name);
			return;
		}
	}

	int size = Vector_Size(&temp->sym->traits);
	Symbol *trait;
	for (int i = temp->index; i < size; i++) {
		trait = Vector_Get(&temp->sym->traits, i);
		s = STable_Get(trait->ptr, sym->name);
		if (!s) {
			debug("not found '%s' in trait '%s'", sym->name, trait->name);
			continue;
		}
		if (s->kind != SYM_PROTO) {
			debug("'%s' in trait '%s' is not a func", sym->name, trait->name);
			continue;
		}
		//check function's proto
		debug("found '%s' in trait '%s'", sym->name, trait->name);
		return;
	}

	s = STable_Get(temp->sym->ptr, sym->name);
	if (!s) {
		error("'%s' has not impl in line-order in class '%s'",
			sym->name, temp->sym->name);
		return;
	}
	if (s->kind != SYM_PROTO) {
		error("'%s' in class '%s' is not a func", sym->name, temp->sym->name);
		return;
	}
	//check function's proto
	debug("'%s' has impl in line-order in class '%s'",
		sym->name,temp->sym->name);
}

// flat extends' traits
static int parser_traits(ParserState *ps, Vector *traits, Symbol *sym)
{
	debug("------parse traits -------");

	Vector syms = VECTOR_INIT;
	TypeDesc *desc;
	Symbol *s;
	Vector_ForEach(desc, traits) {
		//FIXME
		char *typestr = TypeDesc_ToString(desc);
		debug("trait '%s'", typestr);
		s = find_userdef_symbol(ps, desc);
		if (!s) {
			error("cannot find trait '%s'", typestr);
			free(typestr);
			return -1;
		}
		if (s->kind != SYM_TRAIT) {
			error("symbol '%s' is a trait", typestr);
			free(typestr);
			return -1;
		}
		free(typestr);
		assert(s->ptr);
		Vector_Append(&syms, s);
	}

	parser_order(&syms, sym);

	Vector_Fini(&syms, NULL, NULL);

	//check traits abstract funcs whether have impl in class
	if (sym->kind == SYM_CLASS) {
		struct check_trait_s temp = {.sym = sym};
		Vector_ForEach(s, &sym->traits) {
			if (s->kind == SYM_TRAIT) {
				debug(">>>>check abstract functions in trait '%s'<<<<", s->name);
				temp.index = i + 1;
				STable_Traverse(s->ptr, __check_trait_fn, &temp);
			}
		}
	}

	debug("------parse traits end -------");

	return 0;
}

static void parser_class(ParserState *ps, struct stmt *stmt)
{
	debug("------parse class(trait) -------");
	debug("%s:%s", stmt->kind == CLASS_KIND ? "class" : "trait",
		stmt->class_info.id);

	ParserUnit *u = ps->u;
	Symbol *sym = STable_Get(u->stbl, stmt->class_info.id);
	assert(sym && sym->ptr);

	TypeDesc *desc = stmt->class_info.super;
	if (desc) {
		char *typestr = TypeDesc_ToString(desc);
		debug("class's super '%s'", typestr);
		Symbol *super = find_userdef_symbol(ps, desc);
		if (!super) {
			error("cannot find super '%s'", typestr);
			free(typestr);
			return;
		}
		if (super->kind != SYM_CLASS && super->kind != SYM_STABLE) {
			error("super '%s' is not a class", typestr);
			free(typestr);
			return;
		}
		free(typestr);
		assert(super->ptr);
		sym->super = super;
		// Vector_Append(&sym->extends, super);
	}

	if (stmt->class_info.traits) {
		if (parser_traits(ps, stmt->class_info.traits, sym))
			return;
	}

	parser_enter_scope(ps, sym->ptr, SCOPE_CLASS);

	ps->u->sym = sym;

	if (stmt->class_info.body) {
		struct stmt *s;
		Vector_ForEach(s, stmt->class_info.body) {
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
	}

	parser_exit_scope(ps);

	printf("++++++++++linear order++++++++++\n");
	printf("%s ", sym->name);
	Symbol *sb;
	Vector_ForEach_Reverse(sb, &sym->traits) {
		printf("-> %s ", sb->name);
	}
	if (sym->super) printf("-> %s", sym->super->name);
	printf("\n++++++++++++++++++++++++++++++\n");
	debug("------parse class(trait) end---");
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
		jumpfalse->arg.kind = ARG_INT;
		jumpfalse->arg.ival = offset;
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
		Argument val = {.kind = ARG_INT, .ival = offset};
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
	Argument val = {.kind = ARG_INT, .ival = offset};
	Inst_Append(b, OP_JUMP_TRUE, &val);

	if (stmt->while_stmt.btest) {
		jmp->arg.kind = ARG_INT;
		jmp->arg.ival = bodysize;
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

ParserState *new_parser(void *filename)
{
	ParserState *ps = calloc(1, sizeof(ParserState));
	ps->filename = filename;
	yylex_init_extra(ps, &ps->scanner);
	Vector_Init(&ps->stmts);
	init_imports(ps);
	Symbol *sym = Symbol_New(SYM_MODULE);
	sym->ptr = STable_New(NULL);
	ps->sym = sym;
	init_list_head(&ps->ustack);
	Vector_Init(&ps->errors);
	return ps;
}

void destroy_parser(ParserState *ps)
{
	vec_stmt_fini(&ps->stmts);
	fini_imports(ps);
	free(ps->package);
	yylex_destroy(ps->scanner);
	free(ps);
}

void parser_module(ParserState *ps, FILE *in)
{
	parser_enter_scope(ps, ps->sym->ptr, SCOPE_MODULE);
	ps->u->sym = ps->sym;

	yyset_in(in, ps->scanner);
	yyparse(ps, ps->scanner);

	parser_body(ps, &ps->stmts);

	check_unused_imports(ps);

	parser_exit_scope(ps);
}

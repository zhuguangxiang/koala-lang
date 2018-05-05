
#include "parser.h"
#include "log.h"

static void __unused_import_fn(Symbol *sym, void *arg)
{
	ParserState *ps = arg;
	UNUSED_PARAMETER(ps);

	if (sym->refcnt != 0) return;

	warn("package '%s <- %s' is never used",
		sym->name, TypeDesc_ToString(sym->desc));
}

void check_unused_imports(ParserState *ps)
{
	STable_Traverse(ps->extstbl, __unused_import_fn, ps);
}

static void __unused_symbol_fn(Symbol *sym, void *arg)
{
	ParserState *ps = arg;
	UNUSED_PARAMETER(ps);

	if ((sym->access == ACCESS_PRIVATE) && (sym->refcnt == 0) &&
		(!isupper(sym->name[0]))) {
		if (sym->kind == SYM_VAR) {
			warn("variable '%s' is never used", sym->name);
		} else if (sym->kind == SYM_PROTO) {
			warn("function '%s' is never used", sym->name);
		} else if (sym->kind == SYM_CLASS) {
			warn("class '%s' is never used", sym->name);
		} else if (sym->kind == SYM_TRAIT) {
			warn("trait '%s' is never used", sym->name);
		} else if (sym->kind == SYM_IPROTO) {
			warn("abstract function '%s' is never used", sym->name);
		} else {
			error("which symbol?: %d", sym->kind);
		}
	}
}

void check_unused_symbols(ParserState *ps)
{
	ParserUnit *u = ps->u;
	STable_Traverse(u->stbl, __unused_symbol_fn, ps);
}

#if 0
static int check_call_varg(TypeDesc *proto, Vector *vec)
{
	int sz = Vector_Size(vec);
	if (Vector_Size(proto->pdesc) -1 > sz) {
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

#endif

int check_call_args(TypeDesc *proto, Vector *vec)
{
	int psz = Vector_Size(proto->pdesc);
	if (!vec && !psz) {
		return 1;
	}

	//if (Proto_Has_Vargs(proto)) return check_call_varg(proto, vec);

	int sz = Vector_Size(vec);
	if (psz != sz) {
		error("func argc: expected %d, but %d", psz, sz);
		return 0;
	}

	TypeDesc *desc;
	struct expr *exp;
	Vector_ForEach(exp, vec) {
		desc = exp->desc;
		if (!desc) {
			error("expr's type is null");
			return 0;
		}
		if (desc->kind == TYPE_PROTO) {
			/* allow only one return value as function argument */
			if (Vector_Size(desc->rdesc) != 1) return 0;
			if (!TypeDesc_Check(Vector_Get(desc->rdesc, 0),
				Vector_Get(proto->pdesc, i)))
				return 0;
		} else {
			assert(desc->kind == TYPE_PRIMITIVE || desc->kind == TYPE_USERDEF);
			if (!TypeDesc_Check(desc, Vector_Get(proto->pdesc, i))) return 0;
		}
	}

	return 1;
}

int check_return_types(Symbol *sym, Vector *vec)
{
	assert(sym);
	TypeDesc *proto = sym->desc;
	assert(proto->kind == TYPE_PROTO);
	int rsz = Vector_Size(proto->rdesc);
	if (!vec) {
		return (rsz == 0) ? 1 : 0;
	} else {
		int sz = Vector_Size(vec);
		if (rsz != sz) {
			error("number of return values is not matched func's proto");
			return 0;
		}
		struct expr *exp;
		Vector_ForEach(exp, vec) {
			if (!TypeDesc_Check(exp->desc, Vector_Get(proto->rdesc, i))) {
				error("type check failed");
				return 0;
			}
		}
		return 1;
	}
}

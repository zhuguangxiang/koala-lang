
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
	STbl_Traverse(ps->extstbl, __unused_import_fn, ps);
}

static void __unused_symbol_fn(Symbol *sym, void *arg)
{
	ParserState *ps = arg;
	UNUSED_PARAMETER(ps);

	if ((sym->access == ACCESS_PRIVATE) && (sym->refcnt == 0)) {
		if (sym->kind == SYM_VAR) {
			warn("variable '%s' is never used", sym->name);
		} else if (sym->kind == SYM_PROTO) {
			warn("function '%s' is never used", sym->name);
		} else {
			error("which symbol: %d", sym->kind);
		}
	}
}

void check_unused_symbols(ParserState *ps)
{
	ParserUnit *u = ps->u;
	STbl_Traverse(u->stbl, __unused_symbol_fn, ps);
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

int check_call_args(Proto *proto, Vector *vec)
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

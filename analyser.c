
#include "parser.h"
#include "log.h"

void analyse_vardecl(void)
{

}

void analyse_func(void)
{

}

void Analyse(ParserState *ps)
{
	printf("-------Analyse-------\n");
	Parse_Statements(ps, &ps->stmts);
	printf("-------Analyse End---\n");
}

static void __unused_import_fn(Symbol *sym, void *arg)
{
	ParserState *ps = arg;
	UNUSED_PARAMETER(ps);

	if (sym->refcnt != 0) return;

	warn("package '%s <- %s' is never used",
		sym->name, TypeDesc_ToString(sym->desc));
}

void Check_Unused_Imports(ParserState *ps)
{
	STbl_Traverse(&ps->extstbl, __unused_import_fn, ps);
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

void Check_Unused_Symbols(ParserState *ps)
{
	ParserUnit *u = ps->u;
	STbl_Traverse(&u->stbl, __unused_symbol_fn, ps);
}

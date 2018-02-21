
#include "parser.h"

void analyse_vardecl(void)
{

}

void analyse_func(void)
{

}

static void __check_import_fn(Symbol *sym, void *arg)
{
	UNUSED_PARAMETER(arg);
	if (sym->refcnt != 0) return;
	warn("package '%s <- %s' is never used",
		sym->name, TypeDesc_ToString(sym->desc));
}

void Check_Imports(ParserState *ps)
{
	STbl_Traverse(&ps->extstbl, __check_import_fn, NULL);
}

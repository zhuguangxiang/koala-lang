
#include "moduleobject.h"
#include "tupleobject.h"
#include "kstate.h"
#include "log.h"

static Object *__io_print(Object *ob, Object *args)
{
	UNUSED_PARAMETER(ob);
	if (!args) {
		fprintf(stdout, "(nil)\n");
		return NULL;
	}

#define BUF_SIZE  512

	char temp[BUF_SIZE] = {0};
	char *buf = temp;
	int sz = Tuple_Size(args);
	debug("io.Print, argc:%d", sz);
	TValue val;
	int avail = 0;
	for (int i = 0; i < sz; i++) {
		val = Tuple_Get(args, i);
		avail = TValue_Print(buf, BUF_SIZE - (buf - temp), &val, 1);
		buf += avail;
		if (i + 1 < sz) {
			avail = snprintf(buf, BUF_SIZE - (buf - temp), " ");
			buf += avail;
		}
	}
	temp[buf - temp] = '\0';

	fwrite(temp, strlen(temp), 1, stdout);
	fflush(stdout);
	return NULL;
}

static Object *__io_println(Object *ob, Object *args)
{
	__io_print(ob, args);
	fwrite("\n", 1, 1, stdout);
	fflush(stdout);
	return NULL;
}

static FuncDef io_funcs[] = {
	{"Print", NULL, "...A", __io_print},
	{"Println", NULL, "...A", __io_println},
	{NULL}
};

void Init_IO_Module(void)
{
	Object *ob = Koala_New_Module("io", "koala/io");
	assert(ob);
	Module_Add_CFunctions(ob, io_funcs);
}

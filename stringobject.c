
#include "stringobject.h"
#include "tupleobject.h"
#include "moduleobject.h"
#include "symbol.h"
#include "hash.h"

Object *String_New(char *str)
{
	StringObject *sobj = malloc(sizeof(StringObject));
	init_object_head(sobj, &String_Klass);
	sobj->len = strlen(str);
	sobj->str = str;
	//Object_Add_GCList(s);
	return (Object *)sobj;
}

void String_Free(Object *ob)
{
	OB_ASSERT_KLASS(ob, String_Klass);
	free(ob);
}

char *String_RawString(Object *ob)
{
	StringObject *sobj = OB_TYPE_OF(ob, StringObject, String_Klass);
	return sobj->str;
}

static int isneg(char **s)
{
	if (**s == '-') {
		(*s)++;
		return 1;
	} else if (**s == '+') {
		(*s)++;
	}
	return 0;
}

static int hexavalue(int c)
{
	if (isdigit(c)) return c - '0';
	else return (tolower(c) - 'a') + 10;
}

TValue String_ToInteger(Object *ob)
{
	StringObject *sobj = OB_TYPE_OF(ob, StringObject, String_Klass);
	char *s = sobj->str;
	while (isspace(*s)) s++;  /* skip prefix spaces */
	int neg = isneg(&s);
	uint64 a = 0;
	if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {  /* hex? */
		s += 2;  /* skip '0x' */
		while (isxdigit(*s)) {
			a = a * 16 + hexavalue(*s);
			s++;
		}
	} else {  /* decimal */
		while (isdigit(*s)) {
			int d = *s - '0';
			a = a * 10 + d;
			s++;
		}
	}
	while (isspace(*s)) s++;  /* skip trailing spaces */
	if (*s != '\0') return NilValue;
	else { TValue val; setivalue(&val, neg ? 0ul - a : a); return val;}
}

/*-------------------------------------------------------------------------*/

static Object *__string_length(Object *ob, Object *args)
{
	ASSERT(!args);
	StringObject *sobj = OB_TYPE_OF(ob, StringObject, String_Klass);
	return Tuple_Build("i", sobj->len);
}

static Object *__string_tostring(Object *ob, Object *args)
{
	ASSERT(!args);
	OB_ASSERT_KLASS(ob, String_Klass);
	return Tuple_Build("O", ob);
}

static FuncDef string_funcs[] = {
	{"Length", 1, "i", 0, NULL, __string_length},
	{"ToString", 1, "s", 0, NULL, __string_tostring},
	{NULL}
};

void Init_String_Klass(void)
{
	Klass_Add_CFunctions(&String_Klass, string_funcs);
}

/*-------------------------------------------------------------------------*/

static int string_equal(TValue *v1, TValue *v2)
{
	Object *ob1 = VALUE_OBJECT(v1);
	Object *ob2 = VALUE_OBJECT(v2);
	StringObject *s1 = OB_TYPE_OF(ob1, StringObject, String_Klass);
	StringObject *s2 = OB_TYPE_OF(ob2, StringObject, String_Klass);
	return !strcmp(s1->str, s2->str);
}

static uint32 string_hash(TValue *v)
{
	Object *ob = VALUE_OBJECT(v);
	StringObject *s = OB_TYPE_OF(ob, StringObject, String_Klass);
	return hash_string(s->str);
}

static void string_free(Object *ob)
{
	OB_ASSERT_KLASS(ob, String_Klass);
	String_Free(ob);
}

Klass String_Klass = {
	OBJECT_HEAD_INIT(&Klass_Klass),
	.name  = "String",
	.bsize = sizeof(StringObject),

	.ob_free = string_free,

	.ob_hash = string_hash,
	.ob_cmp  = string_equal
};

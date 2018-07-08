
#include "stringobject.h"
#include "tupleobject.h"
#include "moduleobject.h"
#include "gc.h"
#include "hash.h"
#include "log.h"

HashTable StringCache;

static inline StringObject *__find_string(HashTable *cache, char *str, int len)
{
	StringObject strobj = {.len = len, .str = str};
	return HashTable_Find(cache, &strobj);
}

static inline int __add_string(HashTable *cache, StringObject *strobj)
{
	return HashTable_Insert(cache, &strobj->hnode);
}

static void init_string(StringObject *strobj, char *str, int len)
{
	Init_Object_Head(strobj, &String_Klass);
	Init_HashNode(&strobj->hnode, strobj);
	strobj->len = len;
	strobj->str = (char *)(strobj + 1);
	strcpy(strobj->str, str);
	__add_string(&StringCache, strobj);
}

Object *String_New(char *str)
{
	int len = strlen(str);
	StringObject *strobj = __find_string(&StringCache, str, len);
	if (strobj) {
		debug("found '%s' in string cache", str);
		return (Object *)strobj;
	}

	strobj = GC_Alloc(sizeof(StringObject) + (len + 1));
	init_string(strobj, str, len);
	return (Object *)strobj;
}

Object *String_New_NoGC(char *str)
{
	int len = strlen(str);
	StringObject *strobj = __find_string(&StringCache, str, len);
	if (strobj) {
		debug("found '%s' in string cache", str);
		return (Object *)strobj;
	}

	strobj = malloc(sizeof(StringObject) + (len + 1));
	init_string(strobj, str, len);
	return (Object *)strobj;
}

void String_Free(Object *ob)
{
	OB_ASSERT_KLASS(ob, String_Klass);
	StringObject *strobj = (StringObject *)ob;
	HashTable_Remove(&StringCache, &strobj->hnode);
	debug("free string:%s", strobj->str);
	//free(ob);
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
	//FIXME: dup with numberobject.c
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

static uint32 strobj_hash(void *k)
{
	StringObject *strobj = k;
	return hash_nstring(strobj->str, strobj->len);
}

static int strobj_equal(void *k1, void *k2)
{
	StringObject *strobj1 = k1;
	StringObject *strobj2 = k2;
	if (strobj1->len != strobj2->len) return 0;
	return !strcmp(strobj1->str, strobj2->str);
}

/*-------------------------------------------------------------------------*/

static Object *__string_concat(Object *ob, Object *args)
{
	TValue v = Tuple_Get(args, 0);
	OB_ASSERT_KLASS(ob, String_Klass);
	StringObject *s1 = (StringObject *)ob;
	assert(v.klazz == &String_Klass);
	StringObject *s2 = (StringObject *)v.ob;
	char buf[s1->len + s2->len + 1];
	strcpy(buf, s1->str);
	strcat(buf, s2->str);
	Object *res = String_New(buf);
	return Tuple_Build("O", res);
}

static Object *__string_length(Object *ob, Object *args)
{
	assert(!args);
	StringObject *sobj = OB_TYPE_OF(ob, StringObject, String_Klass);
	return Tuple_Build("i", sobj->len);
}

static Object *__string_tostring(Object *ob, Object *args)
{
	assert(!args);
	OB_ASSERT_KLASS(ob, String_Klass);
	return Tuple_Build("O", ob);
}

static FuncDef string_funcs[] = {
	{"Concat", "s", "s", __string_concat},
	{"Length", "i", NULL, __string_length},
	{"ToString", "s", NULL, __string_tostring},
	{NULL}
};

void Init_String_Klass(void)
{
	HashInfo hashinfo = {.hash = strobj_hash, .equal = strobj_equal};
	HashTable_Init(&StringCache, &hashinfo);
	Klass_Add_CFunctions(&String_Klass, string_funcs);
	String_New_NoGC("");
}

/*-------------------------------------------------------------------------*/

static int string_equal(TValue *v1, TValue *v2)
{
	Object *ob1 = v1->ob;
	Object *ob2 = v2->ob;
	StringObject *s1 = OB_TYPE_OF(ob1, StringObject, String_Klass);
	StringObject *s2 = OB_TYPE_OF(ob2, StringObject, String_Klass);
	return !strcmp(s1->str, s2->str);
}

static uint32 string_hash(TValue *v)
{
	Object *ob = v->ob;
	StringObject *s = OB_TYPE_OF(ob, StringObject, String_Klass);
	return hash_string(s->str);
}

static void string_free(Object *ob)
{
	OB_ASSERT_KLASS(ob, String_Klass);
	String_Free(ob);
}

static Object *string_tostring(TValue *v)
{
	return Tuple_From_TValues(v, 1);
}

Klass String_Klass = {
	OBJECT_HEAD_INIT(&String_Klass, &Klass_Klass)
	.name = "String",
	.basesize = sizeof(StringObject),
	.itemsize = 0,
	.ob_free  = string_free,
	.ob_hash  = string_hash,
	.ob_equal = string_equal,
	.ob_tostr = string_tostring,
};


#include "typedesc.h"
#include "log.h"

TypeDesc Byte_Type   = { .kind = TYPE_PRIME, .prime.val = PRIME_BYTE };
TypeDesc Char_Type   = { .kind = TYPE_PRIME, .prime.val = PRIME_CHAR };
TypeDesc Int_Type    = { .kind = TYPE_PRIME, .prime.val = PRIME_INT };
TypeDesc Float_Type  = { .kind = TYPE_PRIME, .prime.val = PRIME_FLOAT };
TypeDesc Bool_Type   = { .kind = TYPE_PRIME, .prime.val = PRIME_BOOL };
TypeDesc String_Type = { .kind = TYPE_PRIME, .prime.val = PRIME_STRING };
TypeDesc Any_Type    = { .kind = TYPE_PRIME, .prime.val = PRIME_ANY };
TypeDesc Varg_Type   = { .kind = TYPE_VARG };

typedef struct prime_type_s {
	int prime;
	char *str;
	TypeDesc *type;
} PrimeType;

static PrimeType prime_types[] = {
	{PRIME_BYTE,   "byte",   &Byte_Type},
	{PRIME_CHAR,   "char",   &Char_Type},
	{PRIME_INT,    "int",    &Int_Type},
	{PRIME_FLOAT,  "float",  &Float_Type},
	{PRIME_BOOL,   "bool",   &Bool_Type},
	{PRIME_STRING, "string", &String_Type},
	{PRIME_ANY,    "any",    &Any_Type},
};

static char *prime_tostring(int type)
{
	for (int i = 0; i < nr_elts(prime_types); i++) {
		if (type == prime_types[i].prime)
			return prime_types[i].str;
	}
	return NULL;
}

static TypeDesc *prime_type(int ch)
{
	for (int i = 0; i < nr_elts(prime_types); i++) {
		if (ch == prime_types[i].prime)
			return prime_types[i].type;
	}
	return NULL;
}

static TypeDesc *type_new(int kind)
{
	TypeDesc *desc = calloc(1, sizeof(TypeDesc));
	desc->kind = kind;
	return desc;
}

static TypeDesc *str_to_usrdef(char *str, int len)
{
	char *dot = strchr(str, '.');
	assert(dot);
	TypeDesc *desc = type_new(TYPE_USRDEF);
	desc->usrdef.path = strndup(str, dot - str);
	desc->usrdef.type = strndup(dot + 1, str + len - dot - 1);
	return desc;
}

/*---------------------------------------------------------------------------*/

static void __type_free_fn(void *item, void *arg)
{
	UNUSED_PARAMETER(arg);
	Type_Free(item);
}

void Type_Free(TypeDesc *desc)
{
	int kind = desc->kind;

	switch (kind) {
		case TYPE_PRIME: /** fallthrough */
		case TYPE_VARG: {
			debug("prime or varg type, no free");
			break;
		}
		case TYPE_USRDEF: {
			if (desc->usrdef.path) free(desc->usrdef.path);
			assert(desc->usrdef.type); free(desc->usrdef.type);
			free(desc);
			break;
		}
		case TYPE_PROTO: {
			Vector_Free(desc->proto.arg, __type_free_fn, NULL);
			Vector_Free(desc->proto.ret, __type_free_fn, NULL);
			free(desc);
			break;
		}
		case TYPE_MAP: {
			Type_Free(desc->map.key);
			Type_Free(desc->map.val);
			free(desc);
			break;
		}
		case TYPE_ARRAY: {
			Type_Free(desc->array.base);
			free(desc);
			break;
		}
		default: {
			assertm(0, "unknown type's kind %d\n", kind);
		}
	}
}

TypeDesc *Type_UsrDef_New(char *path, char *type)
{
	TypeDesc *desc = type_new(TYPE_USRDEF);
	desc->usrdef.path = strdup(path);
	desc->usrdef.type = strdup(type);
	return desc;
}

TypeDesc *Type_Proto_New(Vector *arg, Vector *ret)
{
	TypeDesc *desc = type_new(TYPE_PROTO);
	desc->proto.arg = arg;
	desc->proto.ret = ret;
	return desc;
}

TypeDesc *Type_Map_New(TypeDesc *key, TypeDesc *val)
{
	TypeDesc *desc = type_new(TYPE_MAP);
	desc->map.key = key;
	desc->map.val = val;
	return desc;
}

TypeDesc *Type_Array_New(int dims, TypeDesc *base)
{
	TypeDesc *desc = type_new(TYPE_ARRAY);
	desc->array.dims = dims;
	desc->array.base = base;
	return desc;
}

int TypeList_Equal(Vector *v1, Vector *v2)
{
	if (v1 && v2) {
		if (Vector_Size(v1) != Vector_Size(v2)) return 0;
		TypeDesc *t1, *t2;
		Vector_ForEach(t1, v1) {
			t2 = Vector_Get(v2, i);
			if (!Type_Equal(t1, t2)) return 0;
		}
		return 1;
	} else if (!v1 && !v2) {
		return 1;
	} else {
		return 0;
	}
}

int Type_Equal(TypeDesc *t1, TypeDesc *t2)
{
	if (t1 == t2) return 1;
	if (t1->kind != t2->kind) return 0;

	int kind = t1->kind;
	int eq = 0;
	switch (kind) {
		case TYPE_PRIME: {
			eq = (t1->prime.val == t2->prime.val);
			break;
		}
		case TYPE_USRDEF: {
			eq = !strcmp(t1->usrdef.type, t2->usrdef.type);
			if (!eq) break;

			if (t1->usrdef.path && t2->usrdef.path) {
				eq = !strcmp(t1->usrdef.path, t2->usrdef.path);
			} if (!t1->usrdef.path && !t2->usrdef.path) {
				eq = 1;
			} else {
				eq = 0;
			}
			break;
		}
		case TYPE_PROTO: {
			eq = TypeList_Equal(t1->proto.arg, t2->proto.arg);
			if (eq) eq = TypeList_Equal(t1->proto.ret, t2->proto.ret);
			break;
		}
		default: {
			assertm(0, "unknown type's kind %d\n", kind);
		}
	}
	return eq;
}

/* For print only */
//FIXME: memory free
void Type_ToString(TypeDesc *desc, char *buf)
{
	char *tmp;
	char *str = "";

	if (!desc) {
		debug("desc is null");
		return;
	}

	int kind = desc->kind;

	switch (kind) {
		case TYPE_PRIME: {
			strcpy(buf, prime_tostring(desc->prime.val));
			break;
		}
		case TYPE_USRDEF: {
			sprintf(buf, "%s.%s", desc->usrdef.path, desc->usrdef.type);
			break;
		}
		case TYPE_PROTO: {
			// FIXME
			strcpy(buf, "proto is not implemented");
			break;
		}
		case TYPE_MAP: {
			// FIXME
			strcpy(buf, "map is not implemented");
			break;
		}
		case TYPE_ARRAY: {
			// FIXME
			strcpy(buf, "array is not implemented");
			break;
		}
		case TYPE_VARG: {
			strcpy(buf, "...");
			break;
		}
		default: {
			assertm(0, "unknown type's kind %d\n", kind);
		}
	}
}

/**
 * "i[sOkoala/lang.Tuple;" -->>> int, []string, koala/lang.Tuple
 */
Vector *String_To_TypeList(char *str)
{
	if (!str) return NULL;
	Vector *v = Vector_New();
	TypeDesc *desc;
	char ch;
	int dims = 0;

	while ((ch = *str) != '\0') {
		if (ch == '.') {
			assert(str[1] == '.' && str[2] == '.');
			desc = &Varg_Type;
			Vector_Append(v, desc);
			str += 3;
			assert(!*str);
		} else if ((desc = prime_type(ch))) {
			if (dims > 0) desc = Type_Array_New(dims, desc);
			Vector_Append(v, desc);
			dims = 0;
			str++;
		} else if (ch == 'O') {
			char *start = str + 1;
			while ((ch = *str) != ';') str++;
			desc = str_to_usrdef(start, str - start);
			if (dims > 0) desc = Type_Array_New(dims, desc);
			Vector_Append(v, desc);
			dims = 0;
			str++;
		} else if (ch == '[') {
			while ((ch = *str) == '[') { dims++; str++; }
		} else {
			assertm(0, "unknown type:%c\n", ch);
		}
	}

	return v;
}

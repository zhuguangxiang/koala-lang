
#include "typedesc.h"
#include "log.h"

char *Primitive_ToString(int type)
{
	char *str = "";
	switch (type) {
		case PRIMITIVE_INT:
			str = "int";
			break;
		case PRIMITIVE_FLOAT:
			str = "float";
			break;
		case PRIMITIVE_BOOL:
			str = "bool";
			break;
		case PRIMITIVE_STRING:
			str = "string";
			break;
		case PRIMITIVE_ANY:
			str = "Any";
			break;
		default:
			assertm(0, "unknown primitive %c", type);
			break;
	}
	return str;
}

static int desc_primitive(char ch)
{
	static char chs[] = {
		PRIMITIVE_INT,
		PRIMITIVE_FLOAT,
		PRIMITIVE_BOOL,
		PRIMITIVE_STRING,
		PRIMITIVE_ANY
	};

	for (int i = 0; i < nr_elts(chs); i++) {
		if (ch == chs[i]) return 1;
	}

	return 0;
}

Vector *TypeString_To_Vector(char *str)
{
	if (!str) return NULL;
	Vector *v = Vector_New();
	TypeDesc *desc;
	char ch;
	int dims = 0;
	int varg = 0;
	int vcnt = 0;
	char *start;

	while ((ch = *str) != '\0') {
		if (ch == '.') {
			assert(str[1] == '.' && str[2] == '.');
			varg = 1;
			str += 3;
			vcnt++;
		} else if (desc_primitive(ch)) {
			desc = TypeDesc_From_Primitive(ch);
			desc->varg = varg;
			desc->dims = dims;
			Vector_Append(v, desc);

			/* varg and dims are only one valid */
			assert(varg == 0 || dims == 0);

			varg = 0; dims = 0;
			str++;
		} else if (ch == 'O') {
			start = str + 1;
			while ((ch = *str) != ';') {
				str++;
			}
			desc = TypeDesc_From_TypeNString(start, str - start);
			desc->varg = varg;
			desc->dims = dims;
			Vector_Append(v, desc);

			/* varg and dims are only one valid */
			assert(varg == 0 || dims == 0);

			varg = 0; dims = 0;
			str++;
		} else if (ch == '[') {
			assert(varg == 0);
			while ((ch = *str) == '[') {
				dims++; str++;
			}
		} else {
			assertm(0, "unknown type:%c\n", ch);
		}
	}

	assert(vcnt <= 1);
	return v;
}

/*---------------------------------------------------------------------------*/

TypeDesc *TypeDesc_New(int kind)
{
	TypeDesc *desc = calloc(1, sizeof(TypeDesc));
	desc->kind = kind;
	return desc;
}

static void __typedef_free_fn(void *item, void *arg)
{
	UNUSED_PARAMETER(arg);
	TypeDesc_Free(item);
}

void TypeDesc_Free(TypeDesc *desc)
{
	if (desc->kind == TYPE_USERDEF) {
		if (desc->path) free(desc->path);
		assert(desc->type); free(desc->type);
	} else if (desc->kind == TYPE_PROTO) {
		Vector_Free(desc->pdesc, __typedef_free_fn, NULL);
		Vector_Free(desc->rdesc, __typedef_free_fn, NULL);
	} else if (desc->kind == TYPE_PRIMITIVE) {
		debug("primitive type, just free TypeDesc itself");
	} else {
		assertm(0, "invalid TypeDesc kind:%d", desc->kind);
	}
	free(desc);
}

TypeDesc *TypeDesc_From_Primitive(int primitive)
{
	TypeDesc *desc = TypeDesc_New(TYPE_PRIMITIVE);
	desc->primitive = primitive;
	return desc;
}

TypeDesc *TypeDesc_From_UserDef(char *path, char *type)
{
	TypeDesc *desc = TypeDesc_New(TYPE_USERDEF);
	desc->path = path;
	desc->type = type;
	return desc;
}

TypeDesc *TypeDesc_From_Proto(Vector *rvec, Vector *pvec)
{
	TypeDesc *type = TypeDesc_New(TYPE_PROTO);
	type->pdesc = pvec;
	type->rdesc = rvec;
	return type;
}

TypeDesc *TypeDesc_From_TypeNString(char *typestr, int len)
{
	char *tmp = strchr(typestr, '.');
	assert(tmp);
	char *path = strndup(typestr, tmp - typestr);
	char *type = strndup(tmp + 1, typestr + len - tmp - 1);
	return TypeDesc_From_UserDef(path, type);
}

static int vec_type_check(Vector *v1, Vector *v2)
{
	if (v1 && v2) {
		if (Vector_Size(v1) != Vector_Size(v2)) return 0;
		TypeDesc *t1;
		TypeDesc *t2;
		Vector_ForEach(t1, v1) {
			t2 = Vector_Get(v2, i);
			if (!TypeDesc_Check(t1, t2)) return 0;
		}
		return 1;
	} else if (!v1 && !v2) {
		return 1;
	} else {
		return 0;
	}
}

int TypeDesc_Check(TypeDesc *t1, TypeDesc *t2)
{
	if (t1 == t2) return 1;

	// one of type is Any
	if (((t1->kind == TYPE_PRIMITIVE) && (t1->primitive == PRIMITIVE_ANY)) ||
			((t2->kind == TYPE_PRIMITIVE) && (t2->primitive == PRIMITIVE_ANY)))
		return 1;

	if (t1->kind != t2->kind) return 0;
	if (t1->dims != t2->dims) return 0;

	int kind = t1->kind;
	int eq = 0;
	switch (kind) {
		case TYPE_PRIMITIVE: {
			eq = t1->primitive == t2->primitive;
			break;
		}
		case TYPE_USERDEF: {
			if (t1->path && t2->path) {
				eq = !strcmp(t1->path, t2->path) && !strcmp(t1->type, t2->type);
			} if (!t1->path && !t2->path) {
				eq = !strcmp(t1->type, t2->type);
			} else {
				eq = 0;
			}
			break;
		}
		case TYPE_PROTO: {
			eq = vec_type_check(t1->pdesc, t2->pdesc);
			if (eq) eq = vec_type_check(t1->rdesc, t2->rdesc);
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
char *TypeDesc_ToString(TypeDesc *desc)
{
	char *tmp;
	char *str = "";

	if (!desc) {
		warn("desc is null");
		return NULL;
	}

	int sz = desc->dims * 2;
	int dims = desc->dims;
	int count = 0;

	switch (desc->kind) {
		case TYPE_PRIMITIVE: {
			tmp = Primitive_ToString(desc->primitive);
			sz += strlen(tmp) + 1;
			str = malloc(sz);
			while (dims-- > 0) count += sprintf(str, "%s", "[]");
			strcpy(str + count, tmp);
			break;
		}
		case TYPE_USERDEF: {
			if (desc->path) sz += strlen(desc->path);
			sz += strlen(desc->type) + 1;
			str = malloc(sz);
			while (dims-- > 0) count += sprintf(str, "%s", "[]");
			if (desc->path)
				sprintf(str + count, "%s.%s", desc->path, desc->type);
			else
				sprintf(str + count, "%s", desc->type);
			break;
		}
		case TYPE_PROTO: {
			sz = 1;
			str = malloc(10);
			strcpy(str, "proto");
			break;
		}
		default: {
			assert(0);
			break;
		}
	}
	return str;
}

#if 0
Proto *Proto_New(int rsz, char *rdesc, int psz, char *pdesc)
{
	Proto *proto = malloc(sizeof(Proto));
	Init_Proto(proto, rsz, rdesc, psz, pdesc);
	return proto;
}

void Proto_Free(Proto *proto)
{
	Fini_Proto(proto);
	free(proto);
}

int Init_Proto(Proto *proto, int rsz, char *rdesc, int psz, char *pdesc)
{
	proto->rsz = rsz;
	proto->rdesc = str_to_typearray(rsz, rdesc);
	proto->psz = psz;
	proto->pdesc = str_to_typearray(psz, pdesc);
	return 0;
}

void Fini_Proto(Proto *proto)
{
	//FIXME
	//if (proto->rdesc) free(proto->rdesc);
	proto->rdesc = NULL;
	//if (proto->pdesc) free(proto->pdesc);
	proto->pdesc = NULL;
}

int Proto_Has_Vargs(Proto *proto)
{
	if (proto->psz > 0) {
		TypeDesc *desc = proto->pdesc + proto->psz - 1;
		return (desc->varg) ? 1 : 0;
	} else {
		return 0;
	}
}

Proto *Proto_Dup(Proto *proto)
{
	//FIXME:
	return proto;
}
#endif

int TypeDesc_IsBool(TypeDesc *desc)
{
	if (desc->kind == TYPE_PRIMITIVE && desc->primitive == PRIMITIVE_BOOL)
		return 1;
	else
		return 0;
}

// int Proto_IsEqual(Proto *p1, Proto *p2)
// {
// 	if (!p1 || !p2) return 0;
// 	if (p1 == p2) return 1;
// 	if (p1->psz != p2->psz) return 0;
// 	if (p1->rsz != p2->rsz) return 0;

// 	for (int i = 0; i < p1->psz; i++) {
// 		if (!TypeDesc_Check(p1->pdesc + i, p2->pdesc + i))
// 			return 0;
// 	}

// 	for (int i = 0; i < p1->rsz; i++) {
// 		if (!TypeDesc_Check(p1->rdesc + i, p2->rdesc + i))
// 			return 0;
// 	}

// 	return 1;
// }

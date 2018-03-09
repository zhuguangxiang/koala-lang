
#include "koala_type.h"
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

static TypeDesc *str_to_typearray(int count, char *str)
{
	if (count == 0) return NULL;
	TypeDesc *desc = malloc(sizeof(TypeDesc) * count);

	char ch;
	int idx = 0;
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
			assert(idx < count);

			desc[idx].varg = varg;
			desc[idx].dims = dims;
			desc[idx].kind = TYPE_PRIMITIVE;
			desc[idx].primitive = ch;

			/* varg and dims are only one valid */
			assert(varg == 0 || dims == 0);

			varg = 0; dims = 0;
			idx++; str++;
		} else if (ch == 'O') {
			assert(idx < count);

			desc[idx].varg = varg;
			desc[idx].dims = dims;
			desc[idx].kind = TYPE_USERDEF;

			int cnt = 0;
			start = str + 1;
			while ((ch = *str) != ';') {
				cnt++; str++;
			}
			FullPath_To_TypeDesc(start, str - start, desc + idx);

			/* varg and dims are only one valid */
			assert(varg == 0 || dims == 0);

			varg = 0; dims = 0;
			idx++; str++;
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
	return desc;
}

TypeDesc *TypeDesc_New(int kind)
{
	TypeDesc *desc = calloc(1, sizeof(TypeDesc));
	desc->kind = kind;
	return desc;
}

void TypeDesc_Free(TypeDesc *desc)
{
	if (desc->kind == TYPE_USERDEF) {
		if (desc->path) free(desc->path);
		free(desc->type);
	} else if (desc->kind == TYPE_PROTO) {
		Proto_Free(desc->proto);
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

TypeDesc *TypeDesc_From_Proto(Proto *proto)
{
	TypeDesc *desc = TypeDesc_New(TYPE_PROTO);
	desc->proto = proto;
	return desc;
}

TypeDesc *TypeDesc_From_FuncType(Vector *rvec, Vector *pvec)
{
	Proto *proto = malloc(sizeof(Proto));
	int sz;
	TypeDesc *desc;

	sz = TypeVec_ToTypeArray(rvec, &desc);
	proto->rsz = sz;
	proto->rdesc = desc;

	sz = TypeVec_ToTypeArray(pvec, &desc);
	proto->psz = sz;
	proto->pdesc = desc;

	TypeDesc *type = TypeDesc_New(TYPE_PROTO);
	type->proto = proto;

	return type;
}

int TypeVec_ToTypeArray(Vector *vec, TypeDesc **desc)
{
	if (!vec) {
		*desc = NULL;
		return 0;
	}
	//FIXME
	return Vector_ToArray(vec, sizeof(TypeDesc), NULL, (void **)desc);
}

void FullPath_To_TypeDesc(char *fullpath, int len, TypeDesc *desc)
{
	char *tmp = strchr(fullpath, '.');
	assert(tmp);
	desc->type = strndup(tmp + 1, fullpath + len - tmp - 1);
	desc->path = strndup(fullpath, tmp - fullpath);
}

static inline int type_isany(TypeDesc *t)
{
	if ((t->kind == TYPE_PRIMITIVE) && (t->primitive == PRIMITIVE_ANY))
		return 1;
	else
		return 0;
}

int TypeDesc_Check(TypeDesc *t1, TypeDesc *t2)
{
	if (t1 == t2) return 1;

	if (type_isany(t1) || type_isany(t2))
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
			} else {
				eq = !strcmp(t1->type, t2->type);
			}
			break;
		}
		case TYPE_PROTO: {
			//FIXME: assert(0);
			eq = 1;
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

int TypeDesc_IsBool(TypeDesc *desc)
{
	if (desc->kind == TYPE_PRIMITIVE && desc->primitive == PRIMITIVE_BOOL)
		return 1;
	else
		return 0;
}

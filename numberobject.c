
#include "numberobject.h"
#include "stringobject.h"

static int hexavalue (int c) {
  if (isdigit(c)) return c - '0';
  else return (tolower(c) - 'a') + 10;
}

static int isneg(char **s)
{
	if (**s == '-') { (*s)++; return 1; }
	else if (**s == '+') (*s)++;
	return 0;
}

static int str2int(char *s, uint64 *result)
{
	uint64 a = 0;
	int empty = 1;
	int neg;
	while (isspace(*s)) s++;  /* skip initial spaces */
	neg = isneg(&s);
	if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {  /* hex? */
		s += 2;  /* skip '0x' */
		for (; isxdigit(*s); s++) {
			a = a * 16 + hexavalue(*s);
			empty = 0;
		}
	} else {  /* decimal */
		int d;
		for (; isdigit(*s); s++) {
			d = *s - '0';
			/* check overflow? */
			a = a * 10 + d;
			empty = 0;
		}
	}
	while (isspace(*s)) s++;  /* skip trailing spaces */
	if (empty || *s != '\0') {
		return -1;  /* something wrong in the numeral */
	} else {
		*result = neg ? 0UL - a : a;
		return 0;
	}
}

static int str2flt(char *s, float64 *result)
{
	*result = atof(s);
	return 0;
}

TValue Int_Add_Int(TValue *v1, TValue *v2)
{
	TValue v;
	uint64 i = (uint64)VALUE_INT(v1) + (uint64)VALUE_INT(v2);
	setivalue(&v, i);
	return v;
}

TValue Int_Add_Float(TValue *v1, TValue *v2)
{
	TValue v;
	uint64 i = (uint64)VALUE_INT(v1) + (uint64)VALUE_FLOAT(v2);
	setivalue(&v, i);
	return v;
}

TValue Int_Add_String(TValue *v1, TValue *v2)
{
	TValue v;
	StringObject *strobj = (StringObject *)v2->ob;
	uint64 i = 0;
	float64 f = 0.0L;
	if (!str2int(strobj->str, &i)) {
		i = (uint64)VALUE_INT(v1) + i;
		setivalue(&v, i);
	} else if (!str2flt(strobj->str, &f)) {
		i = (uint64)VALUE_INT(v1) + (uint64)f;
		setivalue(&v, i);
	} else {
		assert(0);
	}
	return v;
}

TValue Int_Add(TValue *v1, TValue *v2)
{
	assert(v1->klazz == &Int_Klass);
	if (v2->klazz == &Int_Klass) {
		return Int_Add_Int(v1, v2);
	} else if (v2->klazz == &Float_Klass) {
		return Int_Add_Float(v1, v2);
	} else if (v2->klazz == &String_Klass) {
		return Int_Add_String(v1, v2);
	} else {
		assertm(0, "Int_Add:not supported with %s", v2->klazz->name);
		return NilValue;
	}
}

NumberFunctions IntOps = {
	.add = Int_Add,
};

Klass Int_Klass = {
	OBJECT_HEAD_INIT(&Int_Klass, &Klass_Klass)
	.name = "Integer",
	.ob_hash = NULL,
	.ob_equal = NULL,
	.ob_tostr = NULL,
	.numops = &IntOps,
};

Klass Float_Klass = {
	OBJECT_HEAD_INIT(&Float_Klass, &Klass_Klass)
	.name = "Float",
	.ob_hash = NULL,
	.ob_equal = NULL,
	.ob_tostr = NULL,
	.numops = NULL,
};

Klass Bool_Klass = {
	OBJECT_HEAD_INIT(&Bool_Klass, &Klass_Klass)
	.name = "Bool",
	.ob_hash = NULL,
	.ob_equal = NULL,
	.ob_tostr = NULL,
	.numops = NULL,
};

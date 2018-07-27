
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

TValue int_add_string(TValue *v1, TValue *v2)
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

static TValue int_add(TValue *v1, TValue *v2)
{
  TValue v = NilValue;
	assert(v1->klazz == &Int_Klass);
	if (v2->klazz == &Int_Klass) {
    uint64 i = (uint64)VALUE_INT(v1) + (uint64)VALUE_INT(v2);
    setivalue(&v, i);
	} else if (v2->klazz == &Float_Klass) {
    float64 f = (float64)VALUE_INT(v1) + (float64)VALUE_FLOAT(v2);
    setfltvalue(&v, f);
	} else {
		kassert(0, "int_add:not supported with %s", v2->klazz->name);
	}
  return v;
}

static TValue int_sub(TValue *v1, TValue *v2)
{
  TValue v = NilValue;
	assert(v1->klazz == &Int_Klass);
	if (v2->klazz == &Int_Klass) {
    uint64 i = (uint64)VALUE_INT(v1) - (uint64)VALUE_INT(v2);
    setivalue(&v, i);
	} else if (v2->klazz == &Float_Klass) {
    float64 f = (float64)VALUE_INT(v1) - (float64)VALUE_FLOAT(v2);
    setfltvalue(&v, f);
	} else {
		kassert(0, "int_sub:not supported with %s", v2->klazz->name);
	}
  return v;
}

static TValue int_mul(TValue *v1, TValue *v2)
{
  TValue v = NilValue;
	assert(v1->klazz == &Int_Klass);
	if (v2->klazz == &Int_Klass) {
    uint64 i = (uint64)VALUE_INT(v1) * (uint64)VALUE_INT(v2);
    setivalue(&v, i);
	} else if (v2->klazz == &Float_Klass) {
    float64 f = (float64)VALUE_INT(v1) * (float64)VALUE_FLOAT(v2);
    setfltvalue(&v, f);
	} else {
		kassert(0, "int_mul:not supported with %s", v2->klazz->name);
	}
  return v;
}

static TValue int_div(TValue *v1, TValue *v2)
{
  TValue v = NilValue;
	assert(v1->klazz == &Int_Klass);
	if (v2->klazz == &Int_Klass) {
    uint64 i = (uint64)VALUE_INT(v1) / (uint64)VALUE_INT(v2);
    setivalue(&v, i);
	} else if (v2->klazz == &Float_Klass) {
    float64 f = (float64)VALUE_INT(v1) / (float64)VALUE_FLOAT(v2);
    setfltvalue(&v, f);
	} else {
		kassert(0, "int_div:not supported with %s", v2->klazz->name);
	}
  return v;
}

static TValue int_mod(TValue *v1, TValue *v2)
{
  TValue v = NilValue;
	assert(v1->klazz == &Int_Klass);
	if (v2->klazz == &Int_Klass) {
    uint64 i = (uint64)VALUE_INT(v1) % (uint64)VALUE_INT(v2);
    setivalue(&v, i);
	} else if (v2->klazz == &Float_Klass) {
    float64 f = 0; //FIXME (float64)VALUE_INT(v1) % (float64)VALUE_FLOAT(v2);
    setfltvalue(&v, f);
	} else {
		kassert(0, "int_mod:not supported with %s", v2->klazz->name);
	}
  return v;
}

static TValue int_neg(TValue *v1)
{
  TValue v = NilValue;
	assert(v1->klazz == &Int_Klass);
  uint64 i = 0 - VALUE_INT(v1);
  setivalue(&v, i);
  return v;
}

NumberOperations int_ops = {
	.add = int_add,
  .sub = int_sub,
  .mul = int_mul,
  .div = int_div,
  .mod = int_mod,
  .neg = int_neg,
  /*
  .gt = int_gt,
  .lt = int_lt,
  .ge = int_ge,
  .le = int_le,
  .eq = int_eq,

  .band = int_bit_and,
  .bor  = int_bit_or,
  .bxor = int_bit_xor,
  .binvert = int_bit_invert,
  .blshift = int_lshift,
  .brshift = int_rshift,

  .land = int_logic_and,
  .lor  = int_logic_or,
  .lnot = int_logic_not,
  */
};

Klass Int_Klass = {
	OBJECT_HEAD_INIT(&Int_Klass, &Klass_Klass)
	.name = "Integer",
	.ob_hash = NULL,
	.ob_equal = NULL,
	.ob_tostr = NULL,
	.numops = &int_ops,
};

static TValue float_add(TValue *v1, TValue *v2)
{
  TValue v = NilValue;
	assert(v1->klazz == &Float_Klass);
	if (v2->klazz == &Int_Klass) {
    float64 f = (float64)VALUE_FLOAT(v1) + (uint64)VALUE_INT(v2);
    setfltvalue(&v, f);
	} else if (v2->klazz == &Float_Klass) {
    float64 f = (float64)VALUE_FLOAT(v1) + (float64)VALUE_FLOAT(v2);
    setfltvalue(&v, f);
	} else {
		kassert(0, "float_add:not supported with %s", v2->klazz->name);
	}
  return v;
}

static TValue float_sub(TValue *v1, TValue *v2)
{
  TValue v = NilValue;
	assert(v1->klazz == &Float_Klass);
	if (v2->klazz == &Int_Klass) {
    float64 f = (float64)VALUE_FLOAT(v1) - (uint64)VALUE_INT(v2);
    setfltvalue(&v, f);
	} else if (v2->klazz == &Float_Klass) {
    float64 f = (float64)VALUE_FLOAT(v1) - (float64)VALUE_FLOAT(v2);
    setfltvalue(&v, f);
	} else {
		kassert(0, "float_sub:not supported with %s", v2->klazz->name);
	}
  return v;
}

static TValue float_mul(TValue *v1, TValue *v2)
{
  TValue v = NilValue;
	assert(v1->klazz == &Float_Klass);
	if (v2->klazz == &Int_Klass) {
    float64 f = (float64)VALUE_FLOAT(v1) * (uint64)VALUE_INT(v2);
    setfltvalue(&v, f);
	} else if (v2->klazz == &Float_Klass) {
    float64 f = (float64)VALUE_FLOAT(v1) * (float64)VALUE_FLOAT(v2);
    setfltvalue(&v, f);
	} else {
		kassert(0, "int_mul:not supported with %s", v2->klazz->name);
	}
  return v;
}

static TValue float_div(TValue *v1, TValue *v2)
{
  TValue v = NilValue;
	assert(v1->klazz == &Float_Klass);
	if (v2->klazz == &Int_Klass) {
    float64 f = (float64)VALUE_FLOAT(v1) / (uint64)VALUE_INT(v2);
    setfltvalue(&v, f);
	} else if (v2->klazz == &Float_Klass) {
    float64 f = (float64)VALUE_FLOAT(v1) / (float64)VALUE_FLOAT(v2);
    setfltvalue(&v, f);
	} else {
		kassert(0, "int_div:not supported with %s", v2->klazz->name);
	}
  return v;
}

static TValue float_mod(TValue *v1, TValue *v2)
{
  TValue v = NilValue;
	assert(v1->klazz == &Float_Klass);
	if (v2->klazz == &Int_Klass) {
    float64 f = 0; //FIXME: (float64)VALUE_FLOAT(v1) % (uint64)VALUE_INT(v2);
    setfltvalue(&v, f);
	} else if (v2->klazz == &Float_Klass) {
    float64 f = 0; //FIXME: (float64)VALUE_FLOAT(v1) % (float64)VALUE_FLOAT(v2);
    setfltvalue(&v, f);
	} else {
		kassert(0, "int_mod:not supported with %s", v2->klazz->name);
	}
  return v;
}

static TValue float_neg(TValue *v1)
{
  TValue v = NilValue;
	assert(v1->klazz == &Float_Klass);
  float64 f = 0 - VALUE_FLOAT(v1);
  setfltvalue(&v, f);
  return v;
}

static NumberOperations float_ops = {
  .add = float_add,
  .sub = float_sub,
  .mul = float_mul,
  .div = float_div,
  .mod = float_mod,
  .neg = float_neg,
};

Klass Float_Klass = {
	OBJECT_HEAD_INIT(&Float_Klass, &Klass_Klass)
	.name = "Float",
	.ob_hash = NULL,
	.ob_equal = NULL,
	.ob_tostr = NULL,
  .numops = &float_ops,
};

TValue bool_and(TValue *v1, TValue *v2)
{
  TValue v;
  assert(v1->klazz == &Bool_Klass);
  assert(v2->klazz == &Bool_Klass);
  int b = VALUE_BOOL(v1) && VALUE_BOOL(v2);
  setbvalue(&v, b);
  return v;
}

TValue bool_or(TValue *v1, TValue *v2)
{
  TValue v;
  assert(v1->klazz == &Bool_Klass);
  assert(v2->klazz == &Bool_Klass);
  int b = VALUE_BOOL(v1) || VALUE_BOOL(v2);
  setbvalue(&v, b);
  return v;
}

TValue bool_not(TValue *v1)
{
  TValue v;
  assert(v1->klazz == &Bool_Klass);
  int b = !VALUE_BOOL(v1);
  setbvalue(&v, b);
  return v;
}

static NumberOperations bool_ops = {
  .land = bool_and,
  .lor  = bool_or,
  .lnot = bool_not,
};

Klass Bool_Klass = {
	OBJECT_HEAD_INIT(&Bool_Klass, &Klass_Klass)
	.name = "Bool",
	.ob_hash = NULL,
	.ob_equal = NULL,
	.ob_tostr = NULL,
  .numops = &bool_ops,
};

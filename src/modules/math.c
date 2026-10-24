/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <math.h>
#include "excobj.h"

#ifdef __cplusplus
extern "C" {
#endif

//
// Power and root functions
//

/*
func sqrt(x float) float
*/
static TValue math_sqrt(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    double x = kl_arg_float(0);
    if (x < 0.0) {
        raise_exc_str("math.sqrt: negative input");
        return error_value;
    }
    return float64_value(sqrt(x));
}

/*
func pow(x float, y float) float
*/
static TValue math_pow(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    double x = kl_arg_float(0);
    double y = kl_arg_float(1);
    return float64_value(pow(x, y));
}

//
// Trigonometric functions
//

/*
func sin(x float) float
*/
static TValue math_sin(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    double x = kl_arg_float(0);
    return float64_value(sin(x));
}

/*
func cos(x float) float
*/
static TValue math_cos(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    double x = kl_arg_float(0);
    return float64_value(cos(x));
}

/*
func tan(x float) float
*/
static TValue math_tan(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    double x = kl_arg_float(0);
    return float64_value(tan(x));
}

/*
func asin(x float) float
*/
static TValue math_asin(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    double x = kl_arg_float(0);
    if (x < -1.0 || x > 1.0) {
        raise_exc_str("math.asin: input out of range [-1, 1]");
        return error_value;
    }
    return float64_value(asin(x));
}

/*
func acos(x float) float
*/
static TValue math_acos(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    double x = kl_arg_float(0);
    if (x < -1.0 || x > 1.0) {
        raise_exc_str("math.acos: input out of range [-1, 1]");
        return error_value;
    }
    return float64_value(acos(x));
}

/*
func atan(x float) float
*/
static TValue math_atan(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    double x = kl_arg_float(0);
    return float64_value(atan(x));
}

/*
func atan2(y float, x float) float
*/
static TValue math_atan2(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    double y = kl_arg_float(0);
    double x = kl_arg_float(1);
    return float64_value(atan2(y, x));
}

//
// Exponential and logarithmic functions
//

/*
func exp(x float) float
*/
static TValue math_exp(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    double x = kl_arg_float(0);
    return float64_value(exp(x));
}

/*
func log(x float) float
*/
static TValue math_log(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    double x = kl_arg_float(0);
    if (x <= 0.0) {
        raise_exc_str("math.log: non-positive input");
        return error_value;
    }
    return float64_value(log(x));
}

/*
func log10(x float) float
*/
static TValue math_log10(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    double x = kl_arg_float(0);
    if (x <= 0.0) {
        raise_exc_str("math.log10: non-positive input");
        return error_value;
    }
    return float64_value(log10(x));
}

/*
func log2(x float) float
*/
static TValue math_log2(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    double x = kl_arg_float(0);
    if (x <= 0.0) {
        raise_exc_str("math.log2: non-positive input");
        return error_value;
    }
    return float64_value(log2(x));
}

//
// Predicate functions
//

/*
func isnan(x float) bool
*/
static TValue math_isnan(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    double x = kl_arg_float(0);
    return bool_value(isnan(x) ? 1 : 0);
}

/*
func isinf(x float) bool
*/
static TValue math_isinf(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    double x = kl_arg_float(0);
    return bool_value(isinf(x) ? 1 : 0);
}

//
// Registration table and init function
//

static MethodDef math_functions[] = {
    { "sqrt", math_sqrt },   { "pow", math_pow },
    { "sin", math_sin },     { "cos", math_cos },
    { "tan", math_tan },     { "asin", math_asin },
    { "acos", math_acos },   { "atan", math_atan },
    { "atan2", math_atan2 }, { "exp", math_exp },
    { "log", math_log },     { "log10", math_log10 },
    { "log2", math_log2 },   { "isnan", math_isnan },
    { "isinf", math_isinf }, { NULL },
};

void math_native_lib_init(NativeLib *lib)
{
    MethodDef *methdef = math_functions;
    while (methdef->name) {
        kl_reg_func(lib, methdef->name, methdef->cfunc);
        ++methdef;
    }
}

#ifdef __cplusplus
}
#endif

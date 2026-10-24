/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <stdlib.h>
#include <time.h>
#include "excobj.h"

#ifdef __cplusplus
extern "C" {
#endif

//
// Core random number generators
//

/*
func random() float
*/
static TValue random_random(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    // Return a random float in [0.0, 1.0)
    double r = (double)rand() / (double)RAND_MAX;
    return float64_value(r);
}

/*
func randint(a int, b int) int
*/
static TValue random_randint(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    int64_t a = kl_arg_int64(0);
    int64_t b = kl_arg_int64(1);
    
    if (a > b) {
        raise_exc_str("random.randint: a must be <= b");
        return error_value;
    }
    
    // Return a random integer in [a, b] (inclusive)
    int64_t range = b - a + 1;
    int64_t r = a + (rand() % range);
    return int64_value(r);
}

/*
func randrange(stop int) int
*/
static TValue random_randrange(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    int64_t stop = kl_arg_int64(0);
    
    if (stop <= 0) {
        raise_exc_str("random.randrange: stop must be > 0");
        return error_value;
    }
    
    // Return a random integer in [0, stop)
    int64_t r = rand() % stop;
    return int64_value(r);
}

/*
func randrange_step(start int, stop int, step int) int
*/
static TValue random_randrange_step(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 3);
    int64_t start = kl_arg_int64(0);
    int64_t stop = kl_arg_int64(1);
    int64_t step = kl_arg_int64(2);
    
    if (step == 0) {
        raise_exc_str("random.randrange_step: step must not be zero");
        return error_value;
    }
    
    // Calculate the number of possible values
    int64_t n;
    if (step > 0) {
        if (start >= stop) {
            raise_exc_str("random.randrange_step: empty range");
            return error_value;
        }
        n = (stop - start + step - 1) / step;
    } else {
        if (start <= stop) {
            raise_exc_str("random.randrange_step: empty range");
            return error_value;
        }
        n = (start - stop - step - 1) / (-step);
    }
    
    // Return a random value from the range
    int64_t idx = rand() % n;
    int64_t r = start + idx * step;
    return int64_value(r);
}

/*
func uniform(a float, b float) float
*/
static TValue random_uniform(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    double a = kl_arg_float(0);
    double b = kl_arg_float(1);
    
    if (a > b) {
        raise_exc_str("random.uniform: a must be <= b");
        return error_value;
    }
    
    // Return a random float in [a, b]
    double r = (double)rand() / (double)RAND_MAX;
    return float64_value(a + r * (b - a));
}

/*
func seed(s int)
*/
static TValue random_seed(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    int64_t s = kl_arg_int64(0);
    
    // Set the random seed
    srand((unsigned int)s);
    
    return no_value;
}

//
// Registration table and init function
//

static MethodDef random_functions[] = {
    { "random", random_random },
    { "randint", random_randint },
    { "randrange", random_randrange },
    { "randrange_step", random_randrange_step },
    { "uniform", random_uniform },
    { "seed", random_seed },
    { NULL },
};

void random_native_lib_init(NativeLib *lib)
{
    // Initialize random seed with current time
    srand((unsigned int)time(NULL));
    
    MethodDef *methdef = random_functions;
    while (methdef->name) {
        kl_reg_func(lib, methdef->name, methdef->cfunc);
        ++methdef;
    }
}

#ifdef __cplusplus
}
#endif

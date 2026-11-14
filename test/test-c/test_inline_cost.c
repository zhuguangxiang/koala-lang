/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "cmd.h"
#include "inline_cost.h"
#include "testhelp.h"

CompileOptions cmd_opt;

int main(void)
{
    RecursiveInlineCost cost;
    recursive_inline_cost_init(&cost, 16, 0, 1, 1);
    test_cond("size exactly at budget is accepted",
              recursive_inline_cost_allow_level(&cost, 1, 204));
    test_cond("size above budget is rejected", !recursive_inline_cost_allow_level(&cost, 2, 1));

    recursive_inline_cost_init(&cost, 1, 0, 0, 4);
    test_cond("block count exactly at budget is accepted",
              recursive_inline_cost_allow_level(&cost, 1, 204));
    test_cond("block count above budget is rejected",
              !recursive_inline_cost_allow_level(&cost, 2, 1));

    recursive_inline_cost_init(&cost, 1, 0, 0, 1);
    test_cond("depth eight is accepted", recursive_inline_cost_allow_level(&cost, 8, 1));
    test_cond("depth nine is rejected", !recursive_inline_cost_allow_level(&cost, 9, 1));

    test_cond("half percent probability is rejected", !recursive_inline_probability_allowed(50));
    test_cond("above half percent probability is accepted",
              recursive_inline_probability_allowed(51));
    test_cond("nested call probabilities multiply",
              recursive_inline_probability_product(5000, 2500) == 1250);
    test_report();
    return 0;
}

/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <stdio.h>
#include "excobj.h"
#include "modobj.h"

#ifdef __cplusplus
extern "C" {
#endif

static void print_test_progress(int err, int index, int total)
{
    static char results[4096]; // store all test results
    results[index] = err ? 'F' : '.';

    const int window = 50; // number of symbols to show
    int start = 0;

    // compute sliding window start
    if (total > window) {
        if (index + 1 <= window)
            start = 0;
        else
            start = (index + 1) - window;
    }

    printf("\r[");
    for (int i = start; i < start + window && i < total; i++) {
        char c = results[i];
        if (i > index) {
            printf(" "); // not executed yet
        } else if (c == 'F') {
            printf("\x1b[31mF\x1b[0m"); // red F
        } else {
            printf("\x1b[32m.\x1b[0m"); // green dot
        }
    }
    printf("]  %d/%d", index + 1, total);

    if (index + 1 == total) printf("\n");

    fflush(stdout);
}

int kl_run_tests(Object *_m)
{
    ModuleObject *m = (ModuleObject *)_m;

    Vector *tests = &m->tests;
    int total = vector_size(tests);

    printf("\nRunning %d tests in file '%s.kl'\n\n", total, m->path);

    // 1. run test cases, print progress and save result

    TestCase *_case;
    vector_foreach_ptr(_case, tests) {
        long long start_ns = now_ns();
        TValue val = kl_call_code(_case->co, NULL, 0);
        long long end_ns = now_ns();

        _case->elapsed_ns = end_ns - start_ns;

        int err = is_error(&val);

        // if there is no error, mark the test case as passed
        if (!err) {
            if (_case->expect_panic) {
                // expected a panic but the test passed without error
                _case->exc = NULL;
                _case->passed = 0;
                print_test_progress(1, i__, total);
            } else {
                _case->exc = NULL;
                _case->passed = 1;
                print_test_progress(0, i__, total);
            }
            continue;
        }

        Object *exc = pop_exc();
        ASSERT(exc);

        if (_case->expect_panic) {
            char *exc_str = kl_exc_get_msg(exc);
            if (str_equal(exc_str, _case->msg)) {
                // eat this exception as it matches the expected panic
                kl_free_exc(exc);
                _case->exc = NULL;
                _case->passed = 1;
                print_test_progress(0, i__, total);
                continue;
            }
            // fall through to unexpected exception handling
        }

        // unexpected exception
        print_test_progress(1, i__, total);
        _case->exc = exc;
        _case->passed = 0;
    }

    // 2. calculate total time and pass count

    long long total_ns = 0;
    int pass = 0;
    vector_foreach_ptr(_case, tests) {
        total_ns += _case->elapsed_ns;

        if (_case->passed) {
            ASSERT(_case->exc == NULL);
            pass++;
            continue;
        }

        if (_case->exc) {
            print_exc_and_free(_case->exc);
            _case->exc = NULL;
        } else {
            // no exception object, but the test failed for some other reason
            if (isatty(1)) {
                printf(
                    "\n\x1b[31mError:\x1b[0m Test '%s' failed.\n  Expected Exception:\n    %s\n  "
                    "But no exception was raised.\n",
                    _case->name, _case->msg);
                printf(
                    "\nHint: It seems this feature was recently implemented, but the negative\n"
                    "  test case was not updated.\n");
            } else {
                printf(
                    "\nError: Test '%s' failed.\n  Expected Exception:\n    %s\n  But no exception "
                    "was raised.\n",
                    _case->name, _case->msg);
                printf(
                    "\nHint: It seems this feature was recently implemented, but the negative\n"
                    "  test case was not updated.\n");
            }
        }
    }

    // 3. print result
    double total_ms = (double)total_ns / 1e6;
    int failed = total - pass;
    printf("\nResult: %d passed, %d failed in %.3f ms\n", pass, failed, total_ms);
    if (failed == 0) printf("\nAll tests passed.\n");

    return failed;
}

#ifdef __cplusplus
}
#endif

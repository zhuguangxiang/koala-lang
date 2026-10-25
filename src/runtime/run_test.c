/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <limits.h>
#include <stdio.h>
#include "excobj.h"
#include "modobj.h"

#ifdef __cplusplus
extern "C" {
#endif

static char test_results[4096]; // store all test results

static void render_progress(int index, int total)
{
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
        char c = test_results[i];
        if (i > index) {
            printf(" "); // not executed yet
        } else if (c == 'F') {
            printf("\x1b[31mF\x1b[0m"); // red F
        } else if (c == 'S') {
            printf("\x1b[33mS\x1b[0m"); // yellow S
        } else {
            printf("\x1b[32m.\x1b[0m"); // green dot
        }
    }
    printf("]  %d/%d", index + 1, total);

    if (index + 1 == total) printf("\n");

    fflush(stdout);
}

static void print_test_progress(int err, int index, int total, int all_bench)
{
    if (all_bench) return;
    test_results[index] = err ? 'F' : '.';
    render_progress(index, total);
}

static void print_test_progress_skip(int index, int total, int all_bench)
{
    if (all_bench) return;
    test_results[index] = 'S';
    render_progress(index, total);
}

static int kl_run_tests(Object *_m)
{
    ModuleObject *m = (ModuleObject *)_m;

    Vector *tests = &m->tests;
    int total = vector_size(tests);

    // check if all tests are bench tests
    int all_bench = 1;
    TestCase *_case;
    vector_foreach_ptr(_case, tests) {
        if (!_case->is_bench) {
            all_bench = 0;
            break;
        }
    }

    if (!all_bench) {
        printf("\nRunning %d tests in file '%s.kl'\n\n", total, m->path);
    }

    // 1. run test cases, print progress and save result

    vector_foreach_ptr(_case, tests) {
        // skip tests marked with @skip
        if (_case->skipped) {
            _case->elapsed_ns = 0;
            _case->passed = 1;
            print_test_progress_skip(i__, total, all_bench);
            continue;
        }

        // handle bench tests
        if (_case->is_bench && _case->bench_config) {
            BenchConfig *cfg = _case->bench_config;
            int warmup = cfg->warmup;
            int runs = cfg->runs;
            int bench_failed = 0;

            // warmup phase
            for (int w = 0; w < warmup; w++) {
                TValue val = kl_call_code(_case->co, NULL, 0);
                if (is_error(&val)) {
                    Object *exc = pop_exc();
                    print_test_progress(1, i__, total, all_bench);
                    _case->exc = exc;
                    _case->passed = 0;
                    bench_failed = 1;
                    break;
                }
            }

            if (!bench_failed) {
                // measurement phase
                long long start_ns = now_ns();
                for (int r = 0; r < runs; r++) {
                    long long run_start = now_ns();
                    TValue val = kl_call_code(_case->co, NULL, 0);
                    long long run_end = now_ns();
                    long long run_time = run_end - run_start;
                    vector_push_back(&_case->bench_times, &run_time);
                    if (is_error(&val)) {
                        Object *exc = pop_exc();
                        print_test_progress(1, i__, total, all_bench);
                        _case->exc = exc;
                        _case->passed = 0;
                        bench_failed = 1;
                        break;
                    }
                }
                long long end_ns = now_ns();

                if (!bench_failed) {
                    _case->elapsed_ns = end_ns - start_ns;
                    _case->passed = 1;
                    print_test_progress(0, i__, total, all_bench);
                }
            }
            continue;
        }

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
                print_test_progress(1, i__, total, all_bench);
            } else {
                _case->exc = NULL;
                _case->passed = 1;
                print_test_progress(0, i__, total, all_bench);
            }
            continue;
        }

        Object *exc = pop_exc();
        ASSERT(exc);

        if (_case->expect_panic) {
            char *exc_str = kl_exc_get_msg(exc);
            if (!_case->msg || (exc_str && str_equal(exc_str, _case->msg))) {
                // eat this exception as it matches the expected panic
                kl_free_exc(exc);
                _case->exc = NULL;
                _case->passed = 1;
                print_test_progress(0, i__, total, all_bench);
                continue;
            }
            // fall through to unexpected exception handling
        }

        // unexpected exception
        print_test_progress(1, i__, total, all_bench);
        _case->exc = exc;
        _case->passed = 0;
    }

    // 2. calculate total time and pass count

    long long total_ns = 0;
    int pass = 0;
    int skipped = 0;
    vector_foreach_ptr(_case, tests) {
        total_ns += _case->elapsed_ns;

        if (_case->skipped) {
            skipped++;
            continue;
        }

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
                    _case->name, _case->msg ? _case->msg : "(any)");
                printf(
                    "\nHint: It seems this feature was recently implemented, but the negative\n"
                    "  test case was not updated.\n");
            } else {
                printf(
                    "\nError: Test '%s' failed.\n  Expected Exception:\n    %s\n  But no exception "
                    "was raised.\n",
                    _case->name, _case->msg ? _case->msg : "(any)");
                printf(
                    "\nHint: It seems this feature was recently implemented, but the negative\n"
                    "  test case was not updated.\n");
            }
        }
    }

    // 3. print result (skip for all-bench mode)
    int failed = total - pass - skipped;
    if (!all_bench) {
        double total_ms = (double)total_ns / 1e6;
        if (skipped > 0) {
            printf("\nResult: %d passed, %d failed, %d skipped in %.3f ms\n", pass, failed, skipped,
                   total_ms);
        } else {
            printf("\nResult: %d passed, %d failed in %.3f ms\n", pass, failed, total_ms);
        }
        if (failed == 0) printf("\nAll tests passed.\n");
    }

    // 4. print bench results (pytest-benchmark style)
    int bench_count = 0;
    int max_name_len = 4;          // minimum "Name"
    const int MAX_NAME_WIDTH = 40; // max width for test name

    vector_foreach_ptr(_case, tests) {
        if (_case->is_bench && _case->passed) {
            bench_count++;
            int name_len = strlen(_case->name);
            if (name_len > max_name_len && name_len <= MAX_NAME_WIDTH) {
                max_name_len = name_len;
            }
        }
    }

    if (max_name_len > MAX_NAME_WIDTH) {
        max_name_len = MAX_NAME_WIDTH;
    }

    if (bench_count > 0) {
        printf("\nBenchmark results:\n\n");
        printf("  %-*.*s  %10s  %10s  %10s  %s\n", max_name_len, max_name_len, "Name", "Min", "Max",
               "Mean", "Rounds");
        printf("  ");
        for (int i = 0; i < max_name_len; i++) printf("-");
        printf("  ----------  ----------  ----------  ------\n");
        vector_foreach_ptr(_case, tests) {
            if (_case->is_bench && _case->passed) {
                int runs = vector_size(&_case->bench_times);
                if (runs == 0) continue;

                // calculate min, max, mean
                long long min_ns = LLONG_MAX, max_ns = 0, sum_ns = 0;
                long long t;
                vector_foreach(t, &_case->bench_times) {
                    if (t < min_ns) min_ns = t;
                    if (t > max_ns) max_ns = t;
                    sum_ns += t;
                }
                double mean_ns = (double)sum_ns / runs;

                // auto-select unit for each test case
                const char *unit;
                double scale;
                if (max_ns < 1000) {
                    unit = "ns";
                    scale = 1.0;
                } else if (max_ns < 1000000) {
                    unit = "us";
                    scale = 1000.0;
                } else {
                    unit = "ms";
                    scale = 1000000.0;
                }

                printf("  %-*.*s  %7.3f %-2s  %7.3f %-2s  %7.3f %-2s  %d\n", max_name_len,
                       max_name_len, _case->name, min_ns / scale, unit, max_ns / scale, unit,
                       mean_ns / scale, unit, runs);
            }
        }
        puts("");
    }

    return failed;
}

int koala_test_file(char *path)
{
    Object *m = kl_load_module(path);
    if (m) return kl_run_tests(m);
    fprintf(stderr, "koala: failed to load module '%s'\n", path);
    return -1;
}

#ifdef __cplusplus
}
#endif

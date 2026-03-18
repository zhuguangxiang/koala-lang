/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "pass.h"
#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline void print_pass_after_header(const char *pass_name, int iteration, FILE *fp)
{
    fprintf(fp, "--- IR Dump After %s (Iteration %d) ---\n", pass_name, iteration);
}

static int pm_run(KlrFunc *fn, void *data)
{
    KlrPassManager *pm = (KlrPassManager *)data;
    int total_changed = 0;
    int changed = 1;
    int iteration = 0;

    char iter_str[64];

    while (changed && iteration < 10) {
        ++iteration;
        changed = 0;

        for (int i = 0; i < pm->count; i++) {
            KlrPass *p = pm->passes[i];
            int pass_changed = p->run(fn, p->data);

            if (p->dump) {
                print_pass_after_header(p->name, iteration, stdout);
                klr_print_func(fn, stdout);
            }

            changed |= pass_changed;
        }

        total_changed |= changed;
    }

    return total_changed;
}

void pm_init(KlrPassManager *pm, const char *name)
{
    pm->capacity = 8;
    pm->count = 0;
    pm->passes = malloc(sizeof(KlrPass *) * pm->capacity);
    pm->name = name;
    pm->run = pm_run;
    pm->data = pm;
}

void pm_fini(KlrPassManager *pm) { free(pm->passes); }

void pm_add_pass(KlrPassManager *pm, KlrPass *pass, int dump)
{
    if (pm->count == pm->capacity) {
        pm->capacity += 8;
        pm->passes = realloc(pm->passes, sizeof(KlrPass *) * pm->capacity);
    }
    pm->passes[pm->count++] = pass;
    pass->dump = dump;
}

void pm_add_pm_as_pass(KlrPassManager *pm, KlrPassManager *pm_pass, int dump)
{
    KlrPass *pass = (KlrPass *)pm_pass;
    pm_add_pass(pm, pass, dump);
}

#ifdef __cplusplus
}
#endif

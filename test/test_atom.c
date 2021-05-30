/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 */

#include "util/atom.h"
#include "util/common.h"

#ifdef __cplusplus
extern "C" {
#endif

int main(int argc, char *argv[])
{
    init_atom();

    char *s = atom("Hello,");
    assert(!strcmp(s, "Hello,"));
    char *s2 = atom_str(" Koala", 6);
    assert(!strcmp(s2, " Koala"));
    char *s3 = atom("Hello,");
    assert(s == s3);
    char *s4 = atom_str(" Koala", 6);
    assert(s2 == s4);
    char *s5 = atom_vstr(2, s, s2);
    assert(!strcmp(s5, "Hello, Koala"));

    fini_atom();
    return 0;
}

#ifdef __cplusplus
}
#endif

/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2018-2022 James <zhuguangxiang@gmail.com>
 */

#include "common/atom.h"
#include "common/common.h"

int main(int argc, char *argv[])
{
    UNUSED(argc);
    UNUSED(argv);
    init_atom();

    // char *s = AtomStr("Hello,");
    // assert(!strcmp(s, "Hello,"));
    // char *s2 = AtomNStr(" Koala", 6);
    // assert(!strcmp(s2, " Koala"));
    // char *s3 = AtomStr("Hello,");
    // assert(s == s3);
    // char *s4 = AtomNStr(" Koala", 6);
    // assert(s2 == s4);
    // char *s5 = AtomVStr(2, s, s2);
    // assert(!strcmp(s5, "Hello, Koala"));

    fini_atom();
    return 0;
}

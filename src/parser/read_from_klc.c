/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "atom.h"
#include "klc.h"
#include "log.h"
#include "mm.h"
#include "symbol.h"

#ifdef __cplusplus
extern "C" {
#endif

void kl_read_from_klc(HashMap *stbl, char *path)
{
    KlcFile klc;
    init_klc_file(&klc, path);
    read_klc_file(&klc, 0);
    klc_dump(&klc);
    fini_klc_file(&klc);
}

#ifdef __cplusplus
}
#endif

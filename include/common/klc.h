/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_KLC_H_
#define _KOALA_KLC_H_

#include "codespec.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _KlcFile {
    uint8_t magic[4];
    uint32_t version;
    uint16_t num_symbols;
    uint16_t num_relocs;
    uint16_t num_consts;
    uint16_t num_codes;
    const char *path;
    FILE *filp;
    Vector objs;
} KlcFile;

void klc_add_var(KlcFile *klc, const char *name, const char *desc, int has_value);
void klc_add_func(KlcFile *klc, const char *name, const char *desc);

void klc_add_int(KlcFile *klc, int64_t val, int len);
void klc_add_code(KlcFile *klc, CodeSpec *cs);

void init_klc_file(KlcFile *klc, const char *path);
void fini_klc_file(KlcFile *klc);
int write_klc_file(KlcFile *klc);
int read_klc_file(KlcFile *klc, int only_meta);
void klc_dump(KlcFile *klc);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_KLC_H_ */

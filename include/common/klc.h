/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_KLC_H_
#define _KOALA_KLC_H_

#include "codespec.h"
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _KlcFile {
    uint8_t magic[4];
    uint32_t version;
    uint8_t num_vars;
    uint8_t num_funcs;
    uint8_t num_types;
    uint8_t num_relocs;
    uint16_t num_codes;
    const char *path;
    FILE *filp;
    Vector objs;
} KlcFile;

void klc_add_int32(KlcFile *klc, int32_t val);
void klc_add_code(KlcFile *klc, CodeSpec *cs);

void init_klc_file(KlcFile *klc, const char *path);
int write_klc_file(KlcFile *klc);
int read_klc_file(KlcFile *klc);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_KLC_H_ */

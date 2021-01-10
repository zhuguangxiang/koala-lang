/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangxiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_CODE_FORMAT_H_
#define _KOALA_CODE_FORMAT_H_

#include "common.h"
#include "typedesc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct klc_header {
    uint8_t magic[4];
    uint8_t version[4];
} klc_header_t;

#define SECT_CUSTOM_ID 0
#define SECT_STRING_ID 1
#define SECT_TYPE_ID   2
#define SECT_VAR_ID    3
#define SECT_FUNC_ID   4
#define SECT_CODE_ID   5
#define SECT_KLASS_ID  6
#define SECT_MAX_ID    7

typedef struct klc_sect {
    uint8_t id;
    uint32_t size;
    Vector vec;
} klc_sect_t;

typedef struct klc_str {
    uint32_t len;
    char *s;
} klc_str_t;

#define KLC_TYPE_HEAD char kind;

typedef struct klc_type {
    KLC_TYPE_HEAD
} klc_type_t;

typedef struct klc_type_proto {
    KLC_TYPE_HEAD
    uint16_t ret;
    uint16_t count;
    uint16_t para[0];
} klc_type_proto_t;

typedef struct klc_type_klass {
    KLC_TYPE_HEAD
    uint16_t name;
    uint16_t path;
} klc_type_klass_t;

typedef struct klc_var {
    uint16_t name;
    uint16_t type;
} klc_var_t;

typedef struct klc_func {
    uint16_t name;
    uint16_t type;
    uint16_t code;
} klc_func_t;

typedef struct klc_code {
    uint32_t size;
    uint8_t codes[0];
} klc_code_t;

typedef struct klc_file {
    HashMap strtab;
    char magic[4];
    uint8_t version[4];
    klc_sect_t strs;
    klc_sect_t types;
    klc_sect_t vars;
    klc_sect_t funcs;
    klc_sect_t codes;
} klc_file_t;

klc_file_t *klc_create(void);
void klc_flush(klc_file_t *klc, char *path);
void klc_read(klc_file_t *klc, char *path);
void klc_destroy(klc_file_t *klc);
void klc_show(klc_file_t *klc);

void klc_add_byte(klc_file_t *klc, int8_t val);
void klc_add_int(klc_file_t *klc, int64_t val);
void klc_add_float(klc_file_t *klc, double val);
void klc_add_bool(klc_file_t *klc, int8_t val);
void klc_add_char(klc_file_t *klc, int32_t val);
void klc_add_string(klc_file_t *klc, char *val);

void klc_add_type(klc_file_t *klc, TypeDesc *type);

void klc_add_var(klc_file_t *klc, char *name, TypeDesc *type);
void klc_add_func(
    klc_file_t *klc, char *name, TypeDesc *type, klc_code_t *code);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_CODE_FORMAT_H_ */

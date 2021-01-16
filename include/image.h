/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangxiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_CODE_IMAGE_H_
#define _KOALA_CODE_IMAGE_H_

#include "common.h"
#include "typedesc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VERSION_MAJOR 0
#define VERSION_MINOR 9

#define SECT_CUSTOM  0
#define SECT_STR     1
#define SECT_LITERAL 2
#define SECT_TYPE    3
#define SECT_VAR     4
#define SECT_FUNC    5
#define SECT_ANONY   6
#define SECT_CODE    7
#define SECT_CLASS   8
#define SECT_TRAIT   9
#define SECT_ENUM    10
#define SECT_MAX     11

typedef struct klc_header {
    uint8_t magic[4];
    uint16_t major;
    uint16_t minor;
} klc_header_t;

typedef struct klc_sect {
    uint8_t id;
    Vector vec;
} klc_sect_t;

typedef struct klc_str {
    uint16_t len;
    char *s;
} klc_str_t;

#define LITERAL_NIL     0
#define LITERAL_INT8    1
#define LITERAL_INT16   2
#define LITERAL_INT32   3
#define LITERAL_INT64   4
#define LITERAL_FLOAT32 5
#define LITERAL_FLOAT64 6
#define LITERAL_BOOL    7
#define LITERAL_STRING  8
#define LITERAL_CHAR    9

typedef struct klc_literal {
    uint8_t tag;
    union {
        int8_t i8;
        int16_t i16;
        int32_t i32;
        int64_t i64;
        float f32;
        double f64;
        int8_t bval;
        int16_t str;
        int32_t ch;
    };
} klc_literal_t;

typedef struct klc_type {
    uint8_t tag;
} klc_type_t;

typedef struct klc_type_array {
    uint8_t tag;
    int16_t subtype;
} klc_type_array_t;

typedef struct klc_type_dict {
    uint8_t tag;
    int16_t ktype;
    int16_t vtype;
} klc_type_dict_t;

typedef struct klc_type_klass {
    uint8_t tag;
    int16_t path;
    int16_t name;
} klc_type_klass_t;

typedef struct klc_type_proto {
    uint8_t tag;
    int16_t rtype;
    Vector *ptypes;
} klc_type_proto_t;

#define ACCESS_FLAGS_PUB   0x01
#define ACCESS_FLAGS_FINAL 0x02

typedef struct klc_var {
    uint8_t flags;
    int16_t name;
    int16_t type;
} klc_var_t;

typedef struct klc_func {
    uint8_t flags;
    int16_t name;
    int16_t type;
    int16_t code;
    Vector paratypes;
    Vector locals;
    Vector freevars;
} klc_func_t;

typedef struct klc_anony {
    int16_t type;
    int16_t code;
    Vector locals;
    Vector freevars;
    Vector upvars;
} klc_anony_t;

typedef struct klc_code {
    uint32_t size;
    uint8_t codes[0];
} klc_code_t;

typedef struct klc_ifunc {
    int16_t name;
    int16_t type;
} klc_ifunc_t;

typedef struct klc_class {
    uint8_t flags;
    int16_t name;
    int16_t base;
    Vector paratypes;
    Vector traits;
    Vector fields;
    Vector methods;
} klc_class_t;

typedef struct klc_trait {
    uint8_t flags;
    int16_t name;
    Vector paratypes;
    Vector traits;
    Vector methods;
    Vector ifuncs;
} klc_trait_t;

typedef struct klc_enum {
    uint8_t flags;
    int16_t name;
    Vector paratypes;
    Vector labels;
    Vector methods;
} klc_enum_t;

typedef struct klc_label {
    int16_t name;
    Vector types;
    int32_t value;
} klc_label_t;

typedef struct klc_para_type {
    int16_t name;
    Vector bounds;
} klc_para_type_t;

typedef struct klc_image {
    HashMap strtab;
    HashMap typtab;
    klc_sect_t strs;
    klc_sect_t literals;
    klc_sect_t types;
    klc_sect_t vars;
    klc_sect_t funcs;
    klc_sect_t anonies;
    klc_sect_t codes;
    klc_sect_t classes;
    klc_sect_t traits;
    klc_sect_t enums;
} klc_image_t;

klc_image_t *klc_create(void);
void klc_destroy(klc_image_t *klc);
void klc_write_file(klc_image_t *klc, char *path);
klc_image_t *klc_read_file(char *path);
void klc_show(klc_image_t *klc);

void klc_add_var(klc_image_t *klc, char *name, TypeDesc *type, uint8_t flags);

typedef struct codeinfo {
    char *name;
    uint8_t flags;
    TypeDesc *desc;
    int size;
    uint8_t *codes;
    Vector *locvec;
    Vector *freevec;
    Vector *upvec;
} codeinfo_t;

void klc_add_func(klc_image_t *klc, codeinfo_t *ci);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_CODE_IMAGE_H_ */

/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2022-2032 James <zhuguangxiang@gmail.com>
 */

#ifndef _KOALA_MODULE_H_
#define _KOALA_MODULE_H_

#include "hashmap.h"
#include "opcode.h"
#include "typedesc.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

/* forward declaration */
typedef struct _KlModule KlModule;
typedef struct _KlConstInfo KlConstInfo;
typedef struct _KlReloInfo KlReloInfo;

/* per dot-klc file */
struct _KlModule {
    /* module path */
    char *path;
    /* module name */
    char *name;
    /* meta info */
    HashMap *mtbl;
    /* constants */
    int num_consts;
    KlConstInfo *consts;
    /* relocations */
    int num_relocs;
    KlReloInfo *relocs;
    /* strings */
    int str_size;
    uint8_t *strs;
    /* global variables */
    int global_size;
    uint32_t *globals;
};

/* const info */
struct _KlConstInfo {
    /* constant kind */
    uint8_t const_kind;
#define CONST_KIND_INT        1
#define CONST_KIND_FLOAT      2
#define CONST_KIND_BYTE_ARRAY 3
    /* byte array size */
    uint16_t byte_array_size;
    union {
        uintptr_t int_val;
        double float_val;
        /* literal string as byte array */
        uint8_t *byte_array_val;
    } v;
};

/* reloc info */
struct _KlReloInfo {
    char *mod_name;
    char *sym_name;
    char *sym_addr;
};

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_MODULE_H_ */

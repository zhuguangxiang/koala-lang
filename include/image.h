/*
 MIT License

 Copyright (c) 2018 James, https://github.com/zhuguangxiang

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
*/

#ifndef _KOALA_IMAGE_H_
#define _KOALA_IMAGE_H_

#include <inttypes.h>
#include "hashmap.h"
#include "typedesc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ITEM_MAP        0
#define ITEM_STRING     1
#define ITEM_LITERAL    2
#define ITEM_TYPE       3
#define ITEM_INDEX      4
#define ITEM_CONST      5
#define ITEM_LOCVAR     6
#define ITEM_VAR        7
#define ITEM_CONSTVAR   8
#define ITEM_FUNC       9
#define ITEM_ANONY      10
#define ITEM_CODE       11
#define ITEM_CLASS      12
#define ITEM_FIELD      13
#define ITEM_METHOD     14
#define ITEM_TRAIT      15
#define ITEM_IFUNC      16
#define ITEM_ENUM       17
#define ITEM_LABEL      18
#define ITEM_MBR        19
#define ITEM_MAX        20

typedef struct mapitem {
  /* one of ITEM_XXX, self is ITEM_MAP */
  uint16_t type;
  uint16_t unused;
  uint32_t offset;
  int32_t size;
} MapItem;

typedef struct stringitem {
  int32_t length;
  char data[0];
} StringItem;

typedef struct typeitem {
  int32_t kind;           /* see: DescKind in typedesc.h */
  union {
    char base;            /* base type */
    struct {
      int32_t typeindex;  /* ->StringItem */
      int32_t pathindex;  /* ->StringItem */
      int32_t typesindex; /* ->IndexItem */
    } klass;
    struct {
      int32_t pindex;     /* ->IndexItem */
      int32_t rindex;     /* ->TypeItem */
    } proto;
    struct {
      int32_t value;      /* index value */
      int32_t nameindex;  /* ->StringItem */
    } pararef;
    struct {
      int32_t value;      /* index value */
      int32_t nameindex;  /* ->StringItem */
      int32_t typesindex; /* ->IndexItem */
    } paradef;
  };
} TypeItem;

#define INDEX_TYPELIST  1
#define INDEX_VALUE     2
typedef struct indexitem {
  int32_t size;
  int32_t kind;
  int32_t index[0]; /* ->TypeItem(value only) */
} IndexItem;

#define LITERAL_INT     1
#define LITERAL_FLOAT   2
#define LITERAL_BOOL    3
#define LITERAL_STRING  4
#define LITERAL_UCHAR   5

typedef struct literalitem {
  int type;
  union {
    int64_t ival;   /* int64 */
    double fval;    /* float64 */
    int bval;       /* bool */
    int32_t index;  /* ->StringItem */
    wchar wch;      /* unicode char */
  };
} LiteralItem;

#define CONST_LITERAL   1
#define CONST_TYPE      2
#define CONST_ANONY     3
typedef struct constitem {
  int32_t kind;       /* CONST_XXX */
  int32_t index;      /* index of literal, type or anonymous */
} ConstItem;

typedef struct localvaritem {
  int32_t nameindex;  /* ->StringItem */
  int32_t typeindex;  /* ->TypeItem */
  int32_t index;      /* ->index of locvars */
} LocVarItem;

typedef struct varitem {
  int32_t nameindex;  /* ->StringItem */
  int32_t typeindex;  /* ->TypeItem */
} VarItem;

typedef struct constvaritem {
  int32_t nameindex;  /* ->StringItem */
  int32_t typeindex;  /* ->TypeItem */
  int32_t index;      /* ->LiteralItem */
} ConstVarItem;

typedef struct funcitem {
  int32_t nameindex;  /* ->StringItem */
  int32_t pindex;     /* ->IndexItem */
  int32_t rindex;     /* ->TypeItem */
  int32_t codeindex;  /* ->CodeItem */
  int32_t locindex;   /* ->IndexItem */
  int32_t freeindex;  /* ->IndexItem */
} FuncItem;

typedef struct anonyitem {
  int32_t nameindex;  /* ->StringItem */
  int32_t pindex;     /* ->IndexItem */
  int32_t rindex;     /* ->TypeItem */
  int32_t codeindex;  /* ->CodeItem */
  int32_t locindex;   /* ->IndexItem */
  int32_t freeindex;  /* ->IndexItem */
  int32_t upindex;    /* ->IndexItem */
} AnonyItem;

typedef struct codeitem {
  int32_t size;
  uint8_t codes[0];
} CodeItem;

typedef struct classitem {
  int32_t nameindex;   /* ->StringItem */
  int32_t parasindex;  /* ->IndexItem */
  int32_t basesindex;  /* ->IndexItem */
  int32_t mbrindex;    /* ->IndexItem */
} ClassItem;

typedef struct ifuncitem {
  int32_t nameindex;  /* ->StringItem */
  int32_t pindex;     /* ->IndexItem */
  int32_t rindex;     /* ->TypeItem */
} IFuncItem;

typedef struct enumitem {
  int32_t nameindex;   /* ->StringItem */
  int32_t parasindex;  /* ->IndexItem */
  int32_t mbrindex;    /* ->IndexItem */
} EnumItem;

typedef struct labelitem {
  int32_t nameindex;  /* ->StringItem */
  int32_t index;      /* ->IndexItem */
  int32_t value;      /* integer value */
} LabelItem;

#define MBR_FIELD   1
#define MBR_METHOD  2
#define MBR_IFUNC   3
#define MBR_LABEL   4
typedef struct mbrindex {
  int kind;
  int index;
} MbrIndex;

typedef struct mbritem {
  int size;
  MbrIndex indexes[0];
} MbrItem;

#define PKG_NAME_MAX 32

/* koala byte code image header */
typedef struct image_header {
  uint8_t  magic[4];
  uint8_t  version[4];
  uint32_t file_size;
  uint32_t header_size;
  uint32_t endian_tag;
  uint32_t map_offset;
  uint32_t map_size;
  uint32_t crc32;
  char name[PKG_NAME_MAX];
} ImageHeader;

typedef struct image {
  ImageHeader header;
  /* hash table for unique data */
  HashMap map;
  /* items' array size */
  int size;
  /* vector array */
  Vector items[0];
} Image;

typedef struct codeinfo {
  char *name;
  TypeDesc *desc;
  uint8_t *codes;
  int size;
  Vector *locvec;
  Vector *freevec;
  Vector *upvec;
} CodeInfo;

typedef struct locvar {
  char *name;
  TypeDesc *desc;
  int index;
} LocVar;

LocVar *locvar_new(char *name, TypeDesc *desc, int index);
void locvar_free(LocVar *loc);

int image_add_integer(Image *image, int64_t val);
int image_add_float(Image *image, double val);
int image_add_bool(Image *image, int val);
int image_add_string(Image *image, char *val);
int image_add_uchar(Image *image, wchar val);
int image_add_literal(Image *image, Literal *val);
int image_add_desc(Image *image, TypeDesc *desc);
int image_add_anony(Image *image, CodeInfo *ci);
void image_add_var(Image *image, char *name, TypeDesc *desc);
void image_add_kvar(Image *image, char *name, TypeDesc *desc, Literal *val);
int image_add_func(Image *image, CodeInfo *ci);
void image_add_class(Image *image, char *name, Vector *typeparas,
                    Vector *bases, int mbrindex);
void image_add_trait(Image *image, char *name, Vector *bases, int mbrindex);
void image_add_enum(Image *image, char *name, int mbrindex);
int image_add_field(Image *image, char *name, TypeDesc *desc);
int image_add_method(Image *image, CodeInfo *ci);
int image_add_ifunc(Image *image, char *name, TypeDesc *desc);
int image_add_label(Image *image, char *name, Vector *types, int32_t val);
int image_add_mbrs(Image *image, MbrIndex *indexes, int size);

Image *image_new(char *name);
void image_free(Image *image);
void image_show(Image *image);
void image_finish(Image *image);
void image_write_file(Image *image, char *path);
/* flags is ITEM_XXX bits, marked not load */
Image *image_read_file(char *path, int unload);

int _size_(Image *image, int type);
typedef void (*getconstfunc)(void *, int, int, void *);
void image_load_consts(Image *image, getconstfunc func, void *arg);
typedef void (*getclassfunc)(char *, int, int, Image *, void *);
void image_load_class(Image *image, int index, getclassfunc func, void *arg);
void image_load_trait(Image *image, int index, getclassfunc func, void *arg);
void image_load_enum(Image *image, int index, getclassfunc func, void *arg);
typedef void (*getbasefunc)(TypeDesc *desc, void *arg);
void image_load_bases(Image *image, int index, getbasefunc func, void *arg);
typedef void (*getmbrfunc)(char *, int, void *, void *);
void image_load_mbrs(Image *image, int index, getmbrfunc func, void *arg);
typedef void (*getvarfunc)(char *, int, TypeDesc *, void *);
void image_load_var(Image *image, int index, getvarfunc func, void *arg);
void image_load_constvar(Image *image, int index, getvarfunc func, void *arg);
typedef void (*getfuncfunc)(char *, CodeInfo *, void *);
void image_load_func(Image *image, int index, getfuncfunc func, void *arg);
#define IMAGE_LOAD_ITEMS(image, kind, which, func, arg) \
({ \
  int size = _size_(image, kind); \
  for (int i = 0; i < size; ++i) \
    image_load_##which(image, i, func, arg); \
})
#define IMAGE_LOAD_VARS(image, func, arg) \
  IMAGE_LOAD_ITEMS(image, ITEM_VAR, var, func, arg)
#define IMAGE_LOAD_FUNCS(image, _func, arg) \
  IMAGE_LOAD_ITEMS(image, ITEM_FUNC, func, _func, arg)
#define IMAGE_LOAD_CONSTVARS(image, func, arg) \
  IMAGE_LOAD_ITEMS(image, ITEM_CONSTVAR, constvar, func, arg)
#define IMAGE_LOAD_CLASSES(image, func, arg) \
  IMAGE_LOAD_ITEMS(image, ITEM_CLASS, class, func, arg)
#define IMAGE_LOAD_TRAITS(image, func, arg) \
  IMAGE_LOAD_ITEMS(image, ITEM_TRAIT, trait, func, arg)
#define IMAGE_LOAD_ENUMS(image, func, arg) \
  IMAGE_LOAD_ITEMS(image, ITEM_ENUM, enum, func, arg)

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_IMAGE_H_ */

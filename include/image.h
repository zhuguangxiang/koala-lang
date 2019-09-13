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
#define ITEM_TYPELIST   4
#define ITEM_CONST      5
#define ITEM_LOCVAR     6
#define ITEM_VAR        7
#define ITEM_FUNC       8
#define ITEM_CODE       9
#define ITEM_CLASS      10
#define ITEM_FIELD      11
#define ITEM_METHOD     12
#define ITEM_TRAIT      13
#define ITEM_NFUNC      14
#define ITEM_IMETH      15
#define ITEM_ENUM       16
#define ITEM_EVAL       17
#define ITEM_MAX        18

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
  int kind;               /* see: DescKind in typedesc.h */
  union {
    char base;            /* base type */
    struct {
      int32_t pathindex;  /* ->StringItem */
      int32_t typeindex;  /* ->StringItem */
      int32_t typesindex; /* ->TypeListItem */
    };
    struct {
      int32_t pindex;     /* ->TypeListItem */
      int32_t rindex;     /* ->TypeItem */
    };
    struct {
      int32_t typeindex;  /* ->TypeItem */
    } varg;
  };
} TypeItem;

typedef struct typelistitem {
  int32_t size;
  int32_t index[0]; /* ->TypeItem */
} TypeListItem;

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
#define CONST_TYPELIST  3

typedef struct constitem {
  int kind;
  int32_t index;
} ConstItem;

typedef struct localvaritem {
  int32_t nameindex;  /* ->StringItem */
  int32_t typeindex;  /* ->TypeItem */
  int32_t pos;        /* ->index of locvars */
  int32_t index;      /* ->Index of FuncItem or MethodItem */
} LocVarItem;

typedef struct varitem {
  int32_t nameindex;  /* ->StringItem */
  int32_t typeindex;  /* ->TypeItem */
  int konst;          /* ->constant */
  int32_t index;      /* constant index */
} VarItem;

typedef struct funcitem {
  int32_t nameindex;  /* ->StringItem */
  int32_t pindex;     /* ->TypeListItem */
  int32_t rindex;     /* ->TypeItem */
  int32_t codeindex;  /* ->CodeItem */
  int32_t nrlocals;   /* number of locals */
} FuncItem;

typedef struct codeitem {
  int32_t size;
  uint8_t codes[0];
} CodeItem;

typedef struct classitem {
  int32_t classindex;  /* ->TypeItem */
  int32_t superindex;  /* ->TypeItem */
} ClassItem;

typedef struct feilditem {
  int32_t nameindex;  /* ->StringItem */
  int32_t typeindex;  /* ->TypeItem */
  int32_t classindex; /* ->ClassItem */
} FieldItem;

typedef struct methoditem {
  int32_t nameindex;  /* ->StringItem */
  int32_t pindex;     /* ->TypeListItem */
  int32_t rindex;     /* ->TypeItem */
  int32_t classindex; /* ->TypeItem */
  int32_t codeindex;  /* ->CodeItem */
  int32_t nrlocals;   /* number of locals */
} MethodItem;

typedef struct traititem {
  int32_t classindex;  /* ->TypeItem */
  int32_t traitsindex; /* ->TypeListItem */
} TraitItem;

typedef struct nfuncitem {
  int32_t nameindex;  /* ->StringItem */
  int32_t pindex;     /* ->TypeListItem */
  int32_t rindex;     /* ->TypeItem */
  int32_t classindex; /* ->TypeItem */
} NFuncItem;

typedef struct imethitem {
  int32_t nameindex;  /* ->StringItem */
  int32_t pindex;     /* ->TypeListItem */
  int32_t rindex;     /* ->TypeItem */
  int32_t classindex; /* ->TypeItem */
} IMethItem;

typedef struct enumitem {
  int32_t classindex;  /* ->TypeItem */
} EnumItem;

typedef struct evalitem {
  int32_t nameindex;  /* ->StringItem */
  int32_t classindex; /* ->TypeItem */
  int32_t index;      /* ->TypeListItem */
  int32_t value;      /* enum integer value */
} EValItem;

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

int Image_Add_Integer(Image *image, int64_t val);
int Image_Add_Float(Image *image, double val);
int Image_Add_Bool(Image *image, int val);
int Image_Add_String(Image *image, char *val);
int Image_Add_UChar(Image *image, wchar val);
int Image_Add_Literal(Image *image, Literal *val);
int Image_Add_Desc(Image *image, TypeDesc *desc);
int Image_Add_DescList(Image *image, Vector *vec);

void Image_Add_Var(Image *image, char *name, TypeDesc *desc);
void Image_Add_Const(Image *image, char *name, TypeDesc *desc,
                     Literal *val);
void Image_Add_LocVar(Image *image, char *name, TypeDesc *desc,
                      int pos, int index);
int Image_Add_Func(Image *image, char *name, TypeDesc *proto,
                   uint8_t *codes, int size, int locals);
void Image_Add_Class(Image *image, char *name, Vector *supers);
void Image_Add_Trait(Image *image, char *name, Vector *traits);
void Image_Add_Enum(Image *image, char *name);
void Image_Add_Field(Image *image, char *klazz, char *name, TypeDesc *desc);
int Image_Add_Method(Image *image, char *klazz, char *name, TypeDesc *proto,
                     uint8_t *codes, int size, int locals);
void Image_Add_NFunc(Image *image, char *klazz, char *name, TypeDesc *proto);
void Image_Add_IMeth(Image *image, char *trait, char *name, TypeDesc *proto);
void Image_Add_EVal(Image *image, char *klazz, char *name,
                    Vector *types, int val);

Image *Image_New(char *name);
void Image_Free(Image *image);
void Image_Show(Image *image);
void Image_Finish(Image *image);
void Image_Write_File(Image *image, char *path);
/* flags is ITEM_XXX bits, marked not load */
Image *Image_Read_File(char *path, int unload);

int Image_Const_Count(Image *image);
typedef void (*getconstfunc)(void *, int, int, void *);
void Image_Get_Consts(Image *image, getconstfunc func, void *arg);
typedef void (*getvarfunc)(char *, TypeDesc *, int, Literal *val, void *);
void Image_Get_Vars(Image *image, getvarfunc func, void *arg);
typedef void (*getlocvarfunc)(char *, TypeDesc *, int, int, void *);
void Image_Get_LocVars(Image *image, getlocvarfunc func, void *arg);
typedef void (*getfuncfunc)(char *, TypeDesc *, int, int, uint8_t *, int, void *);
void Image_Get_Funcs(Image *image, getfuncfunc func, void *arg);
void Image_Get_NFuncs(Image *image, getfuncfunc func, void *arg);
typedef void (*getclassfunc)(char *, void *);
void Image_Get_Classes(Image *image, getclassfunc func, void *arg);
typedef void (*getfieldfunc)(char *, TypeDesc *, char *, void *);
void Image_Get_Fields(Image *image, getfieldfunc func, void *arg);
typedef void (*getmethodfunc)(char *, TypeDesc *, int,
                              uint8_t *, int, char *, void *);
void Image_Get_Methods(Image *image, getmethodfunc func, void *arg);
typedef void (*getenumfunc)(char *, void *);
void Image_Get_Enums(Image *image, getenumfunc func, void *arg);
typedef void (*getevalfunc)(char *, TypeDesc *, int, char *, void *);
void Image_Get_EVals(Image *image, getevalfunc func, void *arg);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_IMAGE_H_ */

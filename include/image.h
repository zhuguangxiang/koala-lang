/*
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _KOALA_IMAGE_H_
#define _KOALA_IMAGE_H_

#include "atomtable.h"
#include "typedesc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ITEM_MAP        0
#define ITEM_STRING     1
#define ITEM_TYPE       2
#define ITEM_TYPELIST   3
#define ITEM_PROTO      4
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
#define ITEM_MAX        16

typedef struct map_item {
  /* one of ITEM_XXX, self is ITEM_MAP */
  uint16 type;
  uint16 unused;
  uint32 offset;
  int32 size;
} MapItem;

typedef struct string_item {
  int32 length;
  char data[0];
} StringItem;

typedef struct type_item {
  int kind;             /* see: TypeDescKind in typedesc.h */
  union {
    char primitive;     /* see: typedesc.h */
    struct {
      int32 pathindex;  /* ->StringItem */
      int32 typeindex;  /* ->StringItem */
    };
    int32 protoindex;   /* ->ProtoItem */
    struct {
      int dims;
      int32 typeindex;  /* ->TypeItem */
    } array;
    struct {
      int32 keyindex;   /* ->TypeItem */
      int32 valueindex; /* ->TypeItem */
    } map;
  };
} TypeItem;

typedef struct typelist_item {
  int32 size;
  int32 index[0]; /* ->TypeItem */
} TypeListItem;

typedef struct proto_item {
  int32 rindex;   /* ->TypeListItem */
  int32 pindex;   /* ->TypeListItem */
} ProtoItem;

#define CONST_INT     1
#define CONST_FLOAT   2
#define CONST_BOOL    3
#define CONST_STRING  4

typedef struct const_item {
  int type;
  union {
    int64 ival;   /* int32 or int64 */
    float64 fval; /* float32 or float64 */
    int bval;     /* bool */
    int32 index;  /* ->StringItem */
  };
} ConstItem;

typedef struct local_var_item {
  int32 nameindex;  /* ->StringItem */
  int32 typeindex;  /* ->TypeItem */
  int32 pos;        /* ->index of locvars */
  int32 index;      /* ->Index of FuncItem or MethodItem */
} LocVarItem;

typedef struct var_item {
  int32 nameindex;  /* ->StringItem */
  int32 typeindex;  /* ->TypeItem */
  int32 konst;      /* ->constant */
} VarItem;

typedef struct func_item {
  int32 nameindex;  /* ->StringItem */
  int32 protoindex; /* ->ProtoItem */
  int32 codeindex;  /* ->CodeItem */
} FuncItem;

typedef struct code_item {
  int32 size;
  uint8 codes[0];
} CodeItem;

typedef struct class_item {
  int32 classindex;  /* ->TypeItem */
  int32 superindex;  /* ->TypeItem */
} ClassItem;

typedef struct feild_item {
  int32 classindex; /* ->TypeItem */
  int32 nameindex;  /* ->StringItem */
  int32 typeindex;  /* ->TypeItem */
} FieldItem;

typedef struct method_item {
  int32 classindex; /* ->TypeItem */
  int32 nameindex;  /* ->StringItem */
  int32 protoindex; /* ->ProtoItem */
  int32 codeindex;  /* ->CodeItem */
} MethodItem;

typedef struct trait_item {
  int32 classindex;  /* ->TypeItem */
  int32 traitsindex; /* ->TypeListItem */
} TraitItem;

typedef struct nfunc_item {
  int32 classindex; /* ->TypeItem */
  int32 nameindex;  /* ->StringItem */
  int32 protoindex; /* ->ProtoItem */
} NFuncItem;

typedef struct imeth_item {
  int32 classindex; /* ->TypeItem */
  int32 nameindex;  /* ->StringItem */
  int32 protoindex; /* ->ProtoItem */
} IMethItem;

/* koala byte code image header */
typedef struct image_header {
  uint8 magic[4];
  uint8 version[4];
  uint32 file_size;
  uint32 header_size;
  uint32 endian_tag;
  uint32 map_offset;
  uint32 map_size;
  uint32 crc32;
} ImageHeader;

typedef struct kimage {
  ImageHeader header;
  AtomTable *table;
} KImage;

KImage *KImage_New(void);
void KImage_Free(KImage *image);
void KImage_Show(KImage *image);

int KImage_Add_Integer(KImage *image, int64 val);
int KImage_Add_Float(KImage *image, float64 val);
int KImage_Add_Bool(KImage *image, int val);
int KImage_Add_String(KImage *image, char *val);

void KImage_Add_Var(KImage *image, char *name, TypeDesc *desc, int konst);
void KImage_Add_LocVar(KImage *image, char *name, TypeDesc *desc,
                       int pos, int index);
int KImage_Add_Func(KImage *image, char *name, TypeDesc *proto,
                    uint8 *codes, int size);
void KImage_Add_Class(KImage *image, char *name, Vector *supers);
void KImage_Add_Trait(KImage *image, char *name, Vector *traits);

void KImage_Add_Field(KImage *image, char *klazz, char *name, TypeDesc *desc);
int KImage_Add_Method(KImage *image, char *klazz, char *name, TypeDesc *proto,
                      uint8 *codes, int size);
void KImage_Add_NFunc(KImage *image, char *klazz, char *name, TypeDesc *proto);
void KImage_Add_IMeth(KImage *image, char *trait, char *name, TypeDesc *proto);

static inline int KImage_Count_Consts(KImage *image)
{
  return AtomTable_Size((image)->table, ITEM_CONST);
}
typedef void (*getconstfunc)(int, void *, int, void *);
void KImage_Get_Consts(KImage *image, getconstfunc func, void *arg);
typedef void (*getvarfunc)(char *, TypeDesc *, int, void *);
void KImage_Get_Vars(KImage *image, getvarfunc func, void *arg);
typedef void (*getlocvarfunc)(char *, TypeDesc *, int, int, void *);
void KImage_Get_LocVars(KImage *image, getlocvarfunc func, void *arg);
typedef void (*getfuncfunc)(char *, TypeDesc *, uint8 *, int, int, void *);
void KImage_Get_Funcs(KImage *image, getfuncfunc func, void *arg);

void KImage_Finish(KImage *image);
void KImage_Write_File(KImage *image, char *path);
KImage *KImage_Read_File(char *path);

// TypeDesc *TypeItem_To_TypeDesc(TypeItem *item, AtomTable *atbl);
// TypeDesc *ProtoItem_To_TypeDesc(ProtoItem *item, AtomTable *atbl);
// int StringItem_Get(AtomTable *table, char *str);
// int StringItem_Set(AtomTable *table, char *str);
// int TypeItem_Get(AtomTable *table, TypeDesc *desc);
// int TypeItem_Set(AtomTable *table, TypeDesc *desc);
// int TypeListItem_Get(AtomTable *table, Vector *desc);
// int TypeListItem_Set(AtomTable *table, Vector *desc);
// int ProtoItem_Get(AtomTable *table, int32 rindex, int32 pindex);
// int ProtoItem_Set(AtomTable *table, TypeDesc *proto);
// int ConstItem_Get(AtomTable *table, ConstItem *item);
// int ConstItem_Set_Int(AtomTable *table, int64 val);
// int ConstItem_Set_Float(AtomTable *table, float64 val);
// int ConstItem_Set_Bool(AtomTable *table, int val);
// int ConstItem_Set_String(AtomTable *table, char *str);
// int Symbol_Access(char *name, int bconst);
// uint32 Item_Hash(void *key);
// int Item_Equal(void *k1, void *k2);
// void Item_Free(int type, void *data, void *arg);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_IMAGE_H_ */

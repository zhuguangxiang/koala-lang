
#ifndef _KOALA_CODEFORMAT_H_
#define _KOALA_CODEFORMAT_H_

#include "common.h"
#include "itemtable.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct image_header {
  uint8 magic[4];
  uint8 version[4];
  uint32 file_size;
  uint32 header_size;
  uint32 endian_tag;
  uint32 map_offset;
  uint32 map_size;
  uint32 pkg_size;
} ImageHeader;

#define ITEM_MAP        0
#define ITEM_STRING     1
#define ITEM_TYPE       2
#define ITEM_TYPELIST   3
#define ITEM_PROTO      4
#define ITEM_CONST      5
#define ITEM_VAR        6
#define ITEM_FUNC       7
#define ITEM_CODE       8
#define ITEM_CLASS      9
#define ITEM_FIELD      10
#define ITEM_METHOD     11
#define ITEM_INTF       12
#define ITEM_IMETH      13
#define ITEM_MAX        14

typedef struct map_item {
  uint16 type;
  uint16 unused;
  uint32 offset;
  uint32 size;
} MapItem;

typedef struct string_item {
  uint32 length;
  char data[0];
} StringItem;

typedef struct type_item {
  uint32 desc_index;  //->StringItem
} TypeItem;

typedef struct typelist_item {
  uint32 size;
  uint32 desc_index[0];   //->StringItem
} TypeListItem;

typedef struct proto_item {
  uint32 return_index;    //->TypeListItem
  uint32 parameter_index; //->TypeListItem
} ProtoItem;

#define CONST_INT     1
#define CONST_FLOAT   2
#define CONST_BOOL    3
#define CONST_STRING  4

typedef struct const_item {
  int type;
  union {
    int64 ival;          // int32 or int64
    float64 fval;         // float32 or float64
    int bval;             // bool
    uint32 string_index;  //->StringItem
  };
} ConstItem;

#define CONST_IVAL_INIT(_v)   {.type = CONST_INT,   .ival = (int64)(_v)}
#define CONST_FVAL_INIT(_v)   {.type = CONST_FLOAT, .fval = (float64)(_v)}
#define CONST_BVAL_INIT(_v)   {.type = CONST_BOOL,  .bval = (int)(_v)}
#define CONST_STRVAL_INIT(_v) \
  {.type = CONST_STRING, .string_index = (uint32)(_v)}

#define const_setstrvalue(v, _v) do { \
  (v)->type = CONST_STRING; (v)->string_index = (uint32)(_v); \
} while (0)

typedef struct var_item {
  uint32 name_index;  //->StringItem
  uint32 type_index;  //->TypeItem
  uint32 flags;       //access and constant
} VarItem;

typedef struct func_item {
  uint32 name_index;    //->StringItem
  uint32 proto_index;   //->ProtoItem
  uint16 flags;         //access
  uint16 nr_returns;    //number of returns
  uint16 nr_paras;      //number of parameters
  uint16 nr_locals;     //number of lcoal variabls
  uint32 code_index;    //->CodeItem
} FuncItem;

typedef struct code_item {
  uint32 size;
  uint8 insts[0];
} CodeItem;

typedef struct struct_item {
  uint32 name_index;    //->StringItem
  uint32 pname_index;   //->StringItem
  int flags;
  uint32 fields_off;    //->FieldListItem
  uint32 methods_off;   //->MethodListItem
} StructItem;

// typedef VarItem FieldItem;
// typedef FuncItem MethodItem;
// typedef struct field_list_item {
//   uint32 size;
//   uint32 field_index[0];
// } FieldListItem;

// typedef struct method_list_item {
//   uint32 size;
//   uint32 method_index[0];
// } MethodListItem;

// typedef struct intf_item {
//   uint32 name_index;        //->StringItem
//   uint32 size;
//   uint32 imethod_index[0];  //->IMethodItem
// } IntfItem;

// typedef struct imethod_item {
//   uint32 name_index;    //->StringItem
//   uint32 proto_index;   //->ProtoItem
// } IMethodItem;

typedef struct klcimage {
  ImageHeader header;
  char *package;
  ItemTable *table;
} KLCImage;

KLCImage *KLCImage_New(char *pkg_name);
void KLCImage_Free(KLCImage *image);
void KLCImage_Finish(KLCImage *image);
void KLCImage_Add_Var(KLCImage *image, char *name, int flags, char *desc);
void KLCImage_Add_Func(KLCImage *image, char *name, int flags, int nr_locals,
                       char *rdesclist, char *pdesclist, uint8 *code, int csz);
void KLCImage_Write_File(KLCImage *image, char *path);
KLCImage *KLCImage_Read_File(char *path);
void KLCImage_Display(KLCImage *image);

static inline int Count_Vars(KLCImage *image)
{
  return ItemTable_Size(image->table, ITEM_VAR);
}

static inline int Count_Konsts(KLCImage *image)
{
  return ItemTable_Size(image->table, ITEM_CONST);
}

static inline int Count_Funcs(KLCImage *image)
{
  return ItemTable_Size(image->table, ITEM_FUNC);
}

int StringItem_Get(ItemTable *itemtable, char *str, int len);
int StringItem_Set(ItemTable *itemtable, char *str, int len);
int TypeItem_Get(ItemTable *itemtable, char *str, int len);
int TypeItem_Set(ItemTable *itemtable, char *str, int len);
int TypeListItem_Get(ItemTable *itemtable, char *desclist, int *size);
int TypeListItem_Set(ItemTable *itemtable, char *desclist, int *size);
int ProtoItem_Get(ItemTable *itemtable, int rindex, int pindex);
int ProtoItem_Set(ItemTable *itemtable, char *rdes, char *pdesc,
                  int *rsz, int *psz);
uint32 item_hash(void *key);
int item_equal(void *k1, void *k2);

typedef struct desc_index {
  int offset;
  int length;
} DescIndex;

typedef struct desc_list {
  char *desclist;
  int size;
  DescIndex index[0];
} DescList;

int Desc_Count(char *descstr);
DescList *DescList_Parse(char *desclist);
void DescList_Free(DescList *dlist);
char *DescList_Desc(DescList *dlist, int index);
int DescList_Length(DescList *dlist, int index);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_CODEFORMAT_H_ */

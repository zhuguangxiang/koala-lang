
#ifndef _KOALA_CODEFORMAT_H_
#define _KOALA_CODEFORMAT_H_

#include "common.h"
#include "itemtable.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TYPE_PRIMITIVE  1
#define TYPE_DEFINED    2

#define PRIMITIVE_INT     'i'
#define PRIMITIVE_FLOAT   'f'
#define PRIMITIVE_BOOL    'b'
#define PRIMITIVE_STRING  's'
#define PRIMITIVE_ANY     'A'

/* Type's descriptor */
typedef struct typedesc {
  short dims;
  short kind;
  union {
    int primitive;
    char *str;
  };
} TypeDesc;

#define Decl_Primitive_Desc(desc, d, p) \
  TypeDesc desc = {.dims = (d), .kind = TYPE_PRIMITIVE, .primitive = (p)}
#define Decl_Defined_Desc(desc, d, s)  \
  TypeDesc desc = {.dims = (d), .kind = TYPE_DEFINED, .str = (s)}
#define Init_Primitive_Desc(desc, d, p) do { \
  (desc)->dims = (d); (desc)->kind = TYPE_PRIMITIVE; \
  (desc)->primitive = (p); \
} while (0)
#define Init_Defined_Desc(desc, d, s) do { \
  (desc)->dims = (d); (desc)->kind = TYPE_DEFINED; \
  (desc)->str = (s); \
} while (0)

typedef struct protoinfo {
  int rsz;
  int psz;
  TypeDesc *rdesc;
  TypeDesc *pdesc;
} ProtoInfo;

typedef struct funcinfo {
  ProtoInfo proto;
  int locals;
  int csz;
  uint8 *codes;
} FuncInfo;

int CStr_To_Desc(char *str, TypeDesc *desc);
TypeDesc *CStr_To_DescList(int count, char *str);
void Init_ProtoInfo(int rsz, char *rdesc, int psz, char *pdesc,
                    ProtoInfo *proto);
void Init_FuncInfo(int rsz, char *rdesc, int psz, char *pdesc,
                   int locals, uint8 *codes, int csz,
                   FuncInfo *funcinfo);
/*-------------------------------------------------------------------------*/

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
  short dims;
  short kind;
  union {
    int primitive;
    uint32 index;   //->StringItem
  };
} TypeItem;

typedef struct typelist_item {
  uint32 size;
  uint32 index[0];  //->TypeItem
} TypeListItem;

typedef struct proto_item {
  uint32 rindex;  //->TypeListItem
  uint32 pindex;  //->TypeListItem
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
  uint16 rets;          //number of returns
  uint16 args;          //number of parameters
  uint16 locals;        //number of lcoal variabls
  uint32 code_index;    //->CodeItem
} FuncItem;

typedef struct code_item {
  uint32 size;
  uint8 codes[0];
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
  ItemTable *itable;
} KLCImage;

KLCImage *KLCImage_New(char *pkg_name);
void KLCImage_Free(KLCImage *image);
void KLCImage_Finish(KLCImage *image);
void KLCImage_Add_Var(KLCImage *image, char *name, int flags, TypeDesc *desc);
void KLCImage_Add_Func(KLCImage *image, char *name, int flags, FuncInfo *info);
void KLCImage_Write_File(KLCImage *image, char *path);
KLCImage *KLCImage_Read_File(char *path);
void KLCImage_Display(KLCImage *image);

#define KLCImage_Count_Vars(image) \
  ItemTable_Size((image)->itable, ITEM_VAR)

#define KLCImage_Count_Consts(image) \
  ItemTable_Size((image)->itable, ITEM_CONST)

#define KLCImage_Count_Functions(image) \
  ItemTable_Size((image)->itable, ITEM_FUNC)

int StringItem_Get(ItemTable *itable, char *str);
int StringItem_Set(ItemTable *itable, char *str);
int TypeItem_Get(ItemTable *itable, TypeDesc *desc);
int TypeItem_Set(ItemTable *itable, TypeDesc *desc);
int TypeListItem_Get(ItemTable *itable, TypeDesc *desc, int sz);
int TypeListItem_Set(ItemTable *itable, TypeDesc *desc, int sz);
int ProtoItem_Get(ItemTable *itable, int rindex, int pindex);
int ProtoItem_Set(ItemTable *itable, ProtoInfo *proto);
uint32 item_hash(void *key);
int item_equal(void *k1, void *k2);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_CODEFORMAT_H_ */

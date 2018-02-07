
#ifndef _KOALA_CODEFORMAT_H_
#define _KOALA_CODEFORMAT_H_

#include "atom.h"

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
  short dims;
  short kind;
  union {
    char primitive;
    int32 index;   //->StringItem
  };
} TypeItem;

typedef struct typelist_item {
  int32 size;
  int32 index[0];  //->TypeItem
} TypeListItem;

typedef struct proto_item {
  int32 rindex;  //->TypeListItem
  int32 pindex;  //->TypeListItem
} ProtoItem;

#define CONST_INT     1
#define CONST_FLOAT   2
#define CONST_BOOL    3
#define CONST_STRING  4

typedef struct const_item {
  int type;
  union {
    int64 ival;   // int32 or int64
    float64 fval; // float32 or float64
    int bval;     // bool
    int32 index;  //->StringItem
  };
} ConstItem;

#define CONST_IVAL_INIT(_v)   {.type = CONST_INT,   .ival = (int64)(_v)}
#define CONST_FVAL_INIT(_v)   {.type = CONST_FLOAT, .fval = (float64)(_v)}
#define CONST_BVAL_INIT(_v)   {.type = CONST_BOOL,  .bval = (int)(_v)}
#define CONST_STRVAL_INIT(_v) \
  {.type = CONST_STRING, .index = (int32)(_v)}

#define const_setstrvalue(v, _v) do { \
  (v)->type = CONST_STRING; (v)->index = (int32)(_v); \
} while (0)

ConstItem *ConstItem_Int_New(int64 val);
ConstItem *ConstItem_Float_New(float64 val);
ConstItem *ConstItem_Bool_New(int val);
ConstItem *ConstItem_String_New(int32 val);

typedef struct var_item {
  int32 name_index;  //->StringItem
  int32 type_index;  //->TypeItem
  int32 flags;       //access
#define VAR_FLAG_PUBLIC   0
#define VAR_FLAG_PRIVATE  1
#define VAR_FLAG_CONST    2
} VarItem;

typedef struct func_item {
  int32 name_index;   //->StringItem
  int32 proto_index;  //->ProtoItem
  int8 access;        //access
  int8 vargs;         //variable args
  int16 rets;         //number of returns
  int16 args;         //number of parameters
  int16 locals;       //number of lcoal variabls
  int32 code_index;   //->CodeItem
} FuncItem;

typedef struct code_item {
  uint32 size; //includes sizeof(CodeItem)
  uint32 ksz;
  uint32 koffset;
  uint32 csz;
  uint8 codes[0];
} CodeItem;

typedef struct struct_item {
  int32 name_index;    //->StringItem
  int32 pname_index;   //->StringItem
  int flags;
  int32 fields_off;    //->FieldListItem
  int32 methods_off;   //->MethodListItem
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

typedef struct kimage {
  ImageHeader header;
  char *package;
  AtomTable *table;
} KImage;

/*-------------------------------------------------------------------------*/

#define TYPE_PRIMITIVE  1
#define TYPE_STRUCTED   2
#define TYPE_PROTO      3

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
    char primitive;
    char *str;
  };
} TypeDesc;

#define DECL_PRIMITIVE_DESC(desc, d, p) \
  TypeDesc desc = {.dims = (d), .kind = TYPE_PRIMITIVE, .primitive = (p)}
#define DECL_STRUCTED_DESC(desc, d, s) \
  TypeDesc desc = {.dims = (d), .kind = TYPE_STRUCTED, .str = (s)}
#define INIT_PRIMITIVE_DESC(desc, d, p) do { \
  (desc)->dims = (d); (desc)->kind = TYPE_PRIMITIVE; \
  (desc)->primitive = (p); \
} while (0)
#define INIT_STRUCTED_DESC(desc, d, s) do { \
  (desc)->dims = (d); (desc)->kind = TYPE_STRUCTED; \
  (desc)->str = (s); \
} while (0)
char *primitive_tostring(int type);

typedef struct protoinfo {
  int rsz;
  int psz;
  int vargs;
  TypeDesc *rdesc;
  TypeDesc *pdesc;
} ProtoInfo;

typedef struct codeinfo {
  int csz;
  uint8 *codes;
  int ksz;
  ConstItem *k;
} CodeInfo;

typedef struct funcinfo {
  ProtoInfo *proto;
  CodeInfo *code;
  int locals;
} FuncInfo;

typedef struct functype {
  int rsz;
  char *rdesc;
  int psz;
  char *pdesc;
} FuncType;

#define DECL_FUNCTYPE_INIT(name, rsz, rdesc, psz, pdesc) \
  FuncType name = {rsz, rdesc, psz, pdesc}
#define INIT_FUNCTYPE(name, _rsz, _rdesc, _psz, _pdesc) do {\
  (name)->rsz = (_rsz); (name)->rdesc = (_rdesc); \
  (name)->psz = (_psz); (name)->pdesc = (_pdesc); \
} while (0)
int String_To_Desc(char *str, TypeDesc *desc);
TypeDesc *String_To_DescList(int count, char *str);
void Init_ProtoInfo(FuncType *type, ProtoInfo *proto);
void Init_Vargs_ProtoInfo(int rsz, char *rdesc, ProtoInfo *proto);
void Init_CodeInfo(uint8 *codes, int csz, ConstItem *k, int ksz,
                   CodeInfo *codeinfo);
void Init_FuncInfo(ProtoInfo *proto, CodeInfo *code, int locals,
                   FuncInfo *funcinfo);

/*-------------------------------------------------------------------------*/

KImage *KImage_New(char *pkg_name);
void KImage_Free(KImage *image);
void KImage_Finish(KImage *image);
void __KImage_Add_Var(KImage *image, char *name, TypeDesc *desc, int bconst);
#define KImage_Add_Var(image, name, desc) \
  __KImage_Add_Var(image, name, desc, 0)
#define KImage_Add_Const(image, name, desc) \
  __KImage_Add_Var(image, name, desc, 1)
void KImage_Add_Func(KImage *image, char *name, FuncInfo *info);
void KImage_Write_File(KImage *image, char *path);
KImage *KImage_Read_File(char *path);
void KImage_Show(KImage *image);

#define KImage_Count_Vars(image) \
  ItemTable_Size((image)->table, ITEM_VAR)

#define KImage_Count_Consts(image) \
  ItemTable_Size((image)->table, ITEM_CONST)

#define KImage_Count_Functions(image) \
  ItemTable_Size((image)->table, ITEM_FUNC)

int StringItem_Get(AtomTable *table, char *str);
int StringItem_Set(AtomTable *table, char *str);
int TypeItem_Get(AtomTable *table, TypeDesc *desc);
int TypeItem_Set(AtomTable *table, TypeDesc *desc);
int TypeListItem_Get(AtomTable *table, TypeDesc *desc, int sz);
int TypeListItem_Set(AtomTable *table, TypeDesc *desc, int sz);
int ProtoItem_Get(AtomTable *table, int32 rindex, int32 pindex);
int ProtoItem_Set(AtomTable *table, ProtoInfo *proto);
int ConstItem_Get(AtomTable *table, ConstItem *item);
int ConstItem_Set_Int(AtomTable *table, int64 val);
int ConstItem_Set_Float(AtomTable *table, float64 val);
int ConstItem_Set_Bool(AtomTable *table, int val);
int ConstItem_Set_String(AtomTable *table, int32 val);
uint32 item_hash(void *key);
int item_equal(void *k1, void *k2);
void AtomTable_Show(AtomTable *table);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_CODEFORMAT_H_ */

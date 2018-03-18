
#ifndef _KOALA_CODE_FORMAT_H_
#define _KOALA_CODE_FORMAT_H_

#include "atomtable.h"
#include "koala_type.h"

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
#define ITEM_LOCVAR     6
#define ITEM_VAR        7
#define ITEM_FUNC       8
#define ITEM_CODE       9
#define ITEM_CLASS      10
#define ITEM_FIELD      11
#define ITEM_METHOD     12
#define ITEM_INTF       13
#define ITEM_IMETH      14
#define ITEM_MAX        15

typedef struct map_item {
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
	int8 kind;
	int8 varg;
	int16 dims;
	union {
		char primitive;
		struct {
			int32 pathindex;  //->StringItem
			int32 typeindex;  //->StringItem
		};
		int32 protoindex;   //->ProtoItem
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

#define FUNCLOCVAR  1
#define METHLOCVAR  2
typedef struct local_var_item {
	int32 nameindex;  //->StringItem
	int32 typeindex;  //->TypeItem
	int32 pos;        //index of locvars
	int16 flags;      //in function or method
	int16 index;      //->Index of FuncItem or MethodItem
} LocVarItem;

typedef struct var_item {
	int32 nameindex;  //->StringItem
	int32 typeindex;  //->TypeItem
	int32 access;     //symbol access, FIXME
} VarItem;

typedef struct func_item {
	int32 nameindex;  //->StringItem
	int32 protoindex; //->ProtoItem
	int16 access;     //symbol access, FIXME
	int16 locvars;    //number of lcoal variables
	int32 codeindex;  //->CodeItem
} FuncItem;

typedef struct code_item {
	int32 size;
	uint8 codes[0];
} CodeItem;

typedef struct class_item {
	int32 classindex; //->TypeItem
	int32 access;     //symbol access, FIXME
	int32 superindex; //->TypeItem
} ClassItem;

typedef struct feild_item {
	int32 classindex; //->TypeItem
	int32 nameindex;  //->StringItem
	int32 typeindex;  //->TypeItem
	int32 access;     //symbol access, FIXME
} FieldItem;

typedef struct method_item {
	int32 classindex; //->TypeItem
	int32 nameindex;  //->StringItem
	int32 protoindex; //->ProtoItem
	int16 access;     //symbol access, FIXME
	int16 locvars;    //number of lcoal variables
	int32 codeindex;  //->CodeItem
} MethodItem;

typedef struct intf_item {
	int32 classindex; //->TypeItem
	int32 access;     //symbol access, FIXME
} IntfItem;

typedef struct imeth_item {
	int32 classindex; //->TypeItem
	int32 nameindex;  //->StringItem
	int32 protoindex; //->ProtoItem
	int32 access;     //symbol access, FIXME
} IMethItem;

typedef struct kimage {
	ImageHeader header;
	char *package;
	int bused;  /* for free this structure */
	AtomTable *table;
} KImage;

/*-------------------------------------------------------------------------*/

int TypeItem_To_Desc(AtomTable *atbl, TypeItem *item, TypeDesc *desc);
Proto *Proto_From_ProtoItem(ProtoItem *item, AtomTable *atbl);


KImage *KImage_New(char *pkg_name);
void KImage_Free(KImage *image);
void KImage_Finish(KImage *image);
void KImage_Add_LocVar(KImage *image, char *name, TypeDesc *desc, int pos,
	int flags, int index);
void __KImage_Add_Var(KImage *image, char *name, TypeDesc *desc, int bconst);
#define KImage_Add_Var(image, name, desc) \
	__KImage_Add_Var(image, name, desc, 0)
#define KImage_Add_Const(image, name, desc) \
	__KImage_Add_Var(image, name, desc, 1)
int KImage_Add_Func(KImage *image, char *name, Proto *proto, int locvars,
	uint8 *codes, int csz);
void KImage_Add_Class(KImage *image, char *name, char *spath, char *stype);
void KImage_Add_Field(KImage *image, char *clazz, char *name, TypeDesc *desc);
int KImage_Add_Method(KImage *image, char *clazz, char *name, Proto *proto,
	int locvars, uint8 *codes, int csz);
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
#define StringItem_Index(table, index) \
	(StringItem *)AtomTable_Get(table, ITEM_STRING, index)

int TypeItem_Get(AtomTable *table, TypeDesc *desc);
int TypeItem_Set(AtomTable *table, TypeDesc *desc);
#define TypeItem_Index(table, index) \
	(TypeItem *)AtomTable_Get(table, ITEM_TYPE, index)

int TypeListItem_Get(AtomTable *table, TypeDesc *desc, int sz);
int TypeListItem_Set(AtomTable *table, TypeDesc *desc, int sz);
#define TypeListItem_Index(table, index) \
	AtomTable_Get(table, ITEM_TYPELIST, index)

int ProtoItem_Get(AtomTable *table, int32 rindex, int32 pindex);
int ProtoItem_Set(AtomTable *table, Proto *proto);
#define ProtoItem_Index(table, index) \
	AtomTable_Get(table, ITEM_PROTO, index)

int ConstItem_Get(AtomTable *table, ConstItem *item);
int ConstItem_Set_Int(AtomTable *table, int64 val);
int ConstItem_Set_Float(AtomTable *table, float64 val);
int ConstItem_Set_Bool(AtomTable *table, int val);
int ConstItem_Set_String(AtomTable *table, char *str);
#define ConstItem_Index(table, index) \
	AtomTable_Get(table, ITEM_CONST, index)

#define CodeItem_Index(table, index) \
	AtomTable_Get(table, ITEM_CODE, index)

uint32 item_hash(void *key);
int item_equal(void *k1, void *k2);
void item_free(int type, void *data, void *arg);
void AtomTable_Show(AtomTable *table);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_CODE_FORMAT_H_ */

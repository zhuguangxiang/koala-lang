
#ifndef _KOALA_CODEFORMAT_H_
#define _KOALA_CODEFORMAT_H_

#include "types.h"
#include "vector.h"
#include "hash_table.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct klcheader {
  uint8 magic[4];
  uint8 version[4];
  uint32 file_size;
  uint32 header_size;
  uint32 endian_tag;
  uint32 map_offset;
  uint32 map_size;
  uint32 pkg_size;
} KLCHeader;

#define ITYPE_MAP       0
#define ITYPE_STRING    1
#define ITYPE_TYPE      2
#define ITYPE_TYPELIST  3
#define ITYPE_STRUCT    4
#define ITYPE_INTF      5
#define ITYPE_VAR       6
#define ITYPE_FIELD     7
#define ITYPE_PROTO     8
#define ITYPE_FUNC      9
#define ITYPE_METHOD    10
#define ITYPE_CODE      11
#define ITYPE_CONST     12
#define ITYPE_MAX       13

typedef struct map_item {
  uint16 type;
  uint16 unused;
  uint32 offset;
  uint32 size;
} MapItem;

typedef struct string_item {
  uint16 length;
  char data[0];
} StringItem;

typedef struct type_item {
  uint32 desc_index;  //->StringItem
} TypeItem;

typedef struct typelist_item {
  uint32 size;
  uint32 type_index[0];   //->TypeItem
} TypeListItem;

#define FLAGS_ACCESS_PRIVATE 1
#define FLAGS_ACCESS_CONST   2

typedef struct var_item {
  uint32 name_index;  //->StringItem
  uint16 type_index;  //->TypeItem
  uint16 flags;       //access and constant
} VarItem;

typedef struct proto_item {
  uint32 return_off;    //->TypeListItem
  uint32 parameter_off; //->TypeListItem
} ProtoItem;

typedef struct func_item {
  uint32 name_index;    //->StringItem
  uint16 proto_index;   //->ProtoItem
  uint16 flags;         //access
  int nr_returns;       //number of returns
  int nr_paras;         //number of parameters
  int nr_locals;        //number of lcoal variabls
  uint32 code_off;      //->CodeItem
} FuncItem;

typedef struct code_item {
  int size;
  uint8 insts[0];
} CodeItem;

#define KTYPE_INT     1
#define KTYPE_FLOAT   2
#define KTYPE_BOOL    3
#define KTYPE_STRING  4

typedef struct konst_item {
  int type;
  union {
    int bval;
    // int32 or int64
    int64 ival;
    // float32 or float64
    float64 fval;
    uint32 string_index;  //->StringItem
  } value;
} KonstItem;

typedef struct {
  struct hash_node hnode;
  int itype;
  int index;
  void *data;
} ItemEntry;

#define ITEM_ENTRY_INIT(type, idx, d) \
  {.itype = (type), .index = (idx), .data = (d)}

typedef struct klcfile {
  FILE *fp;
  char *pkg_name;
  KLCHeader header;
  struct hash_table table;
  uint16 sizes[ITYPE_MAX];
  Vector items[ITYPE_MAX];
} KLCFile;

KLCFile *KLCFile_New(char *pkg_name);
void KLCFile_Free(KLCFile *filp);
void KLCFile_Finish(KLCFile *filp);
void KLCFile_Add_Var(KLCFile *filp, char *name, int flags, char *desc);
void KLCFile_Add_Func(KLCFile *filp, char *name, int flags, int nr_locals,
                      uint8 *code, int csz,
                      char *desc[], int rsz,
                      char *pdesc[], int psz);
void KLCFile_Write_File(KLCFile *filp, char *path);
KLCFile *KLCFile_Read_File(char *file);
void KCLFile_Display(KLCFile *filp);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_CODEFORMAT_H_ */


#include "codeformat.h"
#include "symbol.h"
#include "hash.h"
#include "debug.h"

static int version_major = 0; // 1 byte
static int version_minor = 1; // 1 byte
static int version_build = 1; // 2 bytes

#define ENDIAN_TAG  0x1a2b3c4d

/*-------------------------------------------------------------------------*/

static int desc_primitive(char ch)
{
  static char chs[] = {
    PRIMITIVE_INT,
    PRIMITIVE_FLOAT,
    PRIMITIVE_BOOL,
    PRIMITIVE_STRING,
    PRIMITIVE_ANY
  };

  for (int i = 0; i < nr_elts(chs); i++) {
    if (ch == chs[i]) return 1;
  }

  return 0;
}

int CStr_To_Desc(char *str, TypeDesc *desc)
{
  if (str == NULL) return -1;
  char ch = str[0];
  if (ch == '\0') return -1;

  if (str[1] == '\0') {
    ASSERT(desc_primitive(ch));
    desc->dims = 0;
    desc->kind = TYPE_PRIMITIVE;
    desc->primitive = ch;
  } else {
    int dims;
    while ((ch = *str) == '[') {
      dims++; str++;
    }
    desc->dims = dims;
    desc->kind = TYPE_DEFINED;
    desc->str = strdup(str);
  }

  return 0;
}

TypeDesc *CStr_To_DescList(int count, char *str)
{
  if (count == 0) return NULL;
  int sz = sizeof(TypeDesc) * count;
  TypeDesc *desc = malloc(sz);

  char ch;
  int idx = 0;
  int dims = 0;

  while ((ch = *str) != '\0') {
    if (desc_primitive(ch)) {
      ASSERT(idx < count);

      desc[idx].dims = dims;
      desc[idx].kind = TYPE_PRIMITIVE;
      desc[idx].primitive = ch;

      dims = 0;
      idx++;
      str++;
    } else if (ch == 'O') {
      ASSERT(idx < count);

      desc[idx].dims = dims;
      desc[idx].kind = TYPE_DEFINED;

      int cnt = 0;
      while ((ch = *str) != ';') {
        cnt++; str++;
      }
      desc[idx].str = strndup(str, cnt);

      dims = 0;
      idx++;
    } else if (ch == '[') {
      ASSERT(idx < count);

      while ((ch = *str) == '[') {
        dims++; str++;
      }
    } else {
      ASSERT_MSG("unknown type:%c\n", ch);
    }
  }

  return desc;
}

void Init_ProtoInfo(int rsz, char *rdesc, int psz, char *pdesc,
                    ProtoInfo *proto)
{
  proto->rsz = rsz;
  proto->rdesc = CStr_To_DescList(rsz, rdesc);
  proto->psz = psz;
  proto->pdesc = CStr_To_DescList(psz, pdesc);
}

void Init_Vargs_ProtoInfo(int rsz, char *rdesc, ProtoInfo *proto)
{
  Init_ProtoInfo(rsz, rdesc, 0, NULL, proto);
  proto->vargs = 1;
}

void Init_FuncInfo(ProtoInfo *proto, CodeInfo *code, int locals,
                   FuncInfo *funcinfo)
{
  funcinfo->proto  = proto;
  funcinfo->code   = code;
  funcinfo->locals = locals;
}

/*-------------------------------------------------------------------------*/

MapItem *MapItem_New(int type, int offset, int size)
{
  MapItem *item = malloc(sizeof(*item));
  item->type   = type;
  item->unused = 0;
  item->offset = offset;
  item->size   = size;
  return item;
}

StringItem *StringItem_New(char *name)
{
  int len = strlen(name);
  StringItem *item = malloc(sizeof(StringItem) + len + 1);
  item->length = len + 1;
  memcpy(item->data, name, len);
  item->data[len] = 0;
  return item;
}

TypeItem *TypeItem_Primitive_New(int dims, int primitive)
{
  TypeItem *item = malloc(sizeof(*item));
  item->dims = dims;
  item->kind = TYPE_PRIMITIVE;
  item->primitive = primitive;
  return item;
}

TypeItem *TypeItem_Structed_New(int dims, int32 index)
{
  TypeItem *item = malloc(sizeof(*item));
  item->dims = dims;
  item->kind = TYPE_DEFINED;
  item->index = index;
  return item;
}

TypeListItem *TypeListItem_New(int size, int32 index[])
{
  TypeListItem *item = malloc(sizeof(*item) + size * sizeof(uint32));
  item->size = size;
  for (int i = 0; i < size; i++) {
    item->index[i] = index[i];
  }
  return item;
}

VarItem *VarItem_New(int32 name_index, int32 type_index, int flags)
{
  VarItem *item = malloc(sizeof(*item));
  item->name_index = name_index;
  item->type_index = type_index;
  item->flags = flags;
  return item;
}

ProtoItem *ProtoItem_New(int32 rindex, int32 pindex)
{
  ProtoItem *item = malloc(sizeof(*item));
  item->rindex = rindex;
  item->pindex = pindex;
  return item;
}

FuncItem *FuncItem_New(int name_index, int proto_index, int access, int vargs,
                       int rsz, int psz, int locals, int code_index)
{
  FuncItem *item = malloc(sizeof(*item));
  item->name_index = name_index;
  item->proto_index = proto_index;
  item->access = access;
  item->vargs = vargs;
  item->rets = rsz;
  item->args = psz;
  item->locals = locals;
  item->code_index = code_index;
  return item;
}

CodeItem *CodeItem_New(CodeInfo *codeinfo)
{
  int size = sizeof(CodeItem) + sizeof(uint8) * codeinfo->csz;
  size = ALIGN_UP(size, sizeof(ConstItem));
  int koffset = size;
  size += sizeof(ConstItem) * codeinfo->ksz;
  CodeItem *item = malloc(size);
  item->size = size;
  item->csz = codeinfo->csz;
  memcpy(item->codes, codeinfo->codes, codeinfo->csz);
  item->ksz = codeinfo->ksz;
  memcpy((char *)item + koffset, codeinfo->k,
         sizeof(ConstItem) * codeinfo->ksz);
  return item;
}

/*-------------------------------------------------------------------------*/

int StringItem_Get(ItemTable *itable, char *str)
{
  int len = strlen(str);
  uint8 data[sizeof(StringItem) + len + 1];
  StringItem *item = (StringItem *)data;
  item->length = len + 1;
  memcpy(item->data, str, len);
  item->data[len] = 0;
  return ItemTable_Index(itable, ITEM_STRING, item);
}

int StringItem_Set(ItemTable *itable, char *str)
{
  int index = StringItem_Get(itable, str);

  if (index < 0) {
    StringItem *item = StringItem_New(str);
    index = ItemTable_Append(itable, ITEM_STRING, item, 1);
  }

  return index;
}

int TypeItem_Get(ItemTable *itable, TypeDesc *desc)
{
  TypeItem item;
  if (desc->kind == TYPE_DEFINED) {
    int index = StringItem_Get(itable, desc->str);
    if (index < 0) return index;
    item.dims = desc->dims;
    item.kind = TYPE_DEFINED;
    item.index = index;
  } else {
    ASSERT(desc->kind == TYPE_PRIMITIVE);
    item.dims = desc->dims;
    item.kind = TYPE_PRIMITIVE;
    item.primitive = desc->primitive;
  }
  return ItemTable_Index(itable, ITEM_TYPE, &item);
}

int TypeItem_Set(ItemTable *itable, TypeDesc *desc)
{
  TypeItem *item;
  int index = TypeItem_Get(itable, desc);
  if (index < 0) {
    if (desc->kind == TYPE_DEFINED) {
      int index = StringItem_Set(itable, desc->str);
      ASSERT(index >= 0);
      item = TypeItem_Structed_New(desc->dims, index);
    } else {
      ASSERT(desc->kind == TYPE_PRIMITIVE);
      item = TypeItem_Primitive_New(desc->dims, desc->primitive);
    }
    index = ItemTable_Append(itable, ITEM_TYPE, item, 1);
  }
  return index;
}

int TypeListItem_Get(ItemTable *itable, TypeDesc *desc, int sz)
{
  if (sz <= 0) return -1;
  int32 indexes[sz];
  int index;
  for (int i = 0; i < sz; i++) {
    index = TypeItem_Get(itable, desc + i);
    if (index < 0) return -1;
    indexes[i] = index;
  }
  uint8 data[sizeof(TypeListItem) + sizeof(uint32) * sz];
  TypeListItem *item = (TypeListItem *)data;
  item->size = sz;
  for (int i = 0; i < sz; i++) {
    item->index[i] = indexes[i];
  }

  return ItemTable_Index(itable, ITEM_TYPELIST, item);
}

int TypeListItem_Set(ItemTable *itable, TypeDesc *desc, int sz)
{
  if (sz <= 0) return -1;
  int index = TypeListItem_Get(itable, desc, sz);
  if (index < 0) {
    int32 indexes[sz];
    for (int i = 0; i < sz; i++) {
      index = TypeItem_Set(itable, desc + i);
      if (index < 0) {ASSERT(0); return -1;}
      indexes[i] = index;
    }
    TypeListItem *item = TypeListItem_New(sz, indexes);
    index = ItemTable_Append(itable, ITEM_TYPELIST, item, 1);
  }
  return index;
}

int ProtoItem_Get(ItemTable *itable, int32 rindex, int32 pindex)
{
  ProtoItem item = {rindex, pindex};
  return ItemTable_Index(itable, ITEM_PROTO, &item);
}

int ProtoItem_Set(ItemTable *itable, ProtoInfo *proto)
{
  int rindex = TypeListItem_Set(itable, proto->rdesc, proto->rsz);
  int pindex = TypeListItem_Set(itable, proto->pdesc, proto->psz);
  int index = ProtoItem_Get(itable, rindex, pindex);
  if (index < 0) {
    ProtoItem *item = ProtoItem_New(rindex, pindex);
    index = ItemTable_Append(itable, ITEM_PROTO, item, 1);
  }
  return index;
}

/*-------------------------------------------------------------------------*/

static int codeitem_set(ItemTable *itable, CodeInfo *codeinfo)
{
  CodeItem *item = CodeItem_New(codeinfo);
  return ItemTable_Append(itable, ITEM_CODE, item, 0);
}

/*-------------------------------------------------------------------------*/

char *mapitem_string[] = {
  "map", "string", "type", "typelist", "proto", "const",
  "variable", "function",
  "field", "method", "struct",
  "imethod", "intf",
  "code",
};

int mapitem_length(void *o)
{
  UNUSED_PARAMETER(o);
  return sizeof(MapItem);
}

void mapitem_show(KLCImage *image, void *o)
{
  UNUSED_PARAMETER(image);
  MapItem *item = o;
  printf("  type:%s\n", mapitem_string[item->type]);
  printf("  offset:0x%x\n", item->offset);
  printf("  size:%d\n", item->size);
}

void mapitem_write(FILE *fp, void *o)
{
  fwrite(o, sizeof(MapItem), 1, fp);
}

int stringitem_length(void *o)
{
  StringItem *item = o;
  return sizeof(StringItem) + item->length;
}

uint32 stringitem_hash(void *k)
{
  StringItem *item = k;
  return hash_string(item->data);
}

int stringitem_equal(void *k1, void *k2)
{
  StringItem *item1 = k1;
  StringItem *item2 = k2;
  return strcmp(item1->data, item2->data) == 0;
}

void stringitem_write(FILE *fp, void *o)
{
  StringItem *item = o;
  fwrite(o, sizeof(StringItem) + item->length * sizeof(char), 1, fp);
}

void stringitem_show(KLCImage *image, void *o)
{
  UNUSED_PARAMETER(image);
  StringItem *item = o;
  printf("  length:%d\n", item->length);
  printf("  string:%s\n", item->data);
}

int typeitem_length(void *o)
{
  UNUSED_PARAMETER(o);
  return sizeof(TypeItem);
}

void typeitem_write(FILE *fp, void *o)
{
  fwrite(o, sizeof(TypeItem), 1, fp);
}

uint32 typeitem_hash(void *k)
{
  TypeItem *item = k;
  return hash_uint32(item->index, 32);
}

int typeitem_equal(void *k1, void *k2)
{
  TypeItem *item1 = k1;
  TypeItem *item2 = k2;
  if (item1->kind != item2->kind) return 0;
  if (item1->dims != item2->dims) return 0;
  if (item1->index != item2->index) return 0;
  return 1;
}

void typeitem_show(KLCImage *image, void *o)
{
  TypeItem *item = o;

  printf("  index:%d\n", item->index);
  if (item->kind == TYPE_DEFINED) {
    StringItem *str = ItemTable_Get(image->itable, ITEM_STRING, item->index);
    printf("  (%s)\n", str->data);
  } else {
    printf("  (%c)\n", item->primitive);
  }
}

int typelistitem_length(void *o)
{
  TypeListItem *item = o;
  return sizeof(TypeListItem) + item->size * sizeof(uint32);
}

void typelistitem_write(FILE *fp, void *o)
{
  TypeListItem *item = o;
  fwrite(o, sizeof(TypeListItem) + item->size * sizeof(uint32), 1, fp);
}

uint32 typelistitem_hash(void *k)
{
  TypeListItem *item = k;
  uint32 total = 0;
  for (int i = 0; i < (int)item->size; i++)
    total += item->index[i];
  return hash_uint32(total, 32);
}

int typelistitem_equal(void *k1, void *k2)
{
  TypeListItem *item1 = k1;
  TypeListItem *item2 = k2;
  if (item1->size != item2->size) return 0;
  return !memcmp(item1, item2, sizeof(TypeListItem) + item1->size);
}

void typelistitem_show(KLCImage *image, void *o)
{
  UNUSED_PARAMETER(image);
  UNUSED_PARAMETER(o);
}

int structitem_length(void *o)
{
  UNUSED_PARAMETER(o);
  return 0;
}

void structitem_show(KLCImage *image, void *o)
{
  UNUSED_PARAMETER(image);
  UNUSED_PARAMETER(o);
}

int intfitem_length(void *o)
{
  UNUSED_PARAMETER(o);
  return 0;
}

void intfitem_show(KLCImage *image, void *o)
{
  UNUSED_PARAMETER(image);
  UNUSED_PARAMETER(o);
}

int varitem_length(void *o)
{
  UNUSED_PARAMETER(o);
  return sizeof(VarItem);
}

void varitem_show(KLCImage *image, void *o)
{
  VarItem *item = o;
  StringItem *stritem;
  TypeItem *typeitem;

  printf("  name_index:%d\n", item->name_index);
  stritem = ItemTable_Get(image->itable, ITEM_STRING, item->name_index);
  printf("  (%s)\n", stritem->data);
  printf("  type_index:%d\n", item->type_index);
  typeitem = ItemTable_Get(image->itable, ITEM_TYPE, item->type_index);
  if (typeitem->kind == TYPE_DEFINED) {
    stritem = ItemTable_Get(image->itable, ITEM_STRING, typeitem->index);
    printf("  (%s)\n", stritem->data);
  } else {
    printf("  (%c)\n", typeitem->primitive);
  }
  printf("  flags:0x%x\n", item->flags);

}

void varitem_write(FILE *fp, void *o)
{
  fwrite(o, sizeof(VarItem), 1, fp);
}

int fielditem_length(void *o)
{
  UNUSED_PARAMETER(o);
  return 0;
}

void fielditem_show(KLCImage *image, void *o)
{
  UNUSED_PARAMETER(image);
  UNUSED_PARAMETER(o);
}

int protoitem_length(void *o)
{
  UNUSED_PARAMETER(o);
  return sizeof(ProtoItem);
}

void protoitem_write(FILE *fp, void *o)
{
  fwrite(o, sizeof(ProtoItem), 1, fp);
}

uint32 protoitem_hash(void *k)
{
  ProtoItem *item = k;
  uint32 total = item->rindex + item->pindex;
  return hash_uint32(total, 32);
}

int protoitem_equal(void *k1, void *k2)
{
  ProtoItem *item1 = k1;
  ProtoItem *item2 = k2;
  if (item1->rindex == item2->rindex &&
      item1->pindex == item2->pindex) {
    return 1;
  } else {
    return 0;
  }
}

void protoitem_show(KLCImage *image, void *o)
{
  UNUSED_PARAMETER(image);
  UNUSED_PARAMETER(o);
}

int funcitem_length(void *o)
{
  UNUSED_PARAMETER(o);
  return sizeof(FuncItem);
}

void funcitem_show(KLCImage *image, void *o)
{
  UNUSED_PARAMETER(image);
  UNUSED_PARAMETER(o);
}

void funcitem_write(FILE *fp, void *o)
{
  fwrite(o, sizeof(FuncItem), 1, fp);
}

int methoditem_length(void *o)
{
  UNUSED_PARAMETER(o);
  return 0;
}

void methoditem_show(KLCImage *image, void *o)
{
  UNUSED_PARAMETER(image);
  UNUSED_PARAMETER(o);
}

int codeitem_length(void *o)
{
  CodeItem *item = o;
  return item->size;
}

void codeitem_write(FILE *fp, void *o)
{
  CodeItem *item = o;
  fwrite(o, item->size, 1, fp);
}

void codeitem_show(KLCImage *image, void *o)
{
  UNUSED_PARAMETER(image);
  UNUSED_PARAMETER(o);
}

int constitem_length(void *o)
{
  UNUSED_PARAMETER(o);
  return sizeof(ConstItem);
}

void constitem_write(FILE *fp, void *o)
{
  fwrite(o, sizeof(ConstItem), 1, fp);
}

uint32 constitem_hash(void *k)
{
  uint32 hash = 0;
  ConstItem *item = k;
  switch (item->type) {
    case CONST_INT: {
      hash = hash_uint32((uint32)item->ival, 32);
      break;
    }
    case CONST_FLOAT: {
      hash = hash_uint32((uint32)item, 32);
      break;
    }
    case CONST_BOOL: {
      hash = hash_uint32((uint32)item, 32);
      break;
    }
    case CONST_STRING: {
      hash = hash_uint32(item->string_index, 32);
      break;
    }
    default: {
      ASSERT_MSG("unsupported %d const type\n", item->type);
      break;
    }
  }

  return hash;
}

int constitem_equal(void *k1, void *k2)
{
  int res = 0;
  ConstItem *item1 = k1;
  ConstItem *item2 = k2;
  if (item1->type != item2->type) return 0;
  switch (item1->type) {
    case CONST_INT: {
      res = (item1->ival == item2->ival);
      break;
    }
    case CONST_FLOAT: {
      res = (item1->fval == item2->fval);
      break;
    }
    case CONST_BOOL: {
      res = (item1 == item2);
      break;
    }
    case CONST_STRING: {
      res = (item1->string_index == item2->string_index);
      break;
    }
    default: {
      ASSERT_MSG("unsupported %d const type\n", item1->type);
      break;
    }
  }
  return res;
}

void constitem_show(KLCImage *image, void *o)
{
  UNUSED_PARAMETER(image);
  UNUSED_PARAMETER(o);
}

typedef int (*item_length_t)(void *);
typedef void (*item_fwrite_t)(FILE *, void *);
typedef uint32 (*item_hash_t)(void *);
typedef int (*item_equal_t)(void *, void *);
typedef void (*item_show_t)(KLCImage *, void *);

struct item_funcs {
  item_length_t   ilength;
  item_fwrite_t   iwrite;
  item_fwrite_t   iread;
  item_hash_t     ihash;
  item_equal_t    iequal;
  item_show_t  ishow;
};

struct item_funcs item_func[ITEM_MAX] = {
  {
    mapitem_length,
    mapitem_write, NULL,
    NULL, NULL,
    mapitem_show
  },
  {
    stringitem_length,
    stringitem_write, NULL,
    stringitem_hash, stringitem_equal,
    stringitem_show
  },
  {
    typeitem_length,
    typeitem_write, NULL,
    typeitem_hash, typeitem_equal,
    typeitem_show
  },
  {
    typelistitem_length,
    typelistitem_write, NULL,
    typelistitem_hash, typelistitem_equal,
    typelistitem_show
  },
  {
    protoitem_length,
    protoitem_write, NULL,
    protoitem_hash, protoitem_equal,
    protoitem_show
  },
  {
    constitem_length,
    constitem_write, NULL,
    constitem_hash, constitem_equal,
    constitem_show
  },
  {
    varitem_length,
    varitem_write, NULL,
    NULL, NULL,
    varitem_show
  },
  {
    funcitem_length,
    funcitem_write, NULL,
    NULL, NULL,
    funcitem_show
  },
  {
    fielditem_length,
    NULL, NULL,
    NULL, NULL,
    fielditem_show
  },
  {
    methoditem_length,
    NULL, NULL,
    NULL, NULL,
    methoditem_show
  },
  {
    structitem_length,
    NULL, NULL,
    NULL, NULL,
    structitem_show
  },
  {
    NULL,
    NULL, NULL,
    NULL, NULL,
    NULL
  },
  {
    intfitem_length,
    NULL, NULL,
    NULL, NULL,
    intfitem_show
  },
  {
    codeitem_length,
    codeitem_write, NULL,
    NULL, NULL,
    codeitem_show
  }
};

/*-------------------------------------------------------------------------*/

static void init_header(ImageHeader *h, int pkg_size)
{
  strcpy((char *)h->magic, "KLC");
  h->version[0] = '0' + version_major;
  h->version[1] = '0' + version_minor;
  h->version[2] = '0' + ((version_build >> 8) & 0xFF);
  h->version[3] = '0' + (version_build & 0xFF);
  h->file_size   = 0;
  h->header_size = sizeof(ImageHeader);
  h->endian_tag  = ENDIAN_TAG;
  h->map_offset  = sizeof(ImageHeader) + pkg_size;
  h->map_size    = ITEM_MAX;
  h->pkg_size    = pkg_size;
}

uint32 item_hash(void *key)
{
  ItemEntry *e = key;
  ASSERT(e->type > 0 && e->type < ITEM_MAX);
  item_hash_t hash_fn = item_func[e->type].ihash;
  ASSERT_PTR(hash_fn);
  return hash_fn(e->data);
}

int item_equal(void *k1, void *k2)
{
  ItemEntry *e1 = k1;
  ItemEntry *e2 = k2;
  ASSERT(e1->type > 0 && e1->type < ITEM_MAX);
  ASSERT(e2->type > 0 && e2->type < ITEM_MAX);
  if (e1->type != e2->type) return 0;
  item_equal_t equal_fn = item_func[e1->type].iequal;
  ASSERT_PTR(equal_fn);
  return equal_fn(e1->data, e2->data);
}

void KLCImage_Init(KLCImage *image, char *package)
{
  int pkg_size = ALIGN_UP(strlen(package) + 1, 4);
  image->package = malloc(pkg_size);
  strcpy(image->package, package);
  init_header(&image->header, pkg_size);
  Decl_HashInfo(hashinfo, item_hash, item_equal);
  image->itable = ItemTable_Create(&hashinfo, ITEM_MAX);
}

KLCImage *KLCImage_New(char *package)
{
  KLCImage *image = malloc(sizeof(*image));
  memset(image, 0, sizeof(*image));
  KLCImage_Init(image, package);
  return image;
}

void KLCImage_Free(KLCImage *image)
{
  free(image);
}

void KLCImage_Add_Var(KLCImage *image, char *name, TypeDesc *desc, int bconst)
{
  int flags = bconst ? ACCESS_CONST : 0;
  flags |= isupper(name[0]) ? ACCESS_PUBLIC : ACCESS_PRIVATE;
  int type_index = TypeItem_Set(image->itable, desc);
  int name_index = StringItem_Set(image->itable, name);
  VarItem *varitem = VarItem_New(name_index, type_index, flags);
  ItemTable_Append(image->itable, ITEM_VAR, varitem, 0);
}

void KLCImage_Add_Func(KLCImage *image, char *name, FuncInfo *info)
{
  int access = isupper(name[0]) ? ACCESS_PUBLIC : ACCESS_PRIVATE;
  int name_index = StringItem_Set(image->itable, name);
  int proto_index = ProtoItem_Set(image->itable, info->proto);
  int code_index = codeitem_set(image->itable, info->code);
  FuncItem *funcitem = FuncItem_New(name_index, proto_index, access,
                                    info->proto->vargs,
                                    info->proto->rsz, info->proto->psz,
                                    info->locals, code_index);
  ItemTable_Append(image->itable, ITEM_FUNC, funcitem, 0);
}

/*-------------------------------------------------------------------------*/

void KLCImage_Finish(KLCImage *image)
{
  int size, length = 0, offset = image->header.header_size;
  MapItem *mapitem;
  void *item;

  for (int i = 1; i < ITEM_MAX; i++) {
    size = ItemTable_Size(image->itable, i);
    if (size > 0) offset += sizeof(MapItem);
  }

  for (int i = 1; i < ITEM_MAX; i++) {
    size = ItemTable_Size(image->itable, i);
    if (size > 0) {
      offset += length;
      mapitem = MapItem_New(i, offset, size);
      ItemTable_Append(image->itable, ITEM_MAP, mapitem, 0);

      length = 0;
      for (int j = 0; j < size; j++) {
        item = ItemTable_Get(image->itable, i, j);
        length += item_func[i].ilength(item);
      }
    }
  }

  image->header.file_size = offset + length + image->header.pkg_size;
  image->header.map_size = ItemTable_Size(image->itable, 0);
}

static void __image_write_header(FILE *fp, KLCImage *image)
{
  fwrite(&image->header, image->header.header_size, 1, fp);
}

static void __image_write_item(FILE *fp, KLCImage *image, int type, int size)
{
  void *o;
  item_fwrite_t iwrite = item_func[type].iwrite;
  ASSERT_PTR(iwrite);
  for (int i = 0; i < size; i++) {
    o = ItemTable_Get(image->itable, type, i);
    iwrite(fp, o);
  }
}

static void __image_write_pkgname(FILE *fp, KLCImage *image)
{
  fwrite(image->package, image->header.pkg_size, 1, fp);
}

static void __image_write_items(FILE *fp, KLCImage *image)
{
  int size;
  for (int i = 0; i < ITEM_MAX; i++) {
    size = ItemTable_Size(image->itable, i);
    if (size > 0) {
      __image_write_item(fp, image, i, size);
    }
  }
}

void KLCImage_Write_File(KLCImage *image, char *path)
{
  FILE *fp = fopen(path, "w");
  ASSERT_PTR(fp);
  __image_write_header(fp, image);
  __image_write_pkgname(fp, image);
  __image_write_items(fp, image);
  fclose(fp);
}

KLCImage *KLCImage_Read_File(char *path)
{
  FILE *fp = fopen(path, "r");
  if (fp == NULL) {
    printf("[ERROR] cannot open file:%s\n", path);
    return NULL;
  }

  ImageHeader header;
  int sz = fread(&header, sizeof(ImageHeader), 1, fp);
  if (sz < 1) {
    printf("[ERROR] file %s is not a valid .klc file\n", path);
    return NULL;
  }

  // if (header_check(&header) < 0) {
  //   printf("[ERROR] file %s is not a valid .klc file\n", path);
  //   return NULL;
  // }

  char pkg_name[header.pkg_size];
  sz = fread(pkg_name, sizeof(pkg_name), 1, fp);
  if (sz < 1) {
    printf("[ERROR] read file %s error\n", path);
    return NULL;
  }

  KLCImage *image = KLCImage_New(pkg_name);
  ASSERT_PTR(image);
  image->header = header;

  MapItem mapitems[header.map_size];
  sz = fseek(fp, header.map_offset, SEEK_SET);
  ASSERT(sz == 0);
  sz = fread(mapitems, sizeof(MapItem), header.map_size, fp);
  if (sz < (int)header.map_size) {
    printf("[ERROR] file %s is not a valid .klc file\n", path);
    return NULL;
  }

  MapItem *mapitem;
  for (int i = 0; i < nr_elts(mapitems); i++) {
    mapitem = mapitems + i;
    mapitem = MapItem_New(mapitem->type, mapitem->offset, mapitem->size);
    ItemTable_Append(image->itable, ITEM_MAP, mapitem, 0);
  }

  return image;
}

/*-------------------------------------------------------------------------*/

void header_show(ImageHeader *h)
{
  printf("header:\n");
  printf("magic:%s\n", (char *)h->magic);
  printf("version:%d.%d.%d\n", h->version[0] - '0', h->version[1] - '0', 0);
  printf("header_size:%d\n", h->header_size);
  printf("endian_tag:0x%x\n", h->endian_tag);
  printf("map_size:%d\n", h->map_size);
  printf("--------------------\n");
}

void KLCImage_Show(KLCImage *image)
{
  void *item;
  ImageHeader *h = &image->header;
  header_show(h);
  int size;

  printf("package:%s\n", image->package);
  printf("--------------------\n");

  printf("mapitems:\n");
  size = ItemTable_Size(image->itable, 0);
  for (int j = 0; j < size; j++) {
    printf("[%d]\n", j);
    item = ItemTable_Get(image->itable, 0, j);
    item_func[0].ishow(image, item);
  }
  printf("--------------------\n");

  for (int i = 1; i < ITEM_MAX; i++) {
    size = ItemTable_Size(image->itable, i);
    if (size > 0) {
      printf("%sitems:\n", mapitem_string[i]);
      for (int j = 0; j < size; j++) {
        printf("[%d]\n", j);
        item = ItemTable_Get(image->itable, i, j);
        item_func[i].ishow(image, item);
      }
      printf("--------------------\n");
    }
  }
}

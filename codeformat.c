
#include "codeformat.h"
#include "hash.h"

static int version_major = 0; // 1 byte
static int version_minor = 1; // 1 byte
static int version_build = 1; // 2 bytes

#define ENDIAN_TAG  0x1a2b3c4d

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
  int size = strlen(name);
  StringItem *item = malloc(sizeof(StringItem) + size + 1);
  item->length = size + 1;
  strcpy(item->data, name);
  return item;
}

TypeItem *TypeItem_New(uint32 index)
{
  TypeItem *item = malloc(sizeof(*item));
  item->desc_index = index;
  return item;
}

TypeListItem *TypeListItem_New(int size, uint32 type_index[])
{
  TypeListItem *item = malloc(sizeof(*item) + size * sizeof(uint32));
  item->size = size;
  for (int i = 0; i < size; i++) {
    item->desc_index[i] = type_index[i];
  }
  return item;
}

VarItem *VarItem_New(uint32 name_index, uint32 type_index, int flags)
{
  VarItem *item = malloc(sizeof(*item));
  item->name_index = name_index;
  item->type_index = type_index;
  item->flags = flags;
  return item;
}

ProtoItem *ProtoItem_New(int rindex, int pindex)
{
  ProtoItem *item = malloc(sizeof(*item));
  item->return_index = rindex;
  item->parameter_index = pindex;
  return item;
}

FuncItem *FuncItem_New(int name_index, int proto_index, int flags,
                       int rsz, int psz, int nr_locals, int code_index)
{
  FuncItem *item = malloc(sizeof(*item));
  item->name_index = name_index;
  item->proto_index = proto_index;
  item->flags = flags;
  item->nr_returns = rsz;
  item->nr_paras = psz;
  item->nr_returns = nr_locals;
  item->code_index = code_index;
  return item;
}

CodeItem *CodeItem_New(uint8 *code, int size)
{
  CodeItem *item = malloc(sizeof(*item) + size);
  item->size = size;
  memcpy(item->insts, code, size);
  return item;
}

/*-------------------------------------------------------------------------*/

int StringItem_Get(ItemTable *itemtable, char *str)
{
  int len = strlen(str);
  uint8 data[sizeof(StringItem) + len + 1];
  StringItem *item = (StringItem *)data;
  item->length = len + 1;
  strcpy(item->data, str);
  return ItemTable_Index(itemtable, ITEM_STRING, item);
}

int StringItem_Set(ItemTable *itemtable, char *str)
{
  int index = StringItem_Get(itemtable, str);

  if (index < 0) {
    StringItem *item = StringItem_New(str);
    index = ItemTable_Append(itemtable, ITEM_STRING, &item, 1);
  }

  return index;
}

int TypeItem_Get(ItemTable *itemtable, char *str)
{
  int str_index = StringItem_Get(itemtable, str);
  if (str_index < 0) {
    return str_index;
  }

  TypeItem item = {str_index};
  return ItemTable_Index(itemtable, ITEM_TYPE, &item);
}

int TypeItem_Set(ItemTable *itemtable, char *str)
{
  int index = TypeItem_Get(itemtable, str);
  if (index < 0) {
    int str_index = StringItem_Set(itemtable, str);
    assert(str_index >= 0);
    TypeItem *item = TypeItem_New(str_index);
    index = ItemTable_Append(itemtable, ITEM_TYPE, &item, 1);
  }
  return index;
}

int TypeListItem_Get(ItemTable *itemtable, char *desc[], int sz)
{
  int index;
  int desc_index[sz];
  for (int i = 0; i < sz; i++) {
    index = StringItem_Get(itemtable, desc[i]);
    if (index < 0) return -1;
    desc_index[i] = index;
  }

  uint8 data[sizeof(TypeListItem) + sizeof(uint32) * sz];
  TypeListItem *item = (TypeListItem *)data;
  item->size = sz;
  for (int i = 0; i < sz; i++) {
    item->desc_index[i] = desc_index[i];
  }

  return ItemTable_Index(itemtable, ITEM_TYPELIST, item);
}

int TypeListItem_Set(ItemTable *itemtable, char *desc[], int sz)
{
  int index = TypeListItem_Get(itemtable, desc, sz);
  if (index < 0) {
    int index;
    uint32 desc_index[sz];
    for (int i = 0; i < sz; i++) {
      index = StringItem_Set(itemtable, desc[i]);
      assert(index >= 0);
      desc_index[i] = index;
    }
    TypeListItem *item = TypeListItem_New(sz, desc_index);
    index = ItemTable_Append(itemtable, ITEM_TYPELIST, &item, 1);
  }
  return index;
}

int ProtoItem_Get(ItemTable *itemtable, int rindex, int pindex)
{
  ProtoItem item = {rindex, pindex};
  return ItemTable_Index(itemtable, ITEM_PROTO, &item);
}

int ProtoItem_Set(ItemTable *itemtable, char *rdesc[], int rsz,
                  char *pdesc[], int psz)
{
  int rindex = TypeListItem_Set(itemtable, rdesc, rsz);
  int pindex = TypeListItem_Set(itemtable, pdesc, psz);
  int index = ProtoItem_Get(itemtable, rindex, pindex);
  if (index < 0) {
    ProtoItem *item = ProtoItem_New(rindex, pindex);
    index = ItemTable_Append(itemtable, ITEM_PROTO, &item, 1);
  }
  return index;
}

/*-------------------------------------------------------------------------*/

static int codeitem_set(ItemTable *itemtable, uint8 *code, int csz)
{
  CodeItem *item = CodeItem_New(code, csz);
  return ItemTable_Append(itemtable, ITEM_CODE, &item, 0);
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

void mapitem_display(KLCImage *image, void *o)
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

void stringitem_display(KLCImage *image, void *o)
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
  return hash_uint32(item->desc_index, 32);
}

int typeitem_equal(void *k1, void *k2)
{
  TypeItem *item1 = k1;
  TypeItem *item2 = k2;
  return item1->desc_index == item2->desc_index;
}

void typeitem_display(KLCImage *image, void *o)
{
  TypeItem *item = o;
  StringItem *stritem;

  printf("  desc_index:%d\n", item->desc_index);
  stritem = ItemTable_Get(image->table, ITEM_STRING, item->desc_index);
  printf("  (%s)\n", stritem->data);
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
    total += item->desc_index[i];
  return hash_uint32(total, 32);
}

int typelistitem_equal(void *k1, void *k2)
{
  TypeListItem *item1 = k1;
  TypeListItem *item2 = k2;
  if (item1->size != item2->size) return 0;
  return memcmp(item1, item2, sizeof(TypeListItem) + item1->size) == 0;
}

void typelistitem_display(KLCImage *image, void *o)
{
  UNUSED_PARAMETER(image);
  UNUSED_PARAMETER(o);
}

int structitem_length(void *o)
{
  UNUSED_PARAMETER(o);
  return 0;
}

void structitem_display(KLCImage *image, void *o)
{
  UNUSED_PARAMETER(image);
  UNUSED_PARAMETER(o);
}

int intfitem_length(void *o)
{
  UNUSED_PARAMETER(o);
  return 0;
}

void intfitem_display(KLCImage *image, void *o)
{
  UNUSED_PARAMETER(image);
  UNUSED_PARAMETER(o);
}

int varitem_length(void *o)
{
  UNUSED_PARAMETER(o);
  return sizeof(VarItem);
}

void varitem_display(KLCImage *image, void *o)
{
  VarItem *item = o;
  StringItem *stritem;
  TypeItem *typeitem;

  printf("  name_index:%d\n", item->name_index);
  stritem = ItemTable_Get(image->table, ITEM_STRING, item->name_index);
  printf("  (%s)\n", stritem->data);
  printf("  type_index:%d\n", item->type_index);
  typeitem = ItemTable_Get(image->table, ITEM_TYPE, item->type_index);
  stritem = ItemTable_Get(image->table, ITEM_STRING, typeitem->desc_index);
  printf("  (%s)\n", stritem->data);
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

void fielditem_display(KLCImage *image, void *o)
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
  uint32 total = item->return_index + item->parameter_index;
  return hash_uint32(total, 32);
}

int protoitem_equal(void *k1, void *k2)
{
  ProtoItem *item1 = k1;
  ProtoItem *item2 = k2;
  if (item1->return_index == item2->return_index &&
      item1->parameter_index == item2->parameter_index) {
    return 1;
  } else {
    return 0;
  }
}

void protoitem_display(KLCImage *image, void *o)
{
  UNUSED_PARAMETER(image);
  UNUSED_PARAMETER(o);
}

int funcitem_length(void *o)
{
  UNUSED_PARAMETER(o);
  return sizeof(FuncItem);
}

void funcitem_display(KLCImage *image, void *o)
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

void methoditem_display(KLCImage *image, void *o)
{
  UNUSED_PARAMETER(image);
  UNUSED_PARAMETER(o);
}

int codeitem_length(void *o)
{
  CodeItem *item = o;
  return sizeof(CodeItem) + item->size;
}

void codeitem_write(FILE *fp, void *o)
{
  CodeItem *item = o;
  fwrite(o, sizeof(CodeItem) + item->size, 1, fp);
}

void codeitem_display(KLCImage *image, void *o)
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
      hash = hash_uint32((uint32)item->value.ival, 32);
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
      hash = hash_uint32(item->value.string_index, 32);
      break;
    }
    default: {
      printf("[ERROR]unsupported %d const type\n", item->type);
      assert(0);
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
      res = (item1->value.ival == item2->value.ival);
      break;
    }
    case CONST_FLOAT: {
      res = (item1->value.fval == item2->value.fval);
      break;
    }
    case CONST_BOOL: {
      res = (item1 == item2);
      break;
    }
    case CONST_STRING: {
      res = (item1->value.string_index == item2->value.string_index);
      break;
    }
    default: {
      printf("[ERROR]unsupported %d const type\n", item1->type);
      assert(0);
      break;
    }
  }
  return res;
}

void constitem_display(KLCImage *image, void *o)
{
  UNUSED_PARAMETER(image);
  UNUSED_PARAMETER(o);
}

struct item_funcs item_func[ITEM_MAX] = {
  {
    mapitem_length,
    mapitem_write, NULL,
    NULL, NULL,
    mapitem_display
  },
  {
    stringitem_length,
    stringitem_write, NULL,
    stringitem_hash, stringitem_equal,
    stringitem_display
  },
  {
    typeitem_length,
    typeitem_write, NULL,
    typeitem_hash, typeitem_equal,
    typeitem_display
  },
  {
    typelistitem_length,
    typelistitem_write, NULL,
    typelistitem_hash, typelistitem_equal,
    typelistitem_display
  },
  {
    protoitem_length,
    protoitem_write, NULL,
    protoitem_hash, protoitem_equal,
    protoitem_display
  },
  {
    constitem_length,
    constitem_write, NULL,
    constitem_hash, constitem_equal,
    constitem_display
  },
  {
    varitem_length,
    varitem_write, NULL,
    NULL, NULL,
    varitem_display
  },
  {
    funcitem_length,
    funcitem_write, NULL,
    NULL, NULL,
    funcitem_display
  },
  {
    fielditem_length,
    NULL, NULL,
    NULL, NULL,
    fielditem_display
  },
  {
    methoditem_length,
    NULL, NULL,
    NULL, NULL,
    methoditem_display
  },
  {
    structitem_length,
    NULL, NULL,
    NULL, NULL,
    structitem_display
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
    intfitem_display
  },
  {
    codeitem_length,
    codeitem_write, NULL,
    NULL, NULL,
    codeitem_display
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

static uint32 htable_hash(void *key)
{
  ItemEntry *e = key;
  item_hash_t hash_fn = item_func[e->type].ihash;
  assert(hash_fn != NULL);
  return hash_fn(e->data);
}

static int htable_equal(void *k1, void *k2)
{
  ItemEntry *e1 = k1;
  ItemEntry *e2 = k2;
  if (e1->type != e2->type) return 0;
  item_equal_t equal_fn = item_func[e1->type].iequal;
  assert(equal_fn != NULL);
  return equal_fn(e1->data, e2->data);
}

void KLCImage_Init(KLCImage *image, char *package)
{
  int pkg_size = ALIGN_UP(strlen(package) + 1, 4);
  image->package = malloc(pkg_size);
  strcpy(image->package, package);

  init_header(&image->header, pkg_size);

  image->table = ItemTable_Create(htable_hash, htable_equal, ITEM_MAX);
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

void KLCImage_Add_Var(KLCImage *image, char *name, int flags, char *desc)
{
  int type_index = TypeItem_Set(image->table, desc);
  int name_index = StringItem_Set(image->table, name);
  VarItem *varitem = VarItem_New(name_index, type_index, flags);
  ItemTable_Append(image->table, ITEM_VAR, &varitem, 0);
}

void KLCImage_Add_Func(KLCImage *image, char *name, int flags, int nr_locals,
                       char *desc[], int rsz, char *pdesc[], int psz,
                       uint8 *code, int csz)
{
  int name_index = StringItem_Set(image->table, name);
  int proto_index = ProtoItem_Set(image->table, desc, rsz, pdesc, psz);
  int code_index = codeitem_set(image->table, code, csz);
  FuncItem *funcitem = FuncItem_New(name_index, proto_index, flags,
                                    rsz, psz, nr_locals, code_index);
  ItemTable_Append(image->table, ITEM_FUNC, &funcitem, 0);
}

/*-------------------------------------------------------------------------*/

void KLCImage_Finish(KLCImage *image)
{
  int size, length = 0, offset = image->header.header_size;
  MapItem *mapitem;
  void *item;

  for (int i = 1; i < ITEM_MAX; i++) {
    size = ItemTable_Size(image->table, i);
    if (size > 0) offset += sizeof(MapItem);
  }

  for (int i = 1; i < ITEM_MAX; i++) {
    size = ItemTable_Size(image->table, i);
    if (size > 0) {
      offset += length;
      mapitem = MapItem_New(i, offset, size);
      ItemTable_Append(image->table, ITEM_MAP, &mapitem, 0);

      length = 0;
      for (int j = 0; j < size; j++) {
        item = ItemTable_Get(image->table, i, j);
        length += item_func[i].ilength(item);
      }
    }
  }

  image->header.file_size = offset + length + image->header.pkg_size;
  image->header.map_size = ItemTable_Size(image->table, 0);
}

static void __image_write_header(FILE *fp, KLCImage *image)
{
  fwrite(&image->header, image->header.header_size, 1, fp);
}

static void __image_write_item(FILE *fp, KLCImage *image, int type, int size)
{
  void *o;
  item_fwrite_t iwrite = item_func[type].iwrite;
  assert(iwrite);
  for (int i = 0; i < size; i++) {
    o = ItemTable_Get(image->table, type, i);
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
    size = ItemTable_Size(image->table, i);
    if (size > 0) {
      __image_write_item(fp, image, i, size);
    }
  }
}

void KLCImage_Write_File(KLCImage *image, char *path)
{
  FILE *fp = fopen(path, "w");
  assert(fp != NULL);
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
  assert(image);
  image->header = header;

  MapItem mapitems[header.map_size];
  sz = fseek(fp, header.map_offset, SEEK_SET);
  assert(sz == 0);
  sz = fread(mapitems, sizeof(MapItem), header.map_size, fp);
  if (sz < (int)header.map_size) {
    printf("[ERROR] file %s is not a valid .klc file\n", path);
    return NULL;
  }

  MapItem *mapitem;
  for (int i = 0; i < nr_elts(mapitems); i++) {
    mapitem = mapitems + i;
    mapitem = MapItem_New(mapitem->type, mapitem->offset, mapitem->size);
    ItemTable_Append(image->table, ITEM_MAP, &mapitem, 0);
  }

  return image;
}

/*-------------------------------------------------------------------------*/

void header_display(ImageHeader *h)
{
  printf("header:\n");
  printf("magic:%s\n", (char *)h->magic);
  printf("version:%d.%d.%d\n", h->version[0] - '0', h->version[1] - '0', 0);
  printf("header_size:%d\n", h->header_size);
  printf("endian_tag:0x%x\n", h->endian_tag);
  printf("map_size:%d\n", h->map_size);
  printf("--------------------\n");
}

void KLCImage_Display(KLCImage *image)
{
  void *item;
  ImageHeader *h = &image->header;
  header_display(h);
  int size;

  printf("package:%s\n", image->package);
  printf("--------------------\n");

  printf("mapitems:\n");
  size = ItemTable_Size(image->table, 0);
  for (int j = 0; j < size; j++) {
    printf("[%d]\n", j);
    item = ItemTable_Get(image->table, 0, j);
    item_func[0].idisplay(image, item);
  }
  printf("--------------------\n");

  for (int i = 1; i < ITEM_MAX; i++) {
    size = ItemTable_Size(image->table, i);
    if (size > 0) {
      printf("%sitems:\n", mapitem_string[i]);
      for (int j = 0; j < size; j++) {
        printf("[%d]\n", j);
        item = ItemTable_Get(image->table, i, j);
        item_func[i].idisplay(image, item);
      }
      printf("--------------------\n");
    }
  }
}

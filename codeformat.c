
#include "codeformat.h"
#include "hash.h"

static int version_major = 0; // 1 byte
static int version_minor = 1; // 1 byte
static int version_build = 1; // 2 bytes

#define ENDIAN_TAG  0x1a2b3c4d

/*-------------------------------------------------------------------------*/

ItemEntry *ItemEntry_New(int type, int index, void *data)
{
  ItemEntry *e = malloc(sizeof(*e));
  init_hash_node(&e->hnode, e);
  e->itype = type;
  e->index = index;
  e->data  = data;
  return e;
}

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

TypeListItem *TypeListItem_New(int size, int type_index[])
{
  TypeListItem *item = malloc(sizeof(*item) + size * sizeof(uint32));
  item->size = size;
  for (int i = 0; i < size; i++) {
    item->type_index[i] = (uint32)type_index[i];
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
  item->return_off = rindex;
  item->parameter_off = pindex;
  return item;
}

FuncItem *FuncItem_New(int name_index, int proto_index, int flags,
                       int rsz, int psz, int nr_locals, int code_off)
{
  FuncItem *item = malloc(sizeof(*item));
  item->name_index = name_index;
  item->proto_index = proto_index;
  item->flags = flags;
  item->nr_returns = rsz;
  item->nr_paras = psz;
  item->nr_returns = nr_locals;
  item->code_off = code_off;
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

static int item_index(KLCFile *filp, int type, void *data)
{
  ItemEntry e = ITEM_ENTRY_INIT(type, 0, data);
  struct hash_node *hnode = hash_table_find(&filp->table, &e);
  if (hnode == NULL) return -1;
  return container_of(hnode, ItemEntry, hnode)->index;
}

static int item_set(KLCFile *filp, int type, void *item, int inhash)
{
  int index = Vector_Set(&filp->items[type], filp->sizes[type]++, item);
  if (inhash) {
    ItemEntry *e = ItemEntry_New(type, index, item);
    int res = hash_table_insert(&filp->table, &e->hnode);
    assert(!res);
  }
  return index;
}

static void *item_get(KLCFile *filp, int type, int index)
{
  return Vector_Get(&filp->items[type], index);
}

static int stringitem_get(KLCFile *filp, char *str)
{
  int len = strlen(str);
  uint8 data[sizeof(StringItem) + len + 1];
  StringItem *item = (StringItem *)data;
  item->length = len + 1;
  strcpy(item->data, str);
  return item_index(filp, ITYPE_STRING, item);
}

static int stringitem_set(KLCFile *filp, char *str)
{
  int index = stringitem_get(filp, str);

  if (index < 0) {
    StringItem *item = StringItem_New(str);
    index = item_set(filp, ITYPE_STRING, item, 1);
  }

  return index;
}

static int typeitem_get(KLCFile *filp, char *str)
{
  int str_index = stringitem_get(filp, str);
  if (str_index < 0) {
    return str_index;
  }

  TypeItem item = {str_index};
  return item_index(filp, ITYPE_TYPE, &item);
}

static int typeitem_set(KLCFile *filp, char *str)
{
  int index = typeitem_get(filp, str);
  if (index < 0) {
    int str_index = stringitem_set(filp, str);
    assert(str_index >= 0);
    TypeItem *item = TypeItem_New(str_index);
    index = item_set(filp, ITYPE_TYPE, item, 1);
  }
  return index;
}

static int typelistitem_get(KLCFile *filp, char *desc[], int sz)
{
  int index;
  int type_index[sz];
  for (int i = 0; i < sz; i++) {
    index = typeitem_get(filp, desc[i]);
    if (index < 0) return -1;
    type_index[i] = index;
  }

  uint8 data[sizeof(TypeListItem) + sizeof(uint32) * sz];
  TypeListItem *item = (TypeListItem *)data;
  item->size = sz;
  for (int i = 0; i < sz; i++) {
    item->type_index[i] = type_index[i];
  }

  return item_index(filp, ITYPE_TYPELIST, item);
}

static int typelistitem_set(KLCFile *filp, char *desc[], int sz)
{
  int index = typelistitem_get(filp, desc, sz);
  if (index < 0) {
    int index;
    int type_index[sz];
    for (int i = 0; i < sz; i++) {
      index = typeitem_set(filp, desc[i]);
      assert(index >= 0);
      type_index[i] = index;
    }
    TypeListItem *item = TypeListItem_New(sz, type_index);
    index = item_set(filp, ITYPE_TYPELIST, item, 1);
  }
  return index;
}

static int protoitem_get(KLCFile *filp, int rindex, int pindex)
{
  ProtoItem item = {rindex, pindex};
  return item_index(filp, ITYPE_PROTO, &item);
}

static int protoitem_set(KLCFile *filp, char *rdesc[], int rsz,
                         char *pdesc[], int psz)
{
  int rindex = typelistitem_set(filp, rdesc, rsz);
  int pindex = typelistitem_set(filp, pdesc, psz);
  int index = protoitem_get(filp, rindex, pindex);
  if (index < 0) {
    ProtoItem *item = ProtoItem_New(rindex, pindex);
    index = item_set(filp, ITYPE_PROTO, item, 1);
  }
  return index;
}

static int codeitem_set(KLCFile *filp, uint8 *code, int csz)
{
  CodeItem *item = CodeItem_New(code, csz);
  return item_set(filp, ITYPE_CODE, item, 0);
}

/*-------------------------------------------------------------------------*/

char *mapitem_string[] = {
  "map", "string", "type", "typelist", "struct", "interface",
  "variable", "field", "proto", "function", "method", "code", "const"
};

int map_item_len(void *o)
{
  UNUSED_PARAMETER(o);
  return sizeof(MapItem);
}

void map_item_display(KLCFile *filp, void *o)
{
  UNUSED_PARAMETER(filp);
  MapItem *item = o;
  printf("  type:%s\n", mapitem_string[item->type]);
  printf("  offset:0x%x\n", item->offset);
  printf("  size:%d\n", item->size);
}

void map_item_write(KLCFile *filp, void *o)
{
  fwrite(o, sizeof(MapItem), 1, filp->fp);
}

int string_item_len(void *o)
{
  StringItem *item = o;
  return sizeof(StringItem) + item->length;
}

uint32 string_item_hash(void *k)
{
  StringItem *item = k;
  return hash_string(item->data);
}

int string_item_equal(void *k1, void *k2)
{
  StringItem *item1 = k1;
  StringItem *item2 = k2;
  return strcmp(item1->data, item2->data) == 0;
}

void string_item_write(KLCFile *filp, void *o)
{
  StringItem *item = o;
  fwrite(o, sizeof(StringItem) + item->length * sizeof(char), 1, filp->fp);
}

void string_item_display(KLCFile *filp, void *o)
{
  UNUSED_PARAMETER(filp);
  StringItem *item = o;
  printf("  length:%d\n", item->length);
  printf("  string:%s\n", item->data);
}

int type_item_len(void *o)
{
  UNUSED_PARAMETER(o);
  return sizeof(TypeItem);
}

void type_item_write(KLCFile *filp, void *o)
{
  fwrite(o, sizeof(TypeItem), 1, filp->fp);
}

uint32 type_item_hash(void *k)
{
  TypeItem *item = k;
  return hash_uint32(item->desc_index, 32);
}

int type_item_equal(void *k1, void *k2)
{
  TypeItem *item1 = k1;
  TypeItem *item2 = k2;
  return item1->desc_index == item2->desc_index;
}

void type_item_display(KLCFile *filp, void *o)
{
  UNUSED_PARAMETER(filp);
  TypeItem *item = o;
  StringItem *stritem;

  printf("  desc_index:%d\n", item->desc_index);
  stritem = item_get(filp, ITYPE_STRING, item->desc_index);
  printf("  (%s)\n", stritem->data);
}

int typelist_item_len(void *o)
{
  TypeListItem *item = o;
  return sizeof(TypeListItem) + item->size * sizeof(uint32);
}

void typelist_item_write(KLCFile *filp, void *o)
{
  TypeListItem *item = o;
  fwrite(o, sizeof(TypeListItem) + item->size * sizeof(uint32), 1, filp->fp);
}

uint32 typelist_item_hash(void *k)
{
  TypeListItem *item = k;
  uint32 total = 0;
  for (int i = 0; i < (int)item->size; i++)
    total += item->type_index[i];
  return hash_uint32(total, 32);
}

int typelist_item_equal(void *k1, void *k2)
{
  TypeListItem *item1 = k1;
  TypeListItem *item2 = k2;
  if (item1->size != item2->size) return 0;
  return memcmp(item1, item2, sizeof(TypeListItem) + item1->size) == 0;
}

void typelist_item_display(KLCFile *filp, void *o)
{
  UNUSED_PARAMETER(filp);
  UNUSED_PARAMETER(o);
}

int struct_item_len(void *o)
{
  UNUSED_PARAMETER(o);
  return 0;
}

void struct_item_display(KLCFile *filp, void *o)
{
  UNUSED_PARAMETER(filp);
  UNUSED_PARAMETER(o);
}

int intf_item_len(void *o)
{
  UNUSED_PARAMETER(o);
  return 0;
}

void intf_item_display(KLCFile *filp, void *o)
{
  UNUSED_PARAMETER(filp);
  UNUSED_PARAMETER(o);
}

int var_item_len(void *o)
{
  UNUSED_PARAMETER(o);
  return sizeof(VarItem);
}

void var_item_display(KLCFile *filp, void *o)
{
  UNUSED_PARAMETER(filp);
  VarItem *item = o;
  StringItem *stritem;
  TypeItem *typeitem;

  printf("  name_index:%d\n", item->name_index);
  stritem = item_get(filp, ITYPE_STRING, item->name_index);
  printf("  (%s)\n", stritem->data);
  printf("  type_index:%d\n", item->type_index);
  typeitem = item_get(filp, ITYPE_TYPE, item->type_index);
  stritem = item_get(filp, ITYPE_STRING, typeitem->desc_index);
  printf("  (%s)\n", stritem->data);
  printf("  flags:0x%x\n", item->flags);

}

void var_item_write(KLCFile *filp, void *o)
{
  fwrite(o, sizeof(VarItem), 1, filp->fp);
}

int field_item_len(void *o)
{
  UNUSED_PARAMETER(o);
  return 0;
}

void field_item_display(KLCFile *filp, void *o)
{
  UNUSED_PARAMETER(filp);
  UNUSED_PARAMETER(o);
}

int proto_item_len(void *o)
{
  UNUSED_PARAMETER(o);
  return sizeof(ProtoItem);
}

void proto_item_write(KLCFile *filp, void *o)
{
  fwrite(o, sizeof(ProtoItem), 1, filp->fp);
}

uint32 proto_item_hash(void *k)
{
  ProtoItem *item = k;
  uint32 total = item->return_off + item->parameter_off;
  return hash_uint32(total, 32);
}

int proto_item_equal(void *k1, void *k2)
{
  ProtoItem *item1 = k1;
  ProtoItem *item2 = k2;
  if (item1->return_off == item2->return_off &&
      item1->parameter_off == item2->parameter_off) {
    return 1;
  } else {
    return 0;
  }
}

void proto_item_display(KLCFile *filp, void *o)
{
  UNUSED_PARAMETER(filp);
  UNUSED_PARAMETER(o);
}

int func_item_len(void *o)
{
  UNUSED_PARAMETER(o);
  return sizeof(FuncItem);
}

void func_item_display(KLCFile *filp, void *o)
{
  UNUSED_PARAMETER(filp);
  UNUSED_PARAMETER(o);
}

void func_item_write(KLCFile *filp, void *o)
{
  fwrite(o, sizeof(FuncItem), 1, filp->fp);
}

int method_item_len(void *o)
{
  UNUSED_PARAMETER(o);
  return 0;
}

void method_item_display(KLCFile *filp, void *o)
{
  UNUSED_PARAMETER(filp);
  UNUSED_PARAMETER(o);
}

int code_item_len(void *o)
{
  CodeItem *item = o;
  return sizeof(CodeItem) + item->size;
}

void code_item_write(KLCFile *filp, void *o)
{
  CodeItem *item = o;
  fwrite(o, sizeof(CodeItem) + item->size, 1, filp->fp);
}

void code_item_display(KLCFile *filp, void *o)
{
  UNUSED_PARAMETER(filp);
  UNUSED_PARAMETER(o);
}

int konst_item_len(void *o)
{
  UNUSED_PARAMETER(o);
  return sizeof(KonstItem);
}

void konst_item_write(KLCFile *filp, void *o)
{
  fwrite(o, sizeof(KonstItem), 1, filp->fp);
}

uint32 konst_item_hash(void *k)
{
  uint32 hash = 0;
  KonstItem *item = k;
  switch (item->type) {
    case KTYPE_INT: {
      hash = hash_uint32((uint32)item->value.ival, 32);
      break;
    }
    case KTYPE_FLOAT: {
      hash = hash_uint32((uint32)item, 32);
      break;
    }
    case KTYPE_BOOL: {
      hash = hash_uint32((uint32)item, 32);
      break;
    }
    case KTYPE_STRING: {
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

int konst_item_equal(void *k1, void *k2)
{
  int res = 0;
  KonstItem *item1 = k1;
  KonstItem *item2 = k2;
  if (item1->type != item2->type) return 0;
  switch (item1->type) {
    case KTYPE_INT: {
      res = (item1->value.ival == item2->value.ival);
      break;
    }
    case KTYPE_FLOAT: {
      res = (item1->value.fval == item2->value.fval);
      break;
    }
    case KTYPE_BOOL: {
      res = (item1 == item2);
      break;
    }
    case KTYPE_STRING: {
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

void konst_item_display(KLCFile *filp, void *o)
{
  UNUSED_PARAMETER(filp);
  UNUSED_PARAMETER(o);
}

typedef int (*item_length_t)(void *);
typedef void (*item_fwrite_t)(KLCFile *, void *);
typedef uint32 (*item_hash_t)(void *);
typedef int (*item_equal_t)(void *, void *);
typedef void (*item_display_t)(KLCFile *, void *);

struct {
  item_length_t   ilength;
  item_fwrite_t   iwrite;
  item_fwrite_t   iread;
  item_hash_t     ihash;
  item_equal_t    iequal;
  item_display_t  idisplay;
} item_func[ITYPE_MAX] = {
  {
    map_item_len,
    map_item_write, NULL,
    NULL, NULL,
    map_item_display
  },
  {
    string_item_len,
    string_item_write, NULL,
    string_item_hash, string_item_equal,
    string_item_display
  },
  {
    type_item_len,
    type_item_write, NULL,
    type_item_hash, type_item_equal,
    type_item_display
  },
  {
    typelist_item_len,
    typelist_item_write, NULL,
    typelist_item_hash, typelist_item_equal,
    typelist_item_display
  },
  {
    struct_item_len,
    NULL, NULL,
    NULL, NULL,
    struct_item_display
  },
  {
    intf_item_len,
    NULL, NULL,
    NULL, NULL,
    intf_item_display
  },
  {
    var_item_len,
    var_item_write, NULL,
    NULL, NULL,
    var_item_display
  },
  {
    field_item_len,
    NULL, NULL,
    NULL, NULL,
    field_item_display
  },
  {
    proto_item_len,
    proto_item_write, NULL,
    proto_item_hash, proto_item_equal,
    proto_item_display
  },
  {
    func_item_len,
    func_item_write, NULL,
    NULL, NULL,
    func_item_display
  },
  {
    method_item_len,
    NULL, NULL,
    NULL, NULL,
    method_item_display
  },
  {
    code_item_len,
    code_item_write, NULL,
    NULL, NULL,
    code_item_display
  },
  {
    konst_item_len,
    konst_item_write, NULL,
    konst_item_hash, konst_item_equal,
    konst_item_display
  }
};

/*-------------------------------------------------------------------------*/

static void init_header(KLCHeader *h, int pkg_size)
{
  strcpy((char *)h->magic, "KLC");
  h->version[0] = '0' + version_major;
  h->version[1] = '0' + version_minor;
  h->version[2] = '0' + ((version_build >> 8) & 0xFF);
  h->version[3] = '0' + (version_build & 0xFF);
  h->file_size   = 0;
  h->header_size = sizeof(KLCHeader);
  h->endian_tag  = ENDIAN_TAG;
  h->map_offset  = sizeof(KLCHeader) + pkg_size;
  h->map_size    = ITYPE_MAX;
  h->pkg_size    = pkg_size;
}

uint32 htable_hash(void *key)
{
  ItemEntry *e = key;
  item_hash_t hash_fn = item_func[e->itype].ihash;
  assert(hash_fn != NULL);
  return hash_fn(e->data);
}

int htable_equal(void *k1, void *k2)
{
  ItemEntry *e1 = k1;
  ItemEntry *e2 = k2;
  if (e1->itype != e2->itype) return 0;
  item_equal_t equal_fn = item_func[e1->itype].iequal;
  assert(equal_fn != NULL);
  return equal_fn(e1->data, e2->data);
}

void KLCFile_Init(KLCFile *filp, char *pkg_name)
{
  int pkg_size = ALIGN_UP(strlen(pkg_name) + 1, 4);
  filp->pkg_name = malloc(pkg_size);
  strcpy(filp->pkg_name, pkg_name);

  init_header(&filp->header, pkg_size);

  hash_table_init(&filp->table, htable_hash, htable_equal);

  for (int i = 0; i < ITYPE_MAX; i++) {
    Vector_Init(&filp->items[i], 0);
  }
}

KLCFile *KLCFile_New(char *pkg_name)
{
  KLCFile *filp = malloc(sizeof(*filp));
  memset(filp, 0, sizeof(*filp));
  KLCFile_Init(filp, pkg_name);
  return filp;
}

void KLCFile_Free(KLCFile *filp)
{
  Vector_Fini(&filp->items[0], NULL, NULL);
  free(filp);
}

void KLCFile_Add_Var(KLCFile *filp, char *name, int flags, char *desc)
{
  int type_index = typeitem_set(filp, desc);
  int name_index = stringitem_set(filp, name);
  VarItem *varitem = VarItem_New(name_index, type_index, flags);
  item_set(filp, ITYPE_VAR, varitem, 0);
}

void KLCFile_Add_Func(KLCFile *filp, char *name, int flags, int nr_locals,
                      uint8 *code, int csz,
                      char *desc[], int rsz,
                      char *pdesc[], int psz)
{
  int name_index = stringitem_set(filp, name);
  int proto_index = protoitem_set(filp, desc, rsz, pdesc, psz);
  int code_off = codeitem_set(filp, code, csz);
  FuncItem *funcitem = FuncItem_New(name_index, proto_index, flags,
                                    rsz, psz, nr_locals, code_off);
  item_set(filp, ITYPE_FUNC, funcitem, 0);
}

/*-------------------------------------------------------------------------*/

void KLCFile_Finish(KLCFile *filp)
{
  int size, length = 0, offset = filp->header.header_size;
  MapItem *mapitem;
  void *item;

  for (int i = 1; i < ITYPE_MAX; i++) {
    size = filp->sizes[i];
    if (size > 0) offset += sizeof(MapItem);
  }

  for (int i = 1; i < ITYPE_MAX; i++) {
    size = filp->sizes[i];
    if (size > 0) {
      offset += length;
      mapitem = MapItem_New(i, offset, size);
      item_set(filp, ITYPE_MAP, mapitem, 0);

      length = 0;
      for (int j = 0; j < size; j++) {
        item = Vector_Get(filp->items + i, j);
        length += item_func[i].ilength(item);
      }
    }
  }

  filp->header.file_size = offset + length + filp->header.pkg_size;
  filp->header.map_size = filp->sizes[0];
}

static void KLCFile_Write_Header(KLCFile *filp)
{
  fwrite(&filp->header, filp->header.header_size, 1, filp->fp);
}

static void KLC_File_Write_Item(KLCFile *filp, int type, Vector *vec, int size)
{
  void *o;
  item_fwrite_t iwrite = item_func[type].iwrite;
  assert(iwrite);
  for (int i = 0; i < size; i++) {
    o = Vector_Get(vec, i);
    iwrite(filp, o);
  }
}

static void KLCFile_Write_PackageName(KLCFile *filp)
{
  fwrite(filp->pkg_name, filp->header.pkg_size, 1, filp->fp);
}

static void KLCFile_Write_Items(KLCFile *filp)
{
  Vector *vec;
  int size;
  for (int i = 0; i < ITYPE_MAX; i++) {
    size = filp->sizes[i];
    if (size > 0) {
      vec = filp->items + i;
      KLC_File_Write_Item(filp, i, vec, size);
    }
  }
}

void KLCFile_Write_File(KLCFile *filp, char *path)
{
  filp->fp = fopen(path, "w");
  KLCFile_Write_Header(filp);
  KLCFile_Write_PackageName(filp);
  KLCFile_Write_Items(filp);
  fclose(filp->fp);
}

KLCFile *KLCFile_Read_File(char *path)
{
  FILE *fp = fopen(path, "r");
  if (fp == NULL) {
    printf("[ERROR] cannot open file:%s\n", path);
    return NULL;
  }

  KLCHeader header;
  int sz = fread(&header, sizeof(KLCHeader), 1, fp);
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

  KLCFile *filp = KLCFile_New(pkg_name);
  assert(filp);
  filp->header = header;

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
    item_set(filp, ITYPE_MAP, mapitem, 0);
  }

  return filp;
}

/*-------------------------------------------------------------------------*/

void header_display(KLCHeader *h)
{
  printf("header:\n");
  printf("magic:%s\n", (char *)h->magic);
  printf("version:%d.%d.%d\n", h->version[0] - '0', h->version[1] - '0', 0);
  printf("header_size:%d\n", h->header_size);
  printf("endian_tag:0x%x\n", h->endian_tag);
  printf("map_size:%d\n", h->map_size);
  printf("--------------------\n");
}

void KCLFile_Display(KLCFile *filp)
{
  void *item;
  KLCHeader *h = &filp->header;
  header_display(h);

  printf("mapitems:\n");
  for (int j = 0; j < filp->sizes[0]; j++) {
    printf("[%d]\n", j);
    item = Vector_Get(filp->items + 0, j);
    item_func[0].idisplay(filp, item);
  }
  printf("--------------------\n");

  for (int i = 1, size = 0; i < ITYPE_MAX; i++) {
    size = filp->sizes[i];
    if (size > 0) {
      printf("%sitems:\n", mapitem_string[i]);
      for (int j = 0; j < size; j++) {
        printf("[%d]\n", j);
        item = Vector_Get(filp->items + i, j);
        item_func[i].idisplay(filp, item);
      }
      printf("--------------------\n");
    }
  }
}

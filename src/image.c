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

#include "image.h"
#include "atom.h"
#include "log.h"

static int version_major = 0; // 1 byte
static int version_minor = 1; // 1 byte
static int version_build = 1; // 2 bytes

#define ENDIAN_TAG  0x1a2b3c4d

typedef struct itementry {
  /* hash node for data is unique */
  HashMapEntry entry;
  /* index of Image->item[type] */
  int type;
  /* index in 'type' vectors */
  int index;
  /* item data */
  void *data;
} ItemEntry;

static unsigned int item_hash(ItemEntry *e);

static int _append_(Image *image, int type, void *data, int unique)
{
  Vector *vec = image->items + type;
  vector_push_back(vec, data);
  int index = vector_size(vec) - 1;
  expect(index >= 0);
  if (unique) {
    ItemEntry *e = kmalloc(sizeof(ItemEntry));
    e->type  = type;
    e->index = index;
    e->data  = data;
    hashmap_entry_init(e, item_hash(e));
    int res = hashmap_add(&image->map, e);
    expect(res == 0);
  }
  return index;
}

static inline int _index_(Image *image, int type, void *data)
{
  ItemEntry key = {.type = type, .data = data};
  hashmap_entry_init(&key, item_hash(&key));
  ItemEntry *res = hashmap_get(&image->map, &key);
  return res ? res->index : -1;
}

static inline void *_get_(Image *image, int type, int index)
{
  expect(type >= 0 && type < image->size);
  return vector_get(image->items + type, index);
}

static inline int _size_(Image *image, int type)
{
  expect(type >= 0 && type < image->size);
  return vector_size(image->items + type);
}

static inline void *vargitem_new(int bsize, int isize, int len)
{
  int32_t *data = kmalloc(bsize + isize * len);
  data[0] = len;
  return data;
}

static char *mapitem_string[] = {
  "map", "string", "literal", "type", "typelist", "const",
  "locvar", "variable", "function", "code",
  "class", "field", "method", "trait", "nfunc", "imeth", "enum", "eval"
};

static int mapitem_length(void *o)
{
  return sizeof(MapItem);
}

static void mapitem_write(FILE *fp, void *o)
{
  fwrite(o, sizeof(MapItem), 1, fp);
}

static void mapitem_show(Image *image, void *o)
{
  MapItem *item = o;
  print("  type:%s\n", mapitem_string[item->type]);
  print("  offset:0x%x\n", item->offset);
  print("  size:%d\n", item->size);
}

static void mapitem_free(void *o)
{
  kfree(o);
}

static MapItem *mapitem_new(int type, int offset, int size)
{
  MapItem *item = kmalloc(sizeof(MapItem));
  item->type   = type;
  item->unused = 0;
  item->offset = offset;
  item->size   = size;
  return item;
}

static int stringitem_length(void *o)
{
  StringItem *item = o;
  return sizeof(StringItem) + item->length * sizeof(char);
}

static void stringitem_write(FILE *fp, void *o)
{
  StringItem *item = o;
  fwrite(o, sizeof(StringItem) + item->length * sizeof(char), 1, fp);
}

static unsigned int stringitem_hash(void *k)
{
  StringItem *item = k;
  return strhash(item->data);
}

static int stringitem_equal(void *k1, void *k2)
{
  StringItem *item1 = k1;
  StringItem *item2 = k2;
  return !strcmp(item1->data, item2->data);
}

static void stringitem_show(Image *image, void *o)
{
  StringItem *item = o;
  print("  length:%d\n", item->length);
  print("  string:%s\n", item->data);
}

void stringitem_free(void *o)
{
  kfree(o);
}

static StringItem *stringitem_new(char *name)
{
  int len = strlen(name);
  StringItem *item = kmalloc(sizeof(StringItem) + len + 1);
  item->length = len + 1;
  memcpy(item->data, name, len);
  item->data[len] = 0;
  return item;
}

static int typeitem_length(void *o)
{
  return sizeof(TypeItem);
}

static void typeitem_write(FILE *fp, void *o)
{
  fwrite(o, sizeof(TypeItem), 1, fp);
}

static unsigned int typeitem_hash(void *k)
{
  return memhash(k, sizeof(TypeItem));
}

static int typeitem_equal(void *k1, void *k2)
{
  TypeItem *item1 = k1;
  TypeItem *item2 = k2;
  if (item1->kind != item2->kind)
    return 0;
  if (item1->parasindex != item2->parasindex)
    return 0;
  if (item1->typesindex != item2->typesindex)
    return 0;
  if (item1->klass.pathindex != item2->klass.pathindex)
    return 0;
  if (item1->klass.typeindex != item2->klass.typeindex)
    return 0;
  return 1;
}

static char *array_string(int dims)
{
  char *data = kmalloc(dims * 2 + 1);
  int i = 0;
  while (dims-- > 0) {
    data[i] = '['; data[i+1] = ']';
    i += 2;
  }
  data[i] = '\0';
  return data;
}

static void typeitem_show(Image *image, void *o)
{
  TypeItem *item = o;
  switch (item->kind) {
  case TYPE_BASE: {
    char *s = base_str(item->base);
    print("  (%s)\n", s);
    break;
  }
  case TYPE_KLASS: {
    StringItem *str;
    if (item->klass.pathindex >= 0) {
      str = _get_(image, ITEM_STRING, item->klass.pathindex);
      print("  pathindex:%d\n", item->klass.pathindex);
      print("  (%s)\n", str->data);
    } else {
      print("  pathindex:%d\n", item->klass.pathindex);
    }
    if (item->klass.typeindex >= 0) {
      str = _get_(image, ITEM_STRING, item->klass.typeindex);
      print("  typeindex:%d\n", item->klass.typeindex);
    } else {
      print("  typeindex:%d\n", item->klass.typeindex);
    }
    break;
  }
  default:
    panic("invalid type %d", item->kind);
    break;
  }
}

static void typeitem_free(void *o)
{
  kfree(o);
}

static TypeItem *typeitem_base_new(int base)
{
  TypeItem *item = kmalloc(sizeof(TypeItem));
  item->kind = TYPE_BASE;
  item->base = base;
  return item;
}

static TypeItem *typeitem_klass_new(int32_t pathindex, int32_t typeindex)
{
  TypeItem *item = kmalloc(sizeof(TypeItem));
  item->kind = TYPE_KLASS;
  item->klass.pathindex = pathindex;
  item->klass.typeindex = typeindex;
  return item;
}

static TypeItem *typeitem_proto_new(int32_t pindex, int32_t rindex)
{
  TypeItem *item = kmalloc(sizeof(TypeItem));
  item->kind = TYPE_PROTO;
  item->proto.pindex = pindex;
  item->proto.rindex = rindex;
  return item;
}

static int indexitem_length(void *o)
{
  IndexItem *item = o;
  return sizeof(IndexItem) + item->size * sizeof(int32_t);
}

static void indexitem_write(FILE *fp, void *o)
{
  IndexItem *item = o;
  fwrite(o, sizeof(IndexItem) + item->size * sizeof(int32_t), 1, fp);
}

static unsigned int indexitem_hash(void *k)
{
  int size = sizeof(IndexItem);
  IndexItem *item = k;
  size += item->size * sizeof(int32_t);
  return memhash(k, size);
}

static int indexitem_equal(void *k1, void *k2)
{
  IndexItem *item1 = k1;
  IndexItem *item2 = k2;
  if (item1->kind != item2->kind)
    return 0;
  if (item1->size != item2->size)
    return 0;
  for (int i = 0; i < item1->size; i++) {
    if (item1->index[i] != item2->index[i])
      return 0;
  }
  return 1;
}

static void indexitem_show(Image *image, void *o)
{
  IndexItem *item = o;
  TypeItem *type;
  for (int i = 0; i < item->size; i++) {
    puts("  ---------");
    print("  index:%d\n", item->index[i]);
    if (item->kind == INDEX_TYPELIST) {
      type = _get_(image, ITEM_TYPE, item->index[i]);
      typeitem_show(image, type);
    }
  }
}

static void indexitem_free(void *o)
{
  kfree(o);
}

static IndexItem *indexitem_new(int kind, int size, int32_t index[])
{
  IndexItem *item = kmalloc(sizeof(IndexItem) + size * sizeof(int32_t));
  item->kind = kind;
  item->size = size;
  for (int i = 0; i < size; i++) {
    item->index[i] = index[i];
  }
  return item;
}

static int literalitem_length(void *o)
{
  return sizeof(LiteralItem);
}

static void literalitem_write(FILE *fp, void *o)
{
  fwrite(o, sizeof(LiteralItem), 1, fp);
}

static unsigned int literalitem_hash(void *k)
{
  return memhash(k, sizeof(LiteralItem));
}

static int literalitem_equal(void *k1, void *k2)
{
  int res = 0;
  LiteralItem *item1 = k1;
  LiteralItem *item2 = k2;
  if (item1->type != item2->type)
    return 0;
  switch (item1->type) {
  case LITERAL_INT:
    res = (item1->ival == item2->ival);
    break;
  case LITERAL_FLOAT:
    res = (item1->fval == item2->fval);
    break;
  case LITERAL_BOOL:
    res = (item1 == item2);
    break;
  case LITERAL_STRING:
    res = (item1->index == item2->index);
    break;
  case LITERAL_UCHAR:
    res = item1->wch.val == item2->wch.val;
    break;
  default:
    panic("invalid literal %d", item1->type);
    break;
  }
  return res;
}

static void literalitem_show(Image *image, void *o)
{
  LiteralItem *item = o;
  switch (item->type) {
  case LITERAL_INT:
    print("  int:%ld\n", item->ival);
    break;
  case LITERAL_FLOAT:
    print("  float:%.16lf\n", item->fval);
    break;
  case LITERAL_BOOL:
    print("  bool:%s\n", item->bval ? "true" : "false");
    break;
  case LITERAL_STRING:
    print("  index:%d\n", item->index);
    StringItem *str = _get_(image, ITEM_STRING, item->index);
    print("  (str:%s)\n", str->data);
    break;
  case LITERAL_UCHAR:
    print("  uchar:%s\n", item->wch.data);
    break;
  default:
    panic("invalid literal %d", item->type);
    break;
  }
}

static void literalitem_free(void *o)
{
  kfree(o);
}

static int constitem_length(void *o)
{
  return sizeof(ConstItem);
}

static void constitem_write(FILE *fp, void *o)
{
  fwrite(o, sizeof(ConstItem), 1, fp);
}

static void constitem_show(Image *image, void *o)
{
  static char *kindstr[] = {
    "", "literal", "type", "anonymous"
  };
  ConstItem *item = o;
  print("  kind:%s\n", kindstr[item->kind]);
  print("  index:%d\n", item->index);
}

static void constitem_free(void *o)
{
  kfree(o);
}

static int locvaritem_length(void *o)
{
  return sizeof(LocVarItem);
}

static void locvaritem_write(FILE *fp, void *o)
{
  fwrite(o, sizeof(LocVarItem), 1, fp);
}

static void locvaritem_show(Image *image, void *o)
{
  LocVarItem *item = o;
  StringItem *str;
  TypeItem *type;

  print("  nameindex:%d\n", item->nameindex);
  str = _get_(image, ITEM_STRING, item->nameindex);
  print("  (%s)\n", str->data);
  print("  typeindex:%d\n", item->typeindex);
  type = _get_(image, ITEM_TYPE, item->typeindex);
  typeitem_show(image, type);
  print("  index:%d\n", item->index);
}

static void locvaritem_free(void *o)
{
  kfree(o);
}

static LocVarItem *locvaritem_new(int32_t nameindex, int32_t typeindex,
                                  int32_t index)
{
  LocVarItem *item = kmalloc(sizeof(LocVarItem));
  item->nameindex = nameindex;
  item->typeindex = typeindex;
  item->index = index;
  return item;
}

static int varitem_length(void *o)
{
  return sizeof(VarItem);
}

static void varitem_write(FILE *fp, void *o)
{
  fwrite(o, sizeof(VarItem), 1, fp);
}

static void varitem_show(Image *image, void *o)
{
  VarItem *item = o;
  StringItem *str;
  TypeItem *type;

  print("  nameindex:%d\n", item->nameindex);
  str = _get_(image, ITEM_STRING, item->nameindex);
  print("  (%s)\n", str->data);
  print("  typeindex:%d\n", item->typeindex);
  type = _get_(image, ITEM_TYPE, item->typeindex);
  typeitem_show(image, type);
}

static void varitem_free(void *o)
{
  kfree(o);
}

static VarItem *varitem_new(int32_t nameindex, int32_t typeindex)
{
  VarItem *item = kmalloc(sizeof(VarItem));
  item->nameindex = nameindex;
  item->typeindex = typeindex;
  return item;
}

static int constvaritem_length(void *o)
{
  return sizeof(ConstVarItem);
}

static void constvaritem_write(FILE *fp, void *o)
{
  fwrite(o, sizeof(ConstVarItem), 1, fp);
}

static void constvaritem_show(Image *image, void *o)
{
  ConstVarItem *item = o;
  StringItem *str;
  TypeItem *type;

  print("  nameindex:%d\n", item->nameindex);
  str = _get_(image, ITEM_STRING, item->nameindex);
  print("  (%s)\n", str->data);
  print("  typeindex:%d\n", item->typeindex);
  type = _get_(image, ITEM_TYPE, item->typeindex);
  typeitem_show(image, type);
}

static void constvaritem_free(void *o)
{
  kfree(o);
}

static ConstVarItem *constvaritem_new(int32_t nameindex, int32_t typeindex,
                                      int32_t index)
{
  ConstVarItem *item = kmalloc(sizeof(ConstVarItem));
  item->nameindex = nameindex;
  item->typeindex = typeindex;
  item->index = index;
  return item;
}

static int funcitem_length(void *o)
{
  return sizeof(FuncItem);
}

static void funcitem_write(FILE *fp, void *o)
{
  fwrite(o, sizeof(FuncItem), 1, fp);
}

static void funcitem_show(Image *image, void *o)
{
  FuncItem *item = o;
  StringItem *str;
  print("  nameindex:%d\n", item->nameindex);
  str = _get_(image, ITEM_STRING, item->nameindex);
  print("  (%s)\n", str->data);
  print("  pindex:%d\n", item->pindex);
  print("  rindex:%d\n", item->rindex);
  print("  codeindex:%d\n", item->codeindex);
}

static void funcitem_free(void *o)
{
  kfree(o);
}

static int anonyitem_length(void *o)
{
  return sizeof(AnonyItem);
}

static void anonyitem_write(FILE *fp, void *o)
{
  fwrite(o, sizeof(AnonyItem), 1, fp);
}

static void anonyitem_show(Image *image, void *o)
{
  AnonyItem *item = o;
  StringItem *str;
  print("  nameindex:%d\n", item->nameindex);
  str = _get_(image, ITEM_STRING, item->nameindex);
  print("  (%s)\n", str->data);
  print("  pindex:%d\n", item->pindex);
  print("  rindex:%d\n", item->rindex);
  print("  codeindex:%d\n", item->codeindex);
}

static void anonyitem_free(void *o)
{
  kfree(o);
}

static AnonyItem *anonyitem_new(int nameindex, int pindex, int rindex,
                                int codeindex)
{
  AnonyItem *item = kmalloc(sizeof(AnonyItem));
  item->pindex = pindex;
  item->rindex = rindex;
  item->codeindex = codeindex;
  return item;
}

static int codeitem_length(void *o)
{
  CodeItem *item = o;
  return sizeof(CodeItem) + sizeof(uint8_t) * item->size;
}

static void codeitem_write(FILE *fp, void *o)
{
  CodeItem *item = o;
  fwrite(o, sizeof(CodeItem) + sizeof(uint8_t) * item->size, 1, fp);
}

static void codeitem_show(Image *image, void *o)
{
}

static void codeitem_free(void *o)
{
  kfree(o);
}

static CodeItem *codeitem_new(uint8_t *codes, int size)
{
  int sz = sizeof(CodeItem) + sizeof(uint8_t) * size;
  CodeItem *item = kmalloc(sz);
  item->size = size;
  memcpy(item->codes, codes, size);
  kfree(codes);
  return item;
}

static int classitem_length(void *o)
{
  return sizeof(ClassItem);
}

static void classitem_write(FILE *fp, void *o)
{
  fwrite(o, sizeof(ClassItem), 1, fp);
}

static void classitem_show(Image *image, void *o)
{
  ClassItem *item = o;
  StringItem *str;
  print("  nameindex:%d\n", item->nameindex);
  str = _get_(image, ITEM_STRING, item->nameindex);
  print("  (%s)\n", str->data);
  if (item->basesindex >= 0) {
    print("  basesinfo:\n");
    TypeItem *type = _get_(image, ITEM_TYPE, item->basesindex);
    typeitem_show(image, type);
  }
}

static void classitem_free(void *o)
{
  kfree(o);
}

static ClassItem *classitem_new(int nameindex, int basesindex)
{
  ClassItem *item = kmalloc(sizeof(ClassItem));
  item->nameindex = nameindex;
  item->basesindex = basesindex;
  return item;
}

static int ifuncitem_length(void *o)
{
  return sizeof(IFuncItem);
}

static void ifuncitem_write(FILE *fp, void *o)
{
  fwrite(o, sizeof(IFuncItem), 1, fp);
}

static void ifuncitem_show(Image *image, void *o)
{
  IFuncItem *item = o;
  StringItem *str;
  print("  nameindex:%d\n", item->nameindex);
  str = _get_(image, ITEM_STRING, item->nameindex);
  print("  (%s)\n", str->data);
  print("  pindex:%d\n", item->pindex);
  print("  rindex:%d\n", item->rindex);
}

static void ifuncitem_free(void *o)
{
  kfree(o);
}

static IFuncItem *ifuncitem_new(int nameindex, int pindex, int rindex)
{
  IFuncItem *item = kmalloc(sizeof(IFuncItem));
  item->nameindex = nameindex;
  item->pindex = pindex;
  item->rindex = rindex;
  return item;
}

static int enumitem_length(void *o)
{
  return sizeof(EnumItem);
}

static void enumitem_write(FILE *fp, void *o)
{
  fwrite(o, sizeof(EnumItem), 1, fp);
}

static void enumitem_show(Image *image, void *o)
{
  EnumItem *item = o;
  StringItem *str;
  print("  nameindex:%d\n", item->nameindex);
  str = _get_(image, ITEM_STRING, item->nameindex);
  print("  (%s)\n", str->data);
}

static void enumitem_free(void *o)
{
  kfree(o);
}

static EnumItem *enumitem_new(int nameindex)
{
  EnumItem *item = kmalloc(sizeof(EnumItem));
  item->nameindex = nameindex;
  return item;
}

static int evalitem_length(void *o)
{
  return sizeof(EValItem);
}

static void evalitem_write(FILE *fp, void *o)
{
  fwrite(o, sizeof(EValItem), 1, fp);
}

static void evalitem_show(Image *image, void *o)
{
  EValItem *item = o;
  StringItem *str;
  print("  nameindex:%d\n", item->nameindex);
  str = _get_(image, ITEM_STRING, item->nameindex);
  print("  (%s)\n", str->data);
  print("  index:%d\n", item->index);
  print("  value:%d\n", item->value);
}

static void evalitem_free(void *o)
{
  kfree(o);
}

static EValItem *evalitem_new(int nameindex, int index, int32_t val)
{
  EValItem *item = kmalloc(sizeof(EValItem));
  item->nameindex = nameindex;
  item->index = index;
  item->value = val;
  return item;
}

static int mbritem_length(void *o)
{
  MbrItem *item = o;
  return sizeof(MbrItem) + item->size * sizeof(MbrIndex);
}

static void mbritem_write(FILE *fp, void *o)
{
  MbrItem *item = o;
  fwrite(o, sizeof(MbrItem) + item->size * sizeof(MbrIndex), 1, fp);
}

static void mbritem_show(Image *image, void *o)
{
  MbrItem *item = o;
  MbrIndex *index;
  for (int i = 0; i < item->size; i++) {
    puts("  ---------");
    index = item->indexes + i;
    print("  kind:%d", index->kind);
    print("  index:%d", index->index);
  }
}

static void mbritem_free(void *o)
{
  kfree(o);
}

static inline void *item_copy(int size, void *src)
{
  void *dest = kmalloc(size);
  memcpy(dest, src, size);
  return dest;
}

static int stringitem_get(Image *image, char *str)
{
  int len = strlen(str);
  uint8_t data[sizeof(StringItem) + len + 1];
  StringItem *item = (StringItem *)data;
  item->length = len + 1;
  memcpy(item->data, str, len);
  item->data[len] = 0;
  return _index_(image, ITEM_STRING, item);
}

static int stringitem_set(Image *image, char *str)
{
  int index = stringitem_get(image, str);

  if (index < 0) {
    StringItem *item = stringitem_new(str);
    index = _append_(image, ITEM_STRING, item, 1);
  }

  return index;
}

static int typeitem_get(Image *image, TypeDesc *desc);
static int typeitem_set(Image *image, TypeDesc *desc);

static int indexitem_get(Image *image, int kind, Vector *vec)
{
  int sz = vector_size(vec);
  if (sz <= 0)
    return -1;

  uint8_t data[sizeof(IndexItem) + sizeof(int32_t) * sz];
  IndexItem *item = (IndexItem *)data;
  item->kind = kind;
  item->size = sz;

  int index;
  void *ptr;
  vector_for_each(ptr, vec) {
    if (kind == INDEX_TYPELIST) {
      index = typeitem_get(image, ptr);
    } else {
      expect(kind == INDEX_VALUE);
      index = (intptr_t)ptr;
    }
    if (index < 0)
      return -1;
    item->index[idx] = index;
  }

  return _index_(image, ITEM_INDEX, item);
}

static int indexitem_set(Image *image, int kind, Vector *vec)
{
  int sz = vector_size(vec);
  if (sz <= 0)
    return -1;

  int index = indexitem_get(image, kind, vec);
  if (index < 0) {
    int32_t indexes[sz];
    void *ptr;
    vector_for_each(ptr, vec) {
      if (kind == INDEX_TYPELIST) {
        index = typeitem_set(image, ptr);
      } else {
        expect(kind == INDEX_VALUE);
        index = (intptr_t)ptr;
      }
      expect(index >= 0);
      indexes[idx] = index;
    }
    IndexItem *item = indexitem_new(kind, sz, indexes);
    index = _append_(image, ITEM_INDEX, item, 1);
  }
  return index;
}

static int typeitem_get(Image *image, TypeDesc *desc)
{
  TypeItem item = {0};
  switch (desc->kind) {
  case TYPE_BASE: {
    item.kind = desc->kind;
    item.base = desc->base;
    break;
  }
  case TYPE_KLASS: {
    int pathindex = -1;
    if (desc->klass.path) {
      pathindex = stringitem_get(image, desc->klass.path);
      if (pathindex < 0)
        return pathindex;
    }
    int typeindex = -1;
    if (desc->klass.type) {
      typeindex = stringitem_get(image, desc->klass.type);
      if (typeindex < 0)
        return typeindex;
    }
    int parasindex = -1;
    if (desc->paras != NULL) {
      parasindex = indexitem_get(image, INDEX_TYPELIST, desc->paras);
    }
    int typesindex = -1;
    if (desc->types != NULL) {
      typesindex = indexitem_get(image, INDEX_TYPELIST, desc->types);
    }
    item.kind = TYPE_KLASS;
    item.klass.pathindex = pathindex;
    item.klass.typeindex = typeindex;
    item.parasindex = parasindex;
    item.typesindex = typesindex;
    break;
  }
  case TYPE_PROTO: {
    int rindex = typeitem_get(image, desc->proto.ret);
    int pindex = indexitem_get(image, INDEX_TYPELIST, desc->proto.args);
    item.kind = TYPE_PROTO;
    item.proto.pindex = pindex;
    item.proto.rindex = rindex;
    break;
  }
  default: {
    panic("invalid typedesc %d", desc->kind);
    break;
  }
  }
  return _index_(image, ITEM_TYPE, &item);
}

static int typeitem_set(Image *image, TypeDesc *desc)
{
  if (desc == NULL)
    return -1;

  TypeItem *item = NULL;
  int index = typeitem_get(image, desc);
  if (index < 0) {
    switch (desc->kind) {
    case TYPE_BASE: {
      item = typeitem_base_new(desc->base);
      break;
    }
    case TYPE_KLASS: {
      int pathindex = -1;
      if (desc->klass.path != NULL) {
        pathindex = stringitem_set(image, desc->klass.path);
      }
      int typeindex = -1;
      if (desc->klass.type != NULL) {
        typeindex = stringitem_set(image, desc->klass.type);
      }
      int parasindex = -1;
      if (desc->paras != NULL) {
        parasindex = indexitem_set(image, INDEX_TYPELIST, desc->paras);
      }
      int typesindex = -1;
      if (desc->types != NULL) {
        typesindex = indexitem_set(image, INDEX_TYPELIST, desc->types);
      }
      item = typeitem_klass_new(pathindex, typeindex);
      item->parasindex = parasindex;
      item->typesindex = typesindex;
      break;
    }
    case TYPE_PROTO: {
      int rindex = typeitem_set(image, desc->proto.ret);
      int pindex = indexitem_set(image, INDEX_TYPELIST, desc->proto.args);
      item = typeitem_proto_new(pindex, rindex);
      break;
    }
    default: {
      panic("invalid typedesc %d", desc->kind);
      break;
    }
    }
    index = _append_(image, ITEM_TYPE, item, 1);
  }
  return index;
}

static inline int literalitem_get(Image *image, LiteralItem *item)
{
  return _index_(image, ITEM_LITERAL, item);
}

static inline int codeitem_set(Image *image, uint8_t *codes, int size)
{
  CodeItem *item = codeitem_new(codes, size);
  return _append_(image, ITEM_CODE, item, 0);
}

typedef int (*itemlengthfunc)(void *);
typedef void (*itemwritefunc)(FILE *, void *);
typedef void (*itemshowfunc)(Image *, void *);
typedef void (*itemfreefunc)(void *);

struct item_funcs {
  itemlengthfunc length;
  itemwritefunc write;
  hashfunc hash;
  equalfunc equal;
  itemshowfunc show;
  itemfreefunc free;
} item_func[ITEM_MAX] = {
  {
    mapitem_length, mapitem_write,
    NULL, NULL,
    mapitem_show, mapitem_free,
  },
  {
    stringitem_length, stringitem_write,
    stringitem_hash, stringitem_equal,
    stringitem_show, stringitem_free,
  },
  {
    literalitem_length, literalitem_write,
    literalitem_hash, literalitem_equal,
    literalitem_show, literalitem_free,
  },
  {
    typeitem_length, typeitem_write,
    typeitem_hash, typeitem_equal,
    typeitem_show, typeitem_free,
  },
  {
    indexitem_length, indexitem_write,
    indexitem_hash, indexitem_equal,
    indexitem_show, indexitem_free,
  },
  {
    constitem_length, constitem_write,
    NULL, NULL,
    constitem_show, constitem_free,
  },
  {
    locvaritem_length, locvaritem_write,
    NULL, NULL,
    locvaritem_show, locvaritem_free,
  },
  {
    varitem_length, varitem_write,
    NULL, NULL,
    varitem_show, varitem_free,
  },
  {
    constvaritem_length, constvaritem_write,
    NULL, NULL,
    constvaritem_show, constvaritem_free,
  },
  {
    funcitem_length, funcitem_write,
    NULL, NULL,
    funcitem_show, funcitem_free,
  },
  {
    anonyitem_length, anonyitem_write,
    NULL, NULL,
    anonyitem_show, anonyitem_free,
  },
  {
    codeitem_length, codeitem_write,
    NULL, NULL,
    codeitem_show, codeitem_free,
  },
  {
    classitem_length, classitem_write,
    NULL, NULL,
    classitem_show, classitem_free,
  },
  {
    varitem_length, varitem_write,
    NULL, NULL,
    varitem_show, varitem_free,
  },
  {
    funcitem_length, funcitem_write,
    NULL, NULL,
    funcitem_show, funcitem_free,
  },
  {
    classitem_length, classitem_write,
    NULL, NULL,
    classitem_show, classitem_free,
  },
  {
    ifuncitem_length, ifuncitem_write,
    NULL, NULL,
    ifuncitem_show, ifuncitem_free,
  },
  {
    enumitem_length, enumitem_write,
    NULL, NULL,
    enumitem_show, enumitem_free,
  },
  {
    evalitem_length, evalitem_write,
    NULL, NULL,
    evalitem_show, evalitem_free,
  },
  {
    mbritem_length, mbritem_write,
    NULL, NULL,
    mbritem_show, mbritem_free,
  },
};

static inline LiteralItem *literalitem_int_new(int64_t val)
{
  LiteralItem *item = kmalloc(sizeof(LiteralItem));
  item->type = LITERAL_INT;
  item->ival = val;
  return item;
}

static inline LiteralItem *literalitem_float_new(double val)
{
  LiteralItem *item = kmalloc(sizeof(LiteralItem));
  item->type = LITERAL_FLOAT;
  item->fval = val;
  return item;
}

static inline LiteralItem *literalitem_bool_new(int val)
{
  LiteralItem *item = kmalloc(sizeof(LiteralItem));
  item->type = LITERAL_BOOL;
  item->bval = val;
  return item;
}

static inline LiteralItem *literalitem_string_new(int32_t val)
{
  LiteralItem *item = kmalloc(sizeof(LiteralItem));
  item->type = LITERAL_STRING;
  item->index = val;
  return item;
}

static inline LiteralItem *literalitem_uchar_new(wchar val)
{
  LiteralItem *item = kmalloc(sizeof(LiteralItem));
  item->type = LITERAL_UCHAR;
  item->wch = val;
  return item;
}

static int image_add_const(Image *image, int kind, int index)
{
  ConstItem *item = kmalloc(sizeof(ConstItem));
  item->kind = kind;
  item->index = index;
  return _append_(image, ITEM_CONST, item, 0);
}

int image_add_integer(Image *image, int64_t val)
{
  LiteralItem k = {0};
  k.type = LITERAL_INT;
  k.ival = val;
  int index = literalitem_get(image, &k);
  if (index < 0) {
    LiteralItem *item = literalitem_int_new(val);
    index = _append_(image, ITEM_LITERAL, item, 1);
  }
  return image_add_const(image, CONST_LITERAL, index);
}

int image_add_float(Image *image, double val)
{
  LiteralItem k = {0};
  k.type = LITERAL_FLOAT;
  k.fval = val;
  int index = literalitem_get(image, &k);
  if (index < 0) {
    LiteralItem *item = literalitem_float_new(val);
    index = _append_(image, ITEM_LITERAL, item, 1);
  }
  return image_add_const(image, CONST_LITERAL, index);
}

int image_add_bool(Image *image, int val)
{
  LiteralItem k = {0};
  k.type = LITERAL_BOOL;
  k.bval = val;
  int index = literalitem_get(image, &k);
  if (index < 0) {
    LiteralItem *item = literalitem_bool_new(val);
    index = _append_(image, ITEM_LITERAL, item, 1);
  }
  return image_add_const(image, CONST_LITERAL, index);
}

int image_add_string(Image *image, char *val)
{
  int32_t idx = stringitem_set(image, val);
  LiteralItem k = {0};
  k.type = LITERAL_STRING;
  k.index = idx;
  int index = literalitem_get(image, &k);
  if (index < 0) {
    LiteralItem *item = literalitem_string_new(idx);
    index = _append_(image, ITEM_LITERAL, item, 1);
  }
  return image_add_const(image, CONST_LITERAL, index);
}

int image_add_uchar(Image *image, wchar val)
{
  LiteralItem k = {0};
  k.type = LITERAL_UCHAR;
  k.wch = val;
  int index = literalitem_get(image, &k);
  if (index < 0) {
    LiteralItem *item = literalitem_uchar_new(val);
    index = _append_(image, ITEM_LITERAL, item, 1);
  }
  return index;
}

int image_add_literal(Image *image, Literal *val)
{
  int index;
  if (val->kind == BASE_INT) {
    index = image_add_integer(image, val->ival);
  } else if (val->kind == BASE_STR) {
    index = image_add_string(image, val->str);
  } else if (val->kind == BASE_BOOL) {
    index = image_add_bool(image, val->bval);
  } else if (val->kind == BASE_FLOAT) {
    index = image_add_float(image, val->fval);
  } else if (val->kind == BASE_CHAR) {
    index = image_add_uchar(image, val->cval);
  } else {
    index = -1;
  }
  return index;
}

int image_add_desc(Image *image, TypeDesc *desc)
{
  expect(desc != NULL);
  int index = typeitem_set(image, desc);
  return image_add_const(image, CONST_TYPE, index);
}

static int add_locvar(Image *image, LocVar *var)
{
  int nameindex = stringitem_set(image, var->name);
  int typeindex = typeitem_set(image, var->desc);
  LocVarItem *item = locvaritem_new(nameindex, typeindex, var->index);
  return _append_(image, ITEM_LOCVAR, item, 0);
}

static int add_locvars(Image *image, Vector *locvec)
{
  Vector *indexvec = NULL;
  if (vector_size(locvec) > 0)
    indexvec = vector_new();

  int index;
  LocVar *var;
  vector_for_each(var, locvec) {
    index = add_locvar(image, var);
    vector_append_int(indexvec, index);
  }

  index = indexitem_set(image, INDEX_VALUE, indexvec);
  vector_free(indexvec);
  return index;
}

LocVar *locvar_new(char *name, TypeDesc *desc, int index)
{
  LocVar *loc = kmalloc(sizeof(LocVar));
  loc->name = name;
  loc->desc = TYPE_INCREF(desc);
  loc->index = index;
  return loc;
}

void locvar_free(LocVar *loc)
{
  TYPE_DECREF(loc->desc);
  kfree(loc);
}

static TypeDesc *to_typedesc(TypeItem *item, Image *image);

static LocVar *load_locvar(Image *image, int index)
{
  LocVarItem *item = _get_(image, ITEM_LOCVAR, index);
  StringItem *str = _get_(image, ITEM_STRING, item->nameindex);;
  TypeItem *type = _get_(image, ITEM_TYPE, item->typeindex);
  TypeDesc *desc = to_typedesc(type, image);
  LocVar *var = locvar_new(str->data, desc, item->index);
  TYPE_DECREF(desc);
  return var;
}

static Vector *load_locvars(Image *image, IndexItem *indexes)
{
  if (indexes == NULL)
    return NULL;

  Vector *vec = vector_new();
  LocVar *var;
  for (int i = 0; i < indexes->size; ++i) {
    var = load_locvar(image, indexes->index[i]);
    vector_push_back(vec, var);
  }
  return vec;
}

int image_add_anony(Image *image, CodeInfo *ci)
{
  debug("image_add_anony: %s-%d-%d-%d",
        ci->name, vector_size(ci->locvec),
        vector_size(ci->freevec), vector_size(ci->upvec));
  int nameindex = stringitem_set(image, ci->name);
  int pindex = indexitem_set(image, INDEX_TYPELIST, ci->desc->proto.args);
  int rindex = typeitem_set(image, ci->desc->proto.ret);
  int codeindex = codeitem_set(image, ci->codes, ci->size);
  AnonyItem *anony = anonyitem_new(nameindex, pindex, rindex, codeindex);
  anony->locindex = add_locvars(image, ci->locvec);
  anony->freeindex = indexitem_set(image, INDEX_VALUE, ci->freevec);
  anony->upindex = indexitem_set(image, INDEX_VALUE, ci->upvec);
  int index = _append_(image, ITEM_ANONY, anony, 0);
  return image_add_const(image, CONST_ANONY, index);
}

void image_add_var(Image *image, char *name, TypeDesc *desc)
{
  int type_index = typeitem_set(image, desc);
  int name_index = stringitem_set(image, name);
  VarItem *varitem = varitem_new(name_index, type_index);
  _append_(image, ITEM_VAR, varitem, 0);
}

void image_add_kvar(Image *image, char *name, TypeDesc *desc, Literal *val)
{
  int type_index = typeitem_set(image, desc);
  int name_index = stringitem_set(image, name);
  int index = image_add_literal(image, val);
  ConstVarItem *item = constvaritem_new(name_index, type_index, index);
  _append_(image, ITEM_CONSTVAR, item, 0);
}

static FuncItem *funcitem_new(Image *image, CodeInfo *ci)
{
  int nameindex = stringitem_set(image, ci->name);
  int pindex = indexitem_set(image, INDEX_TYPELIST, ci->desc->proto.args);
  int rindex = typeitem_set(image, ci->desc->proto.ret);
  int codeindex = codeitem_set(image, ci->codes, ci->size);
  FuncItem *item = kmalloc(sizeof(FuncItem));
  item->nameindex = nameindex;
  item->pindex = pindex;
  item->rindex = rindex;
  item->codeindex = codeindex;
  item->locindex = add_locvars(image, ci->locvec);
  item->freeindex = indexitem_set(image, INDEX_VALUE, ci->freevec);
  return item;
}

int image_add_func(Image *image, CodeInfo *ci)
{
  FuncItem *funcitem = funcitem_new(image, ci);
  return _append_(image, ITEM_FUNC, funcitem, 0);
}

static inline ClassItem *klass_new(Image *image, char *name, Vector *bases)
{
  int nameindex = stringitem_set(image, name);
  int basesindex = indexitem_set(image, INDEX_TYPELIST, bases);
  return classitem_new(nameindex, basesindex);
}

void image_add_class(Image *image, char *name, Vector *bases, int mbrindex)
{
  ClassItem *classitem = klass_new(image, name, bases);
  classitem->mbrindex = mbrindex;
  _append_(image, ITEM_CLASS, classitem, 0);
}

int image_add_field(Image *image, char *name, TypeDesc *desc)
{
  int name_index = stringitem_set(image, name);
  int type_index = typeitem_set(image, desc);
  VarItem *varitem = varitem_new(name_index, type_index);
  return _append_(image, ITEM_FIELD, varitem, 0);
}

int image_add_method(Image *image, CodeInfo *ci)
{
  FuncItem *funcitem = funcitem_new(image, ci);
  return _append_(image, ITEM_METHOD, funcitem, 0);
}

void image_add_trait(Image *image, char *name, Vector *bases, int mbrindex)
{
  ClassItem *classitem = klass_new(image, name, bases);
  classitem->mbrindex = mbrindex;
  _append_(image, ITEM_TRAIT, classitem, 0);
}

int image_add_ifunc(Image *image, char *name, TypeDesc *desc)
{
  int nameindex = stringitem_set(image, name);
  int pindex = indexitem_set(image, INDEX_TYPELIST, desc->proto.args);
  int rindex = typeitem_set(image, desc->proto.ret);
  IFuncItem *ifuncitem = ifuncitem_new(nameindex, pindex, rindex);
  return _append_(image, ITEM_IFUNC, ifuncitem, 0);
}

void image_add_enum(Image *image, char *name, int mbrindex)
{
  int nameindex = stringitem_set(image, name);
  EnumItem *enumitem = enumitem_new(nameindex);
  enumitem->mbrindex = mbrindex;
  _append_(image, ITEM_ENUM, enumitem, 0);
}

int image_add_eval(Image *image, char *name, Vector *types, int32_t val)
{
  int nameindex = stringitem_set(image, name);
  int index = indexitem_set(image, INDEX_TYPELIST, types);
  EValItem *evitem = evalitem_new(nameindex, index, val);
  return _append_(image, ITEM_EVAL, evitem, 0);
}

int image_add_mbrs(Image *image, MbrIndex *indexes, int size)
{
  int isize = size * sizeof(MbrIndex);
  MbrItem *item = kmalloc(sizeof(MbrItem) + isize);
  item->size = size;
  memcpy(item->indexes, indexes, isize);
  return _append_(image, ITEM_MBR, item, 0);
}

static unsigned int item_hash(ItemEntry *e)
{
  if (e->type < 0 || e->type >= ITEM_MAX)
    panic("type '%d' out of range", e->type);
  hashfunc fn = item_func[e->type].hash;
  expect(fn != NULL);
  return fn(e->data);
}

static int _item_equal_(void *k1, void *k2)
{
  ItemEntry *e1 = k1;
  ItemEntry *e2 = k2;
  expect(e1->type >= 0 && e1->type < ITEM_MAX);
  expect(e2->type >= 0 && e2->type < ITEM_MAX);
  if (e1->type != e2->type)
    return 0;
  equalfunc fn = item_func[e1->type].equal;
  expect(fn != NULL);
  return fn(e1->data, e2->data);
}

static void _itementry_free_(void *entry, void *arg)
{
  kfree(entry);
}

static void init_header(ImageHeader *h, char *name)
{
  strcpy((char *)h->magic, "KLC");
  h->version[0] = '0' + version_major;
  h->version[1] = '0' + version_minor;
  h->version[2] = '0' + ((version_build >> 8) & 0xFF);
  h->version[3] = '0' + (version_build & 0xFF);
  h->file_size   = 0;
  h->header_size = sizeof(ImageHeader);
  h->endian_tag  = ENDIAN_TAG;
  h->map_offset  = sizeof(ImageHeader);
  h->map_size    = ITEM_MAX;
  strncpy(h->name, name, PKG_NAME_MAX-1);
}

Image *image_new(char *name)
{
  int sz = sizeof(Image) + ITEM_MAX * sizeof(Vector);
  Image *image = kmalloc(sz);
  init_header(&image->header, name);
  hashmap_init(&image->map, _item_equal_);
  image->size = ITEM_MAX;
  return image;
}

void image_free(Image *image)
{
  hashmap_fini(&image->map, _itementry_free_, NULL);

  void *data;
  for (int i = 0; i < image->size; i++) {
    vector_for_each(data, image->items + i) {
      expect(i >= 0 && i < ITEM_MAX);
      itemfreefunc fn = item_func[i].free;
      expect(fn != NULL);
      fn(data);
    }
    vector_fini(image->items + i);
  }
  kfree(image);
}

static Vector *to_typedescvec(IndexItem *item, Image *image)
{
  if (item == NULL)
    return NULL;

  Vector *v = vector_new();
  TypeItem *typeitem;
  TypeDesc *t;
  for (int i = 0; i < item->size; i++) {
    typeitem = _get_(image, ITEM_TYPE, item->index[i]);
    t = to_typedesc(typeitem, image);
    vector_push_back(v, t);
  }
  return v;
}

static TypeDesc *to_typedesc(TypeItem *item, Image *image)
{
  TypeDesc *t = NULL;
  switch (item->kind) {
  case TYPE_BASE: {
    t = desc_from_base(item->base);
    break;
  }
  case TYPE_KLASS: {
    StringItem *s;
    char *path;
    char *type;
    if (item->klass.pathindex >= 0) {
      s = _get_(image, ITEM_STRING, item->klass.pathindex);
      path = atom(s->data);
    } else {
      path = NULL;
    }
    s = _get_(image, ITEM_STRING, item->klass.typeindex);
    type = atom(s->data);
    t = desc_from_klass(path, type);
    if (item->parasindex >= 0) {
      IndexItem *listitem = _get_(image, ITEM_INDEX, item->parasindex);
      t->paras = to_typedescvec(listitem, image);
    }
    if (item->typesindex >= 0) {
      IndexItem *listitem = _get_(image, ITEM_INDEX, item->typesindex);
      t->types = to_typedescvec(listitem, image);
    }
    break;
  }
  case TYPE_PROTO: {
    IndexItem *listitem = _get_(image, ITEM_INDEX, item->proto.pindex);
    TypeItem *item = _get_(image, ITEM_TYPE, item->proto.rindex);
    Vector *args = to_typedescvec(listitem, image);
    TypeDesc *ret = to_typedesc(item, image);
    t = desc_from_proto(args, ret);
    TYPE_DECREF(ret);
    break;
  }
  default:
    panic("invalid type %d", item->kind);
    break;
  }
  return t;
}

static Literal to_literal(LiteralItem *item, Image *image)
{
  Literal value;
  StringItem *s;
  switch (item->type) {
  case LITERAL_INT:
    value.kind = BASE_INT;
    value.ival = item->ival;
    break;
  case LITERAL_STRING:
    s = _get_(image, ITEM_STRING, item->index);
    value.kind = BASE_STR;
    value.str = s->data;
    break;
  case LITERAL_FLOAT:
    value.kind = BASE_FLOAT;
    value.fval = item->fval;
    break;
  case LITERAL_BOOL:
    value.kind = BASE_BOOL;
    value.bval = item->bval;
    break;
  case LITERAL_UCHAR:
    value.kind = BASE_CHAR;
    value.cval = item->wch;
    break;
  default:
    panic("invalid literal %d", item->type);
    break;
  }
  return value;
}

int image_const_size(Image *image)
{
  return _size_(image, ITEM_CONST);
}

static Vector *getindexvec(IndexItem *index)
{
  Vector *vec = NULL;
  if (index != NULL) {
    vec = vector_new();
    for (int i = 0; i < index->size; ++i) {
      vector_append_int(vec, index->index[i]);
    }
  }
  return vec;
}

static void image_load_anony(Image *image, ConstItem *item, CodeInfo *ci)
{
  AnonyItem *anony = _get_(image, ITEM_ANONY, item->index);
  StringItem *str = _get_(image, ITEM_STRING, anony->nameindex);
  CodeItem *code = _get_(image, ITEM_CODE, anony->codeindex);
  IndexItem *listitem = _get_(image, ITEM_INDEX, anony->pindex);
  TypeItem *typeitem = _get_(image, ITEM_TYPE, anony->rindex);
  Vector *args = to_typedescvec(listitem, image);
  TypeDesc *ret = to_typedesc(typeitem, image);
  TypeDesc *desc = desc_from_proto(args, ret);
  TYPE_DECREF(ret);

  IndexItem *locindex = _get_(image, ITEM_INDEX, anony->locindex);
  Vector *locvec = load_locvars(image, locindex);
  IndexItem *freeindex = _get_(image, ITEM_INDEX, anony->freeindex);
  Vector *freevec = getindexvec(freeindex);
  IndexItem *upindex = _get_(image, ITEM_INDEX, anony->upindex);
  Vector *upvec = getindexvec(upindex);
  ci->name = str->data;
  ci->desc = TYPE_INCREF(desc);
  ci->codes = code->codes;
  ci->size = code->size;
  ci->locvec = locvec;
  ci->freevec = freevec;
  ci->upvec = upvec;
  TYPE_DECREF(desc);
  debug("image_load_anony: %s-%d-%d-%d",
        ci->name, vector_size(ci->locvec),
        vector_size(ci->freevec), vector_size(ci->upvec));
}

void image_load_consts(Image *image, getconstfunc func, void *arg)
{
  void *data;
  ConstItem *item;
  LiteralItem *liteitem;
  TypeItem *typeitem;
  Literal val;
  TypeDesc *desc;
  Vector *vec;
  int size = _size_(image, ITEM_CONST);
  for (int i = 0; i < size; i++) {
    item = _get_(image, ITEM_CONST, i);
    if (item->kind == CONST_LITERAL) {
      liteitem = _get_(image, ITEM_LITERAL, item->index);
      val = to_literal(liteitem, image);
      func(&val, CONST_LITERAL, i, arg);
    } else if (item->kind == CONST_TYPE) {
      typeitem = _get_(image, ITEM_TYPE, item->index);
      desc = to_typedesc(typeitem, image);
      func(desc, CONST_TYPE, i, arg);
      TYPE_DECREF(desc);
    } else {
      expect(item->kind == CONST_ANONY);
      CodeInfo ci;
      image_load_anony(image, item, &ci);
      func(&ci, CONST_ANONY, i, arg);
      TYPE_DECREF(ci.desc);
      vector_free(ci.locvec);
      vector_free(ci.freevec);
      vector_free(ci.upvec);
    }
  }
}

void image_load_class(Image *image, int index, getclassfunc func, void *arg)
{
  ClassItem *item = _get_(image, ITEM_CLASS, index);
  StringItem *str = _get_(image, ITEM_STRING, item->nameindex);
  func(str->data, item->mbrindex, image, arg);
}

void image_load_trait(Image *image, int index, getclassfunc func, void *arg)
{
  ClassItem *item = _get_(image, ITEM_TRAIT, index);
  StringItem *str = _get_(image, ITEM_STRING, item->nameindex);
  func(str->data, item->mbrindex, image, arg);
}

void image_load_enum(Image *image, int index, getclassfunc func, void *arg)
{
  EnumItem *item = _get_(image, ITEM_ENUM, index);
  StringItem *str = _get_(image, ITEM_STRING, item->nameindex);
  func(str->data, item->mbrindex, image, arg);
}

void image_load_field(Image *image, int index, getmbrfunc func, void *arg)
{
  VarItem *item = _get_(image, ITEM_FIELD, index);
  StringItem *str = _get_(image, ITEM_STRING, item->nameindex);
  TypeItem *type = _get_(image, ITEM_TYPE, item->typeindex);
  TypeDesc *desc = to_typedesc(type, image);
  func(str->data, MBR_FIELD, desc, arg);
  TYPE_DECREF(desc);
}

void image_load_method(Image *image, int index, getmbrfunc func, void *arg)
{

}

void image_load_mbrs(Image *image, int index, getmbrfunc func, void *arg)
{
  MbrItem *item = _get_(image, ITEM_MBR, index);
  MbrIndex *mbr;
  for (int i = 0; i < item->size; i++) {
    mbr = item->indexes + i;
    switch (mbr->kind) {
    case MBR_FIELD:
      /* code */
      image_load_field(image, mbr->index, func, arg);
      break;
    case MBR_METHOD:
      image_load_method(image, mbr->index, func, arg);
      break;
    case MBR_IFUNC:
      panic("MBR_IFUNC: not implemented");
      break;
    case MBR_EVAL:
      panic("MBR_EVAL: not implemented");
      break;
    default:
      panic("invalid mbr kind %d", mbr->kind);
      break;
    }
  }
}

#if 0
void Image_Get_Vars(Image *image, getvarfunc func, void *arg)
{
  VarItem *var;
  StringItem *id;
  TypeItem *type;
  TypeDesc *desc;
  Literal value;
  int size = _size_(image, ITEM_VAR);
  for (int i = 0; i < size; i++) {
    var = _get_(image, ITEM_VAR, i);
    id = _get_(image, ITEM_STRING, var->nameindex);
    type = _get_(image, ITEM_TYPE, var->typeindex);
    desc = to_typedesc(type, image);
    if (var->konst) {
      LiteralItem *item = _get_(image, ITEM_LITERAL, var->index);
      value = to_literal(item, image);
      func(id->data, desc, 1, &value, arg);
    } else {
      func(id->data, desc, 0, NULL, arg);
    }
    TYPE_DECREF(desc);
  }
}

void Image_Get_Funcs(Image *image, getfuncfunc func, void *arg)
{
  FuncItem *funcitem;
  StringItem *str;
  IndexItem *listitem;
  TypeItem *item;
  Vector *args;
  TypeDesc *ret;
  TypeDesc *desc;
  CodeItem *code;
  int size = _size_(image, ITEM_FUNC);
  for (int i = 0; i < size; i++) {
    funcitem = _get_(image, ITEM_FUNC, i);
    str = _get_(image, ITEM_STRING, funcitem->nameindex);
    code = _get_(image, ITEM_CODE, funcitem->codeindex);
    listitem = _get_(image, ITEM_INDEX, funcitem->pindex);
    item = _get_(image, ITEM_TYPE, funcitem->rindex);
    args = to_typedescvec(listitem, image);
    ret = to_typedesc(item, image);
    desc = desc_from_proto(args, ret);
    if (code != NULL)
      func(str->data, desc, ITEM_FUNC, code->codes, code->size, arg);
    else
      func(str->data, desc, ITEM_FUNC, NULL, 0, arg);
    TYPE_DECREF(desc);
  }
}

void Image_Get_Classes(Image *image, getclassfunc func, void *arg)
{
  ClassItem *item;
  TypeItem *type;
  StringItem *str;
  int size = _size_(image, ITEM_CLASS);
  for (int i = 0; i < size; i++) {
    item = _get_(image, ITEM_CLASS, i);
    type = _get_(image, ITEM_TYPE, item->classindex);
    str = _get_(image, ITEM_STRING, type->klass.typeindex);
    func(str->data, arg);
  }
}

void Image_Get_Enums(Image *image, getenumfunc func, void *arg)
{
  EnumItem *item;
  TypeItem *type;
  StringItem *str;
  int size = _size_(image, ITEM_ENUM);
  for (int i = 0; i < size; i++) {
    item = _get_(image, ITEM_ENUM, i);
    type = _get_(image, ITEM_TYPE, item->classindex);
    str = _get_(image, ITEM_STRING, type->klass.typeindex);
    func(str->data, arg);
  }
}
#endif

/*
void Image_Get_EVals(Image *image, getevalfunc func, void *arg)
{
  EValItem *item;
  TypeItem *type;
  IndexItem *typelist;
  StringItem *estr;
  StringItem *str;
  TypeDesc *desc;
  int size = _size_(image, ITEM_EVAL);
  for (int i = 0; i < size; i++) {
    item = _get_(image, ITEM_EVAL, i);
    type = _get_(image, ITEM_TYPE, item->classindex);
    estr = _get_(image, ITEM_STRING, type->typeindex);
    str = _get_(image, ITEM_STRING, item->nameindex);
    typelist = _get_(image, ITEM_TYPELIST, item->index);
    desc = TypeDesc_New_Tuple(TypeListItem_To_Vector(typelist, image));
    func(str->data, desc, item->value, estr->data, arg);
    TYPE_DECREF(desc);
  }
}
*/
#if 0
void Image_Get_Fields(Image *image, getfieldfunc func, void *arg)
{
  FieldItem *item;
  TypeItem *type;
  StringItem *classstr;
  StringItem *str;
  TypeDesc *desc;
  int size = _size_(image, ITEM_FIELD);
  for (int i = 0; i < size; i++) {
    item = _get_(image, ITEM_FIELD, i);
    type = _get_(image, ITEM_TYPE, item->classindex);
    classstr = _get_(image, ITEM_STRING, type->klass.typeindex);
    str = _get_(image, ITEM_STRING, item->nameindex);
    type = _get_(image, ITEM_TYPE, item->typeindex);
    desc = to_typedesc(type, image);
    func(str->data, desc, classstr->data, arg);
    TYPE_DECREF(desc);
  }
}

void Image_Get_Methods(Image *image, getmethodfunc func, void *arg)
{
  MethodItem *item;
  TypeItem *type;
  StringItem *classstr;
  StringItem *str;
  IndexItem *listitem;
  TypeItem *typeitem;
  Vector *args;
  TypeDesc *ret;
  TypeDesc *desc;
  CodeItem *code;
  int size = _size_(image, ITEM_METHOD);
  for (int i = 0; i < size; i++) {
    item = _get_(image, ITEM_METHOD, i);
    type = _get_(image, ITEM_TYPE, item->classindex);
    classstr = _get_(image, ITEM_STRING, type->klass.typeindex);
    str = _get_(image, ITEM_STRING, item->nameindex);
    code = _get_(image, ITEM_CODE, item->codeindex);
    listitem = _get_(image, ITEM_INDEX, item->pindex);
    typeitem = _get_(image, ITEM_TYPE, item->rindex);
    args = to_typedescvec(listitem, image);
    ret = to_typedesc(typeitem, image);
    desc = desc_from_proto(args, ret);
    func(str->data, desc, item->nrlocals,
         code->codes, code->size, classstr->data, arg);
    TYPE_DECREF(desc);
  }
}
#endif

void image_finish(Image *image)
{
  int size, length = 0, offset;
  MapItem *mapitem;
  void *item;

  offset = image->header.header_size;

  for (int i = 1; i < ITEM_MAX; i++) {
    size = _size_(image, i);
    if (size > 0)
      offset += sizeof(MapItem);
  }

  for (int i = 1; i < ITEM_MAX; i++) {
    size = _size_(image, i);
    if (size > 0) {
      offset += length;
      mapitem = mapitem_new(i, offset, size);
      _append_(image, ITEM_MAP, mapitem, 0);

      length = 0;
      for (int j = 0; j < size; j++) {
        item = _get_(image, i, j);
        length += item_func[i].length(item);
      }
    }
  }

  image->header.file_size = offset + length;
  image->header.map_size = _size_(image, ITEM_MAP);
}

static void __image_write_header(FILE *fp, Image *image)
{
  fwrite(&image->header, image->header.header_size, 1, fp);
}

static void __image_write_item(FILE *fp, Image *image, int type, int size)
{
  void *o;
  itemwritefunc write = item_func[type].write;
  expect(write != NULL);
  for (int i = 0; i < size; i++) {
    o = _get_(image, type, i);
    write(fp, o);
  }
}

static void __image_write_items(FILE *fp, Image *image)
{
  int size;
  for (int i = 0; i < ITEM_MAX; i++) {
    size = _size_(image, i);
    if (size > 0) {
      __image_write_item(fp, image, i, size);
    }
  }
}

static FILE *open_image_file(char *path, char *mode)
{
  FILE *fp = fopen(path, mode);
  if (fp == NULL) {
    char *end = strrchr(path, '/');
    char *dir = string_ndup(path, end - path);
    char *fmt = "mkdir -p %s";
    char *cmd = kmalloc(strlen(fmt) + strlen(dir));
    sprintf(cmd, fmt, dir);
    int status = system(cmd);
    expect(status == 0);
    kfree(cmd);
    kfree(dir);
    fp = fopen(path, "w");
  }
  return fp;
}

void image_write_file(Image *image, char *path)
{
  FILE *fp = open_image_file(path, "w");
  expect(fp != NULL);
  __image_write_header(fp, image);
  __image_write_items(fp, image);
  fflush(fp);
  fclose(fp);
}

static int header_check(ImageHeader *header)
{
  char *magic = (char *)header->magic;
  if (magic[0] != 'K') return -1;
  if (magic[1] != 'L') return -1;
  if (magic[2] != 'C') return -1;
  return 0;
}

Image *image_read_file(char *path, int unload)
{
#define LOAD(item) (!(unload & (1 << (item))))

  FILE *fp = fopen(path, "r");
  if (!fp) {
    print("error: cannot open %s file\n", path);
    return NULL;
  }

  ImageHeader header;
  int sz = fread(&header, sizeof(ImageHeader), 1, fp);
  if (sz < 1) {
    print("error: file %s is not a valid .klc file\n", path);
    fclose(fp);
    return NULL;
  }

  if (header_check(&header) < 0) {
    print("error: file %s is not a valid .klc file\n", path);
    return NULL;
  }

  Image *image = image_new(header.name);
  expect(image != NULL);
  memcpy(&image->header, &header, sizeof(ImageHeader));

  MapItem mapitems[header.map_size];
  int status = fseek(fp, header.map_offset, SEEK_SET);
  expect(status == 0);
  sz = fread(mapitems, sizeof(MapItem), header.map_size, fp);
  if (sz < (int)header.map_size) {
    print("error: file %s is not a valid .klc file\n", path);
    fclose(fp);
    return NULL;
  }

  MapItem *map;
  for (int i = 0; i < COUNT_OF(mapitems); i++) {
    map = mapitems + i;
    map = mapitem_new(map->type, map->offset, map->size);
    _append_(image, ITEM_MAP, map, 0);
  }

  for (int i = 0; i < COUNT_OF(mapitems); i++) {
    map = mapitems + i;
    status = fseek(fp, map->offset, SEEK_SET);
    expect(status == 0);
    switch (map->type) {
    case ITEM_STRING:
      if (LOAD(ITEM_STRING)) {
        StringItem *item;
        uint32_t len;
        for (int i = 0; i < map->size; i++) {
          sz = fread(&len, 4, 1, fp);
          expect(sz == 1);
          item = vargitem_new(sizeof(StringItem), sizeof(char), len);
          sz = fread(item->data, sizeof(char) * len, 1, fp);
          expect(sz == 1);
          _append_(image, ITEM_STRING, item, 1);
        }
      }
      break;
    case ITEM_LITERAL:
      if (LOAD(ITEM_LITERAL)) {
        LiteralItem *item;
        LiteralItem items[map->size];
        sz = fread(items, sizeof(LiteralItem), map->size, fp);
        expect(sz == map->size);
        for (int i = 0; i < map->size; i++) {
          item = item_copy(sizeof(LiteralItem), items + i);
          _append_(image, ITEM_LITERAL, item, 1);
        }
      }
      break;
    case ITEM_TYPE:
      if (LOAD(ITEM_TYPE)) {
        TypeItem *item;
        TypeItem items[map->size];
        sz = fread(items, sizeof(TypeItem), map->size, fp);
        expect(sz == map->size);
        for (int i = 0; i < map->size; i++) {
          item = item_copy(sizeof(TypeItem), items + i);
          _append_(image, ITEM_TYPE, item, 1);
        }
      }
      break;
    case ITEM_INDEX:
      if (LOAD(ITEM_INDEX)) {
        IndexItem *item;
        uint32_t len;
        for (int i = 0; i < map->size; i++) {
          sz = fread(&len, 4, 1, fp);
          expect(sz == 1);
          item = vargitem_new(sizeof(IndexItem), sizeof(int32_t), len);
          sz = fread(item->index, sizeof(int32_t) * len, 1, fp);
          expect(sz == 1);
          _append_(image, ITEM_INDEX, item, 1);
        }
      }
      break;
    case ITEM_CONST:
      if (LOAD(ITEM_CONST)) {
        ConstItem *item;
        ConstItem items[map->size];
        sz = fread(items, sizeof(ConstItem), map->size, fp);
        expect(sz == map->size);
        for (int i = 0; i < map->size; i++) {
          item = item_copy(sizeof(ConstItem), items + i);
          _append_(image, ITEM_CONST, item, 0);
        }
      }
      break;
    case ITEM_LOCVAR:
      if (LOAD(ITEM_LOCVAR)) {
        LocVarItem *item;
        LocVarItem items[map->size];
        sz = fread(items, sizeof(LocVarItem), map->size, fp);
        expect(sz == map->size);
        for (int i = 0; i < map->size; i++) {
          item = item_copy(sizeof(LocVarItem), items + i);
          _append_(image, ITEM_LOCVAR, item, 0);
        }
      }
      break;
    case ITEM_VAR:
      if (LOAD(ITEM_VAR)) {
        VarItem *item;
        VarItem items[map->size];
        sz = fread(items, sizeof(VarItem), map->size, fp);
        expect(sz == map->size);
        for (int i = 0; i < map->size; i++) {
          item = item_copy(sizeof(VarItem), items + i);
          _append_(image, ITEM_VAR, item, 0);
        }
      }
      break;
    case ITEM_CONSTVAR:
      if (LOAD(ITEM_CONSTVAR)) {
        ConstVarItem *item;
        ConstVarItem items[map->size];
        sz = fread(items, sizeof(ConstVarItem), map->size, fp);
        expect(sz == map->size);
        for (int i = 0; i < map->size; i++) {
          item = item_copy(sizeof(ConstVarItem), items + i);
          _append_(image, ITEM_CONSTVAR, item, 0);
        }
      }
      break;
    case ITEM_FUNC:
      if (LOAD(ITEM_FUNC)) {
        FuncItem *item;
        FuncItem items[map->size];
        sz = fread(items, sizeof(FuncItem), map->size, fp);
        expect(sz == map->size);
        for (int i = 0; i < map->size; i++) {
          item = item_copy(sizeof(FuncItem), items + i);
          _append_(image, ITEM_FUNC, item, 0);
        }
      }
      break;
    case ITEM_ANONY:
      if (LOAD(ITEM_ANONY)) {
        AnonyItem *item;
        AnonyItem items[map->size];
        sz = fread(items, sizeof(AnonyItem), map->size, fp);
        expect(sz == map->size);
        for (int i = 0; i < map->size; i++) {
          item = item_copy(sizeof(AnonyItem), items + i);
          _append_(image, ITEM_ANONY, item, 0);
        }
      }
      break;
    case ITEM_CODE:
      if (LOAD(ITEM_CODE)) {
        CodeItem *item;
        uint32_t len;
        for (int i = 0; i < map->size; i++) {
          sz = fread(&len, 4, 1, fp);
          expect(sz == 1);
          if (len > 0) {
            item = vargitem_new(sizeof(CodeItem), sizeof(uint8_t), len);
            sz = fread(item->codes, sizeof(uint8_t) * len, 1, fp);
            expect(sz == 1);
            _append_(image, ITEM_CODE, item, 0);
          }
        }
      }
      break;
    case ITEM_CLASS:
      if (LOAD(ITEM_CLASS)) {
        ClassItem *item;
        ClassItem items[map->size];
        sz = fread(items, sizeof(ClassItem), map->size, fp);
        expect(sz == map->size);
        for (int i = 0; i < map->size; i++) {
          item = item_copy(sizeof(ClassItem), items + i);
          _append_(image, ITEM_CLASS, item, 0);
        }
      }
      break;
    case ITEM_FIELD:
      if (LOAD(ITEM_FIELD)) {
        VarItem *item;
        VarItem items[map->size];
        sz = fread(items, sizeof(VarItem), map->size, fp);
        expect(sz == map->size);
        for (int i = 0; i < map->size; i++) {
          item = item_copy(sizeof(VarItem), items + i);
          _append_(image, ITEM_FIELD, item, 0);
        }
      }
      break;
    case ITEM_METHOD:
      if (LOAD(ITEM_METHOD)) {
        FuncItem *item;
        FuncItem items[map->size];
        sz = fread(items, sizeof(FuncItem), map->size, fp);
        expect(sz == map->size);
        for (int i = 0; i < map->size; i++) {
          item = item_copy(sizeof(FuncItem), items + i);
          _append_(image, ITEM_METHOD, item, 0);
        }
      }
      break;
    case ITEM_TRAIT:
      if (LOAD(ITEM_TRAIT)) {
        ClassItem *item;
        ClassItem items[map->size];
        sz = fread(items, sizeof(ClassItem), map->size, fp);
        expect(sz == map->size);
        for (int i = 0; i < map->size; i++) {
          item = item_copy(sizeof(ClassItem), items + i);
          _append_(image, ITEM_TRAIT, item, 0);
        }
      }
      break;
    case ITEM_IFUNC:
      if (LOAD(ITEM_IFUNC)) {
        IFuncItem *item;
        IFuncItem items[map->size];
        sz = fread(items, sizeof(IFuncItem), map->size, fp);
        expect(sz == map->size);
        for (int i = 0; i < map->size; i++) {
          item = item_copy(sizeof(IFuncItem), items + i);
          _append_(image, ITEM_IFUNC, item, 0);
        }
      }
      break;
    case ITEM_ENUM:
      if (LOAD(ITEM_ENUM)) {
        EnumItem *item;
        EnumItem items[map->size];
        sz = fread(items, sizeof(EnumItem), map->size, fp);
        expect(sz == map->size);
        for (int i = 0; i < map->size; i++) {
          item = item_copy(sizeof(EnumItem), items + i);
          _append_(image, ITEM_ENUM, item, 0);
        }
      }
      break;
    case ITEM_EVAL:
      if (LOAD(ITEM_EVAL)) {
        EValItem *item;
        EValItem items[map->size];
        sz = fread(items, sizeof(EValItem), map->size, fp);
        expect(sz == map->size);
        for (int i = 0; i < map->size; i++) {
          item = item_copy(sizeof(EValItem), items + i);
          _append_(image, ITEM_EVAL, item, 0);
        }
      }
      break;
    case ITEM_MBR:
      if (LOAD(ITEM_MBR)) {
        MbrItem *item;
        uint32_t len;
        for (int i = 0; i < map->size; i++) {
          sz = fread(&len, 4, 1, fp);
          expect(sz == 1);
          item = vargitem_new(sizeof(MbrItem), sizeof(MbrIndex), len);
          sz = fread(item->indexes, sizeof(MbrIndex) * len, 1, fp);
          expect(sz == 1);
          _append_(image, ITEM_MBR, item, 0);
        }
      }
      break;
    default:
      panic("invalid map %d", map->type);
      break;
    }
  }

  fclose(fp);
  return image;
}

void header_show(ImageHeader *h)
{
  print("magic:%s\n", (char *)h->magic);
  print("version:%d.%d.%d\n", h->version[0] - '0', h->version[1] - '0', 0);
  print("header_size:%d\n", h->header_size);
  print("endian:0x%x\n", h->endian_tag);
  print("map_offset:0x%x\n", h->map_offset);
  print("map_size:%d\n\n", h->map_size);
  puts("--------------------");
}

void image_show(Image *image)
{
  if (image == NULL)
    return;
  puts("\n------show image--------------\n");
  ImageHeader *h = &image->header;
  header_show(h);

  void *item;
  int size;
  print("map:\n");
  size = _size_(image, 0);
  for (int j = 0; j < size; j++) {
    print("[%d]\n", j);
    item = _get_(image, 0, j);
    item_func[0].show(image, item);
  }

  for (int i = 1; i < image->size; i++) {
    if (i == ITEM_CODE)
      continue;
    size = _size_(image, i);
    if (size > 0) {
      puts("--------------------");
      print("%s:\n", mapitem_string[i]);
      for (int j = 0; j < size; j++) {
        print("[%d]\n", j);
        item = _get_(image, i, j);
        item_func[i].show(image, item);
      }
    }
  }

  puts("\n------end of image------------");
}

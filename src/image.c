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

static int typelistitem_length(void *o)
{
  TypeListItem *item = o;
  return sizeof(TypeListItem) + item->size * sizeof(int32_t);
}

static void typelistitem_write(FILE *fp, void *o)
{
  TypeListItem *item = o;
  fwrite(o, sizeof(TypeListItem) + item->size * sizeof(int32_t), 1, fp);
}

static unsigned int typelistitem_hash(void *k)
{
  int size = sizeof(TypeListItem);
  TypeListItem *item = k;
  size += item->size * sizeof(int32_t);
  return memhash(k, size);
}

static int typelistitem_equal(void *k1, void *k2)
{
  TypeListItem *item1 = k1;
  TypeListItem *item2 = k2;
  if (item1->size != item2->size)
    return 0;
  for (int i = 0; i < item1->size; i++) {
    if (item1->index[i] != item2->index[i])
      return 0;
  }
  return 1;
}

static void typelistitem_show(Image *image, void *o)
{
  TypeListItem *item = o;
  TypeItem *type;
  for (int i = 0; i < item->size; i++) {
    puts("  ---------");
    print("  index:%d\n", item->index[i]);
    type = _get_(image, ITEM_TYPE, item->index[i]);
    typeitem_show(image, type);
  }
}

static void typelistitem_free(void *o)
{
  kfree(o);
}

static TypeListItem *typelistitem_new(int size, int32_t index[])
{
  TypeListItem *item = kmalloc(sizeof(TypeListItem) + size * sizeof(int32_t));
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
                                  int32_t pos, int index)
{
  LocVarItem *item = kmalloc(sizeof(LocVarItem));
  item->nameindex = nameindex;
  item->typeindex = typeindex;
  item->pos = pos;
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

static char *access_tostring(int access)
{
  char *str;
  switch (access) {
  case 0:
    str = "var,public";
    break;
  case 1:
    str = "var,private";
    break;
  case 2:
    str = "const,public";
    break;
  case 3:
    str = "const,private";
    break;
  default:
    str = "";
    break;
  }
  return str;
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
  print("  const:%s\n", item->konst ? "true" : "false");

}

static void varitem_free(void *o)
{
  kfree(o);
}

static VarItem *varitem_new(int32_t nameindex, int32_t typeindex,
                            int konst, int index)
{
  VarItem *item = kmalloc(sizeof(VarItem));
  item->nameindex = nameindex;
  item->typeindex = typeindex;
  item->konst = konst;
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
  print("  locals:%d\n", item->nrlocals);
}

static void funcitem_free(void *o)
{
  kfree(o);
}

static FuncItem *funcitem_new(int nameindex, int pindex, int rindex,
                              int codeindex)
{
  FuncItem *item = kmalloc(sizeof(FuncItem));
  item->nameindex = nameindex;
  item->pindex = pindex;
  item->rindex = rindex;
  item->codeindex = codeindex;
  return item;
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
  print("  locals:%d\n", item->nrlocals);
  print("  upvals:%d\n", item->nrupvals);
}

static void anonyitem_free(void *o)
{
  kfree(o);
}

static AnonyItem *anonyitem_new(int nameindex, int pindex, int rindex,
  int codeindex, int locals, int upvals)
{
  AnonyItem *item = kmalloc(sizeof(AnonyItem));
  item->pindex = pindex;
  item->rindex = rindex;
  item->codeindex = codeindex;
  item->nrlocals = locals;
  item->nrupvals = upvals;
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
  print("  classindex:%d\n", item->classindex);
  if (item->superindex >= 0) {
    print("  superinfo:\n");
    TypeItem *type = _get_(image, ITEM_TYPE, item->superindex);
    typeitem_show(image, type);
  }
}

static void classitem_free(void *o)
{
  kfree(o);
}

static ClassItem *classitem_new(int classindex, int superindex)
{
  ClassItem *item = kmalloc(sizeof(ClassItem));
  item->classindex = classindex;
  item->superindex = superindex;
  return item;
}

static int fielditem_length(void *o)
{
  return sizeof(FieldItem);
}

static void fielditem_write(FILE *fp, void *o)
{
  fwrite(o, sizeof(FieldItem), 1, fp);
}

static void fielditem_show(Image *image, void *o)
{
  FieldItem *item = o;
  print("  classindex:%d\n", item->classindex);
  StringItem *id = _get_(image, ITEM_STRING, item->nameindex);
  stringitem_show(image, id);
  TypeItem *type = _get_(image, ITEM_TYPE, item->typeindex);
  typeitem_show(image, type);
}

static void fielditem_free(void *o)
{
  kfree(o);
}

static FieldItem *fielditem_new(int nameindex, int classindex, int typeindex)
{
  FieldItem *item = kmalloc(sizeof(FieldItem));
  item->classindex = classindex;
  item->nameindex = nameindex;
  item->typeindex = typeindex;
  return item;
}

static int methoditem_length(void *o)
{
  return sizeof(MethodItem);
}

static void methoditem_write(FILE *fp, void *o)
{
  fwrite(o, sizeof(MethodItem), 1, fp);
}

static void methoditem_show(Image *image, void *o)
{
  MethodItem *item = o;
  print("  classindex:%d\n", item->classindex);
  StringItem *str;
  print("  nameindex:%d\n", item->nameindex);
  str = _get_(image, ITEM_STRING, item->nameindex);
  print("  (%s)\n", str->data);
  print("  pindex:%d\n", item->pindex);
  print("  rindex:%d\n", item->rindex);
  print("  codeindex:%d\n", item->codeindex);
}

static void methoditem_free(void *o)
{
  kfree(o);
}

static MethodItem *methoditem_new(int nameindex, int classindex,
                                  int pindex, int rindex, int codeindex)
{
  MethodItem *item = kmalloc(sizeof(MethodItem));
  item->classindex = classindex;
  item->nameindex = nameindex;
  item->pindex = pindex;
  item->rindex = rindex;
  item->codeindex = codeindex;
  return item;
}

static int traititem_length(void *o)
{
  return sizeof(TraitItem);
}

static void traititem_write(FILE *fp, void *o)
{
  fwrite(o, sizeof(TraitItem), 1, fp);
}

static void traititem_show(Image *image, void *o)
{
  TraitItem *item = o;
  print("  classindex:%d\n", item->classindex);
  TypeItem *type = _get_(image, ITEM_TYPE, item->classindex);
  typeitem_show(image, type);
  if (item->traitsindex >= 0) {
    TypeListItem *typelist;
    typelist = _get_(image, ITEM_TYPELIST, item->traitsindex);
    typelistitem_show(image, typelist);
  }
}

static void traititem_free(void *o)
{
  kfree(o);
}

static TraitItem *traititem_new(int classindex, int traitsindex)
{
  TraitItem *item = kmalloc(sizeof(TraitItem));
  item->classindex = classindex;
  item->traitsindex = traitsindex;
  return item;
}

static int nfuncitem_length(void *o)
{
  return sizeof(NFuncItem);
}

static void nfuncitem_write(FILE *fp, void *o)
{
  fwrite(o, sizeof(NFuncItem), 1, fp);
}

static void nfuncitem_show(Image *image, void *o)
{
  NFuncItem *item = o;
  print("  classindex:%d\n", item->classindex);
  StringItem *str;
  print("  nameindex:%d\n", item->nameindex);
  str = _get_(image, ITEM_STRING, item->nameindex);
  print("  (%s)\n", str->data);
  print("  pindex:%d\n", item->pindex);
  print("  rindex:%d\n", item->rindex);
}

static void nfuncitem_free(void *o)
{
  kfree(o);
}

static NFuncItem *nfuncitem_new(int nameindex, int classindex,
                                int pindex, int rindex)
{
  NFuncItem *item = kmalloc(sizeof(NFuncItem));
  item->classindex = classindex;
  item->nameindex = nameindex;
  item->pindex = pindex;
  item->rindex = rindex;
  return item;
}

static int imethitem_length(void *o)
{
  return sizeof(IMethItem);
}

static void imethitem_write(FILE *fp, void *o)
{
  fwrite(o, sizeof(IMethItem), 1, fp);
}

static void imethitem_show(Image *image, void *o)
{
  IMethItem *item = o;
  print("  classindex:%d\n", item->classindex);
  StringItem *str;
  print("  nameindex:%d\n", item->nameindex);
  str = _get_(image, ITEM_STRING, item->nameindex);
  print("  (%s)\n", str->data);
  print("  pindex:%d\n", item->pindex);
  print("  rindex:%d\n", item->rindex);
}

static void imethitem_free(void *o)
{
  kfree(o);
}

static IMethItem *imethoditem_new(int nameindex, int classindex,
                                  int pindex, int rindex)
{
  IMethItem *item = kmalloc(sizeof(IMethItem));
  item->classindex = classindex;
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
  print("  classindex:%d\n", item->classindex);
}

static void enumitem_free(void *o)
{
  kfree(o);
}

static EnumItem *enumitem_new(int classindex)
{
  EnumItem *item = kmalloc(sizeof(EnumItem));
  item->classindex = classindex;
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
  print("  classindex:%d\n", item->classindex);
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

static EValItem *evalitem_new(int nameindex, int classindex, int index, int32_t val)
{
  EValItem *item = kmalloc(sizeof(EValItem));
  item->classindex = classindex;
  item->nameindex = nameindex;
  item->index = index;
  item->value = val;
  return item;
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

static int typelistitem_get(Image *image, Vector *vec)
{
  int sz = vector_size(vec);
  if (sz <= 0)
    return -1;

  uint8_t data[sizeof(TypeListItem) + sizeof(int32_t) * sz];
  TypeListItem *item = (TypeListItem *)data;
  item->size = sz;

  int index;
  TypeDesc *desc;
  vector_for_each(desc, vec) {
    index = typeitem_get(image, desc);
    if (index < 0)
      return -1;
    item->index[idx] = index;
  }

  return _index_(image, ITEM_TYPELIST, item);
}

static int typelistitem_set(Image *image, Vector *vec)
{
  int sz = vector_size(vec);
  if (sz <= 0)
    return -1;

  int index = typelistitem_get(image, vec);
  if (index < 0) {
    int32_t indexes[sz];
    TypeDesc *desc;
    vector_for_each(desc, vec) {
      index = typeitem_set(image, desc);
      expect(index >= 0);
      indexes[idx] = index;
    }
    TypeListItem *item = typelistitem_new(sz, indexes);
    index = _append_(image, ITEM_TYPELIST, item, 1);
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
      parasindex = typelistitem_get(image, desc->paras);
    }
    int typesindex = -1;
    if (desc->types != NULL) {
      typesindex = typelistitem_get(image, desc->types);
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
    int pindex = typelistitem_get(image, desc->proto.args);
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
        parasindex = typelistitem_set(image, desc->paras);
      }
      int typesindex = -1;
      if (desc->types != NULL) {
        typesindex = typelistitem_set(image, desc->types);
      }
      item = typeitem_klass_new(pathindex, typeindex);
      item->parasindex = parasindex;
      item->typesindex = typesindex;
      break;
    }
    case TYPE_PROTO: {
      int rindex = typeitem_set(image, desc->proto.ret);
      int pindex = typelistitem_set(image, desc->proto.args);
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
    typelistitem_length, typelistitem_write,
    typelistitem_hash, typelistitem_equal,
    typelistitem_show, typelistitem_free,
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
    fielditem_length, fielditem_write,
    NULL, NULL,
    fielditem_show, fielditem_free,
  },
  {
    methoditem_length, methoditem_write,
    NULL, NULL,
    methoditem_show, methoditem_free,
  },
  {
    traititem_length, traititem_write,
    NULL, NULL,
    traititem_show, traititem_free,
  },
  {
    nfuncitem_length, nfuncitem_write,
    NULL, NULL,
    nfuncitem_show, nfuncitem_free,
  },
  {
    imethitem_length, imethitem_write,
    NULL, NULL,
    imethitem_show, imethitem_free,
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

int Image_Add_Integer(Image *image, int64_t val)
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

int Image_Add_Float(Image *image, double val)
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

int Image_Add_Bool(Image *image, int val)
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

int Image_Add_String(Image *image, char *val)
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

int Image_Add_UChar(Image *image, wchar val)
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

int Image_Add_Literal(Image *image, Literal *val)
{
  int index;
  if (val->kind == BASE_INT) {
    index = Image_Add_Integer(image, val->ival);
  } else if (val->kind == BASE_STR) {
    index = Image_Add_String(image, val->str);
  } else if (val->kind == BASE_BOOL) {
    index = Image_Add_Bool(image, val->bval);
  } else if (val->kind == BASE_FLOAT) {
    index = Image_Add_Float(image, val->fval);
  } else if (val->kind == BASE_CHAR) {
    index = Image_Add_UChar(image, val->cval);
  } else {
    index = -1;
  }
  return index;
}

int Image_Add_Desc(Image *image, TypeDesc *desc)
{
  expect(desc != NULL);
  int index = typeitem_set(image, desc);
  return image_add_const(image, CONST_TYPE, index);
}

int Image_Add_Anony(Image *image, char *name, TypeDesc *desc,
                    uint8_t *codes, int size, int locals, int upvals)
{
  int nameindex = stringitem_set(image, name);
  int pindex = typelistitem_set(image, desc->proto.args);
  int rindex = typeitem_set(image, desc->proto.ret);
  int codeindex = codeitem_set(image, codes, size);
  AnonyItem *anony = anonyitem_new(nameindex, pindex, rindex, codeindex,
                                   locals, upvals);
  int index = _append_(image, ITEM_ANONY, anony, 0);
  return image_add_const(image, CONST_ANONY, index);
}

void Image_Add_Var(Image *image, char *name, TypeDesc *desc)
{
  int type_index = typeitem_set(image, desc);
  int name_index = stringitem_set(image, name);
  VarItem *varitem = varitem_new(name_index, type_index, 0, -1);
  _append_(image, ITEM_VAR, varitem, 0);
}

void Image_Add_Const(Image *image, char *name, TypeDesc *desc,
                     Literal *val)
{
  int type_index = typeitem_set(image, desc);
  int name_index = stringitem_set(image, name);
  int index = Image_Add_Literal(image, val);
  VarItem *varitem = varitem_new(name_index, type_index, 1, index);
  _append_(image, ITEM_VAR, varitem, 0);
}

void Image_Add_LocVar(Image *image, char *name, TypeDesc *desc,
                      int pos, int index)
{
  int typeindex = typeitem_set(image, desc);
  int nameindex = stringitem_set(image, name);
  LocVarItem *item = locvaritem_new(nameindex, typeindex, pos, index);
  _append_(image, ITEM_LOCVAR, item, 0);
}

int Image_Add_Func(Image *image, char *name, TypeDesc *desc,
                   uint8_t *codes, int size, int locals)
{
  int nameindex = stringitem_set(image, name);
  int pindex = typelistitem_set(image, desc->proto.args);
  int rindex = typeitem_set(image, desc->proto.ret);
  int codeindex = codeitem_set(image, codes, size);
  FuncItem *funcitem = funcitem_new(nameindex, pindex, rindex, codeindex);
  funcitem->nrlocals = locals;
  return _append_(image, ITEM_FUNC, funcitem, 0);
}

void Image_Add_Class(Image *image, char *name, Vector *supers)
{
  TypeDesc tmp = {.kind = TYPE_KLASS, .klass.type = name};
  int classindex = typeitem_set(image, &tmp);
  int superindex = typelistitem_set(image, supers);
  ClassItem *classitem = classitem_new(classindex, superindex);
  _append_(image, ITEM_CLASS, classitem, 0);
}

void Image_Add_Field(Image *image, char *clazz, char *name, TypeDesc *desc)
{
  TypeDesc tmp = {.kind = TYPE_KLASS, .klass.type = clazz};
  int classindex = typeitem_set(image, &tmp);
  int nameindex = stringitem_set(image, name);
  int typeindex = typeitem_set(image, desc);
  FieldItem *fielditem = fielditem_new(classindex, nameindex, typeindex);
  _append_(image, ITEM_FIELD, fielditem, 0);
}

int Image_Add_Method(Image *image, char *klazz, char *name, TypeDesc *desc,
                      uint8_t *codes, int csz, int locals)
{
  TypeDesc tmp = {.kind = TYPE_KLASS, .klass.type = klazz};
  int classindex = typeitem_set(image, &tmp);
  int nameindex = stringitem_set(image, name);
  int pindex = typelistitem_set(image, desc->proto.args);
  int rindex = typeitem_set(image, desc->proto.ret);
  int codeindex = codeitem_set(image, codes, csz);
  MethodItem *methitem = methoditem_new(classindex, nameindex,
                                        pindex, rindex, codeindex);
  methitem->nrlocals = locals;
  return _append_(image, ITEM_METHOD, methitem, 0);
}

void Image_Add_Trait(Image *image, char *name, Vector *traits)
{
  TypeDesc tmp = {.kind = TYPE_KLASS, .klass.type = name};
  int classindex = typeitem_set(image, &tmp);
  int traitsindex = typelistitem_set(image, traits);
  TraitItem *traititem = traititem_new(classindex, traitsindex);
  _append_(image, ITEM_TRAIT, traititem, 0);
}

void Image_Add_NFunc(Image *image, char *klazz, char *name, TypeDesc *desc)
{
  int classindex = -1;
  if (klazz != NULL) {
    TypeDesc tmp = {.kind = TYPE_KLASS, .klass.type = klazz};
    classindex = typeitem_set(image, &tmp);
  }
  int nameindex = stringitem_set(image, name);
  int pindex = typelistitem_set(image, desc->proto.args);
  int rindex = typeitem_set(image, desc->proto.ret);
  NFuncItem *nfuncitem = nfuncitem_new(classindex, nameindex,
                                       pindex, rindex);
  _append_(image, ITEM_NFUNC, nfuncitem, 0);
}

void Image_Add_IMeth(Image *image, char *trait, char *name, TypeDesc *desc)
{
  TypeDesc tmp = {.kind = TYPE_KLASS, .klass.type = trait};
  int classindex = typeitem_set(image, &tmp);
  int nameindex = stringitem_set(image, name);
  int pindex = typelistitem_set(image, desc->proto.args);
  int rindex = typeitem_set(image, desc->proto.ret);
  IMethItem *imethitem = imethoditem_new(classindex, nameindex,
                                         pindex, rindex);
  _append_(image, ITEM_IMETH, imethitem, 0);
}

void Image_Add_Enum(Image *image, char *name)
{
  TypeDesc tmp = {.kind = TYPE_KLASS, .klass.type = name};
  int classindex = typeitem_set(image, &tmp);
  EnumItem *enumitem = enumitem_new(classindex);
  _append_(image, ITEM_ENUM, enumitem, 0);
}

void Image_Add_EVal(Image *image, char *klazz, char *name,
                     Vector *types, int32_t val)
{
  TypeDesc tmp = {.kind = TYPE_KLASS, .klass.type = klazz};
  int classindex = typeitem_set(image, &tmp);
  int nameindex = stringitem_set(image, name);
  int index = typelistitem_set(image, types);

  EValItem *evitem = evalitem_new(classindex, nameindex, index, val);
  _append_(image, ITEM_EVAL, evitem, 0);
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

Image *Image_New(char *name)
{
  int sz = sizeof(Image) + ITEM_MAX * sizeof(Vector);
  Image *image = kmalloc(sz);
  init_header(&image->header, name);
  hashmap_init(&image->map, _item_equal_);
  image->size = ITEM_MAX;
  return image;
}

void Image_Free(Image *image)
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

static TypeDesc *to_typedesc(TypeItem *item, Image *image);

static Vector *to_typedescvec(TypeListItem *item, Image *atbl)
{
  if (item == NULL)
    return NULL;

  Vector *v = vector_new();
  TypeItem *typeitem;
  TypeDesc *t;
  for (int i = 0; i < item->size; i++) {
    typeitem = _get_(atbl, ITEM_TYPE, item->index[i]);
    t = to_typedesc(typeitem, atbl);
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
      TypeListItem *listitem = _get_(image, ITEM_TYPELIST, item->parasindex);
      t->paras = to_typedescvec(listitem, image);
    }
    if (item->typesindex >= 0) {
      TypeListItem *listitem = _get_(image, ITEM_TYPELIST, item->typesindex);
      t->types = to_typedescvec(listitem, image);
    }
    break;
  }
  case TYPE_PROTO: {
    TypeListItem *listitem = _get_(image, ITEM_TYPELIST, item->proto.pindex);
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

int Image_Const_Count(Image *image)
{
  return _size_(image, ITEM_CONST);
}

void Image_Get_Consts(Image *image, getconstfunc func, void *arg)
{
  void *data;
  ConstItem *item;
  LiteralItem *liteitem;
  TypeItem *typeitem;
  TypeListItem *typelistitem;
  AnonyItem *anony;
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
      anony = _get_(image, ITEM_ANONY, item->index);
      StringItem *str = _get_(image, ITEM_STRING, anony->nameindex);
      CodeItem *code = _get_(image, ITEM_CODE, anony->codeindex);
      TypeListItem *listitem = _get_(image, ITEM_TYPELIST, anony->pindex);
      TypeItem *item = _get_(image, ITEM_TYPE, anony->rindex);
      Vector *args = to_typedescvec(listitem, image);
      TypeDesc *ret = to_typedesc(item, image);
      desc = desc_from_proto(args, ret);
      FuncInfo funcinfo = {
        str->data, desc, code->codes, code->size,
        anony->nrlocals, anony->nrupvals
      };
      func(&funcinfo, CONST_ANONY, i, arg);
      TYPE_DECREF(ret);
      TYPE_DECREF(desc);
    }
  }
}

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

void Image_Get_LocVars(Image *image, getlocvarfunc func, void *arg)
{
  LocVarItem *locvar;
  StringItem *str;
  TypeItem *type;
  TypeDesc *desc;
  int size = _size_(image, ITEM_LOCVAR);
  for (int i = 0; i < size; i++) {
    locvar = _get_(image, ITEM_LOCVAR, i);
    str = _get_(image, ITEM_STRING, locvar->nameindex);;
    type = _get_(image, ITEM_TYPE, locvar->typeindex);
    desc = to_typedesc(type, image);
    func(str->data, desc, locvar->pos, locvar->index, arg);
    TYPE_DECREF(desc);
  }
}

void Image_Get_Funcs(Image *image, getfuncfunc func, void *arg)
{
  FuncItem *funcitem;
  StringItem *str;
  TypeListItem *listitem;
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
    listitem = _get_(image, ITEM_TYPELIST, funcitem->pindex);
    item = _get_(image, ITEM_TYPE, funcitem->rindex);
    args = to_typedescvec(listitem, image);
    ret = to_typedesc(item, image);
    desc = desc_from_proto(args, ret);
    if (code != NULL)
      func(str->data, desc, funcitem->nrlocals, ITEM_FUNC,
           code->codes, code->size, arg);
    else
      func(str->data, desc, funcitem->nrlocals, ITEM_FUNC, NULL, 0, arg);
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

void Image_Get_NFuncs(Image *image, getfuncfunc func, void *arg)
{
  NFuncItem *nfuncitem;
  StringItem *str;
  TypeListItem *listitem;
  TypeItem *item;
  Vector *args;
  TypeDesc *ret;
  TypeDesc *desc;
  int size = _size_(image, ITEM_NFUNC);
  for (int i = 0; i < size; i++) {
    nfuncitem = _get_(image, ITEM_NFUNC, i);
    str = _get_(image, ITEM_STRING, nfuncitem->nameindex);
    listitem = _get_(image, ITEM_TYPELIST, nfuncitem->pindex);
    item = _get_(image, ITEM_TYPE, nfuncitem->rindex);
    args = to_typedescvec(listitem, image);
    ret = to_typedesc(item, image);
    desc = desc_from_proto(args, ret);
    func(str->data, desc, i, ITEM_NFUNC, NULL, 0, arg);
    TYPE_DECREF(desc);
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

/*
void Image_Get_EVals(Image *image, getevalfunc func, void *arg)
{
  EValItem *item;
  TypeItem *type;
  TypeListItem *typelist;
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
  TypeListItem *listitem;
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
    listitem = _get_(image, ITEM_TYPELIST, item->pindex);
    typeitem = _get_(image, ITEM_TYPE, item->rindex);
    args = to_typedescvec(listitem, image);
    ret = to_typedesc(typeitem, image);
    desc = desc_from_proto(args, ret);
    func(str->data, desc, item->nrlocals,
         code->codes, code->size, classstr->data, arg);
    TYPE_DECREF(desc);
  }
}

void Image_Finish(Image *image)
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

void Image_Write_File(Image *image, char *path)
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

Image *Image_Read_File(char *path, int unload)
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

  Image *image = Image_New(header.name);
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
    case ITEM_TYPELIST:
      if (LOAD(ITEM_TYPELIST)) {
        TypeListItem *item;
        uint32_t len;
        for (int i = 0; i < map->size; i++) {
          sz = fread(&len, 4, 1, fp);
          expect(sz == 1);
          item = vargitem_new(sizeof(TypeListItem), sizeof(int32_t), len);
          sz = fread(item->index, sizeof(int32_t) * len, 1, fp);
          expect(sz == 1);
          _append_(image, ITEM_TYPELIST, item, 1);
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
        FieldItem *item;
        FieldItem items[map->size];
        sz = fread(items, sizeof(FieldItem), map->size, fp);
        expect(sz == map->size);
        for (int i = 0; i < map->size; i++) {
          item = item_copy(sizeof(FieldItem), items + i);
          _append_(image, ITEM_FIELD, item, 0);
        }
      }
      break;
    case ITEM_METHOD:
      if (LOAD(ITEM_METHOD)) {
        MethodItem *item;
        MethodItem items[map->size];
        sz = fread(items, sizeof(MethodItem), map->size, fp);
        expect(sz == map->size);
        for (int i = 0; i < map->size; i++) {
          item = item_copy(sizeof(MethodItem), items + i);
          _append_(image, ITEM_METHOD, item, 0);
        }
      }
      break;
    case ITEM_TRAIT:
      if (LOAD(ITEM_TRAIT)) {
        TraitItem *item;
        TraitItem items[map->size];
        sz = fread(items, sizeof(TraitItem), map->size, fp);
        expect(sz == map->size);
        for (int i = 0; i < map->size; i++) {
          item = item_copy(sizeof(TraitItem), items + i);
          _append_(image, ITEM_TRAIT, item, 0);
        }
      }
      break;
    case ITEM_NFUNC:
      if (LOAD(ITEM_NFUNC)) {
        NFuncItem *item;
        NFuncItem items[map->size];
        sz = fread(items, sizeof(NFuncItem), map->size, fp);
        expect(sz == map->size);
        for (int i = 0; i < map->size; i++) {
          item = item_copy(sizeof(NFuncItem), items + i);
          _append_(image, ITEM_NFUNC, item, 0);
        }
      }
      break;
    case ITEM_IMETH:
      if (LOAD(ITEM_IMETH)) {
        IMethItem *item;
        IMethItem items[map->size];
        sz = fread(items, sizeof(IMethItem), map->size, fp);
        expect(sz == map->size);
        for (int i = 0; i < map->size; i++) {
          item = item_copy(sizeof(IMethItem), items + i);
          _append_(image, ITEM_IMETH, item, 0);
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

void Image_Show(Image *image)
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

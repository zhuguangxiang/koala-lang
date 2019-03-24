/*
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "image.h"
#include "hashfunc.h"
#include "stringex.h"
#include "mem.h"
#include "log.h"

LOGGER(1)

static int version_major = 0; // 1 byte
static int version_minor = 1; // 1 byte
static int version_build = 1; // 2 bytes

#define ENDIAN_TAG  0x1a2b3c4d

TypeDesc *ProtoItem_To_TypeDesc(ProtoItem *item, AtomTable *atbl);

TypeDesc *TypeItem_To_TypeDesc(TypeItem *item, AtomTable *atbl)
{
  TypeDesc *t = NULL;
  switch (item->kind) {
  case TYPE_BASE:
    t = TypeDesc_Get_Base(item->primitive);
    break;
  case TYPE_KLASS: {
    StringItem *s;
    char *path;
    char *type;
    if (item->pathindex >= 0) {
      s = AtomTable_Get(atbl, ITEM_STRING, item->pathindex);
      path = AtomString_New(s->data).str;
    } else {
      path = NULL;
    }
    s = AtomTable_Get(atbl, ITEM_STRING, item->typeindex);
    type = AtomString_New(s->data).str;
    t = TypeDesc_Get_Klass(path, type);
    break;
  }
  case TYPE_PROTO: {
    ProtoItem *proto = AtomTable_Get(atbl, ITEM_PROTO, item->protoindex);
    t = ProtoItem_To_TypeDesc(proto, atbl);
    break;
  }
  case TYPE_ARRAY: {
    TypeItem *base = AtomTable_Get(atbl, ITEM_TYPE, item->array.typeindex);
    TypeDesc *base_type = TypeItem_To_TypeDesc(base, atbl);
    t = TypeDesc_Get_Array(item->array.dims, base_type);
    break;
  }
  case TYPE_VARG: {
    TypeItem *base = AtomTable_Get(atbl, ITEM_TYPE, item->varg.typeindex);
    TypeDesc *desc = TypeItem_To_TypeDesc(base, atbl);
    t = TypeDesc_Get_Varg(desc);
    break;
  }
  default:
    assert(0);
    break;
  }
  return t;
}

Vector *TypeListItem_To_Vector(TypeListItem *item, AtomTable *atbl)
{
  if (item == NULL)
    return NULL;

  Vector *v = Vector_Capacity(item->size);
  TypeItem *typeitem;
  TypeDesc *t;
  for (int i = 0; i < item->size; i++) {
    typeitem = AtomTable_Get(atbl, ITEM_TYPE, item->index[i]);
    t = TypeItem_To_TypeDesc(typeitem, atbl);
    TYPE_INCREF(t);
    Vector_Append(v, t);
  }
  return v;
}

TypeDesc *ProtoItem_To_TypeDesc(ProtoItem *item, AtomTable *atbl)
{
  Vector *ret;
  Vector *arg;
  TypeListItem *typelist;

  if (item->rindex >= 0)
    typelist = AtomTable_Get(atbl, ITEM_TYPELIST, item->rindex);
  else
    typelist = NULL;
  ret = TypeListItem_To_Vector(typelist, atbl);

  if (item->pindex >= 0)
    typelist = AtomTable_Get(atbl, ITEM_TYPELIST, item->pindex);
  else
    typelist = NULL;
  arg = TypeListItem_To_Vector(typelist, atbl);

  return TypeDesc_Get_Proto(arg, ret);
}

MapItem *MapItem_New(int type, int offset, int size)
{
  MapItem *item = Malloc(sizeof(MapItem));
  item->type   = type;
  item->unused = 0;
  item->offset = offset;
  item->size   = size;
  return item;
}

StringItem *StringItem_New(char *name)
{
  int len = strlen(name);
  StringItem *item = Malloc(sizeof(StringItem) + len + 1);
  item->length = len + 1;
  memcpy(item->data, name, len);
  item->data[len] = 0;
  return item;
}

TypeItem *TypeItem_Primitive_New(char primitive)
{
  TypeItem *item = Malloc(sizeof(TypeItem));
  item->kind = TYPE_BASE;
  item->primitive = primitive;
  return item;
}

TypeItem *TypeItem_Klass_New(int32 pathindex, int32 typeindex)
{
  TypeItem *item = Malloc(sizeof(TypeItem));
  item->kind = TYPE_KLASS;
  item->pathindex = pathindex;
  item->typeindex = typeindex;
  return item;
}

TypeItem *TypeItem_Proto_New(int32 protoindex)
{
  TypeItem *item = Malloc(sizeof(TypeItem));
  item->kind = TYPE_PROTO;
  item->protoindex = protoindex;
  return item;
}

TypeItem *TypeItem_Array_New(int dims, int32 typeindex)
{
  TypeItem *item = Malloc(sizeof(TypeItem));
  item->kind = TYPE_ARRAY;
  item->array.dims = dims;
  item->array.typeindex = typeindex;
  return item;
}

TypeItem *TypeItem_Varg_New(int32 typeindex)
{
  TypeItem *item = Malloc(sizeof(TypeItem));
  item->kind = TYPE_VARG;
  item->varg.typeindex = typeindex;
  return item;
}

TypeListItem *TypeListItem_New(int size, int32 index[])
{
  TypeListItem *item = Malloc(sizeof(TypeListItem) + size * sizeof(int32));
  item->size = size;
  for (int i = 0; i < size; i++) {
    item->index[i] = index[i];
  }
  return item;
}

ProtoItem *ProtoItem_New(int32 rindex, int32 pindex)
{
  ProtoItem *item = Malloc(sizeof(ProtoItem));
  item->rindex = rindex;
  item->pindex = pindex;
  return item;
}

ConstItem *ConstItem_New(int type)
{
  ConstItem *item = Malloc(sizeof(ConstItem));
  item->type = type;
  return item;
}

ConstItem *ConstItem_Int_New(int64 val)
{
  ConstItem *item = ConstItem_New(CONST_INT);
  item->ival = val;
  return item;
}

ConstItem *ConstItem_Float_New(float64 val)
{
  ConstItem *item = ConstItem_New(CONST_FLOAT);
  item->fval = val;
  return item;
}

ConstItem *ConstItem_Bool_New(int val)
{
  ConstItem *item = ConstItem_New(CONST_BOOL);
  item->bval = val;
  return item;
}

ConstItem *ConstItem_String_New(int32 val)
{
  ConstItem *item = ConstItem_New(CONST_STRING);
  item->index = val;
  return item;
}

ConstItem *ConstItem_UChar_New(uchar val)
{
  ConstItem *item = ConstItem_New(CONST_UCHAR);
  item->uch = val;
  return item;
}

LocVarItem *LocVarItem_New(int32 nameindex, int32 typeindex,
                           int32 pos, int index)
{
  LocVarItem *item = Malloc(sizeof(LocVarItem));
  item->nameindex = nameindex;
  item->typeindex = typeindex;
  item->pos = pos;
  item->index = index;
  return item;
}

VarItem *VarItem_New(int32 nameindex, int32 typeindex, int konst)
{
  VarItem *item = Malloc(sizeof(VarItem));
  item->nameindex = nameindex;
  item->typeindex = typeindex;
  item->konst = konst;
  return item;
}

FuncItem *FuncItem_New(int nameindex, int protoindex, int codeindex)
{
  FuncItem *item = Malloc(sizeof(FuncItem));
  item->nameindex = nameindex;
  item->protoindex = protoindex;
  item->codeindex = codeindex;
  return item;
}

CodeItem *CodeItem_New(uint8 *codes, int size)
{
  int sz = sizeof(CodeItem) + sizeof(uint8) * size;
  CodeItem *item = Malloc(sz);
  item->size = size;
  memcpy(item->codes, codes, size);
  Mfree(codes);
  return item;
}

ClassItem *ClassItem_New(int classindex, int superindex)
{
  ClassItem *item = Malloc(sizeof(ClassItem));
  item->classindex = classindex;
  item->superindex = superindex;
  return item;
}

FieldItem *FieldItem_New(int classindex, int nameindex, int typeindex)
{
  FieldItem *item = Malloc(sizeof(FieldItem));
  item->classindex = classindex;
  item->nameindex = nameindex;
  item->typeindex = typeindex;
  return item;
}

MethodItem *MethodItem_New(int classindex, int nameindex, int protoindex,
                           int codeindex)
{
  MethodItem *item = Malloc(sizeof(MethodItem));
  item->classindex = classindex;
  item->nameindex = nameindex;
  item->protoindex = protoindex;
  item->codeindex = codeindex;
  return item;
}

TraitItem *TraitItem_New(int classindex, int traitsindex)
{
  TraitItem *item = Malloc(sizeof(TraitItem));
  item->classindex = classindex;
  item->traitsindex = traitsindex;
  return item;
}

NFuncItem *NFuncItem_New(int classindex, int nameindex, int protoindex)
{
  NFuncItem *item = Malloc(sizeof(NFuncItem));
  item->classindex = classindex;
  item->nameindex = nameindex;
  item->protoindex = protoindex;
  return item;
}

IMethItem *IMethItem_New(int classindex, int nameindex, int protoindex)
{
  IMethItem *item = Malloc(sizeof(IMethItem));
  item->classindex = classindex;
  item->nameindex = nameindex;
  item->protoindex = protoindex;
  return item;
}

static void *VaItem_New(int bsize, int isize, int len)
{
  int32 *data = Malloc(bsize + isize * len);
  data[0] = len;
  return data;
}

static void *Item_Copy(int size, void *src)
{
  void *dest = Malloc(size);
  memcpy(dest, src, size);
  return dest;
}

int StringItem_Get(AtomTable *table, char *str)
{
  int len = strlen(str);
  uint8 data[sizeof(StringItem) + len + 1];
  StringItem *item = (StringItem *)data;
  item->length = len + 1;
  memcpy(item->data, str, len);
  item->data[len] = 0;
  return AtomTable_Index(table, ITEM_STRING, item);
}

int StringItem_Set(AtomTable *table, char *str)
{
  int index = StringItem_Get(table, str);

  if (index < 0) {
    StringItem *item = StringItem_New(str);
    index = AtomTable_Append(table, ITEM_STRING, item, 1);
  }

  return index;
}

int TypeItem_Get(AtomTable *table, TypeDesc *desc);
int TypeItem_Set(AtomTable *table, TypeDesc *desc);

int TypeListItem_Get(AtomTable *table, Vector *desc)
{
  int sz = Vector_Size(desc);
  if (sz <= 0)
    return -1;

  uint8 data[sizeof(TypeListItem) + sizeof(int32) * sz];
  TypeListItem *item = (TypeListItem *)data;
  item->size = sz;

  int index;
  for (int i = 0; i < sz; i++) {
    index = TypeItem_Get(table, Vector_Get(desc, i));
    if (index < 0) return -1;
    item->index[i] = index;
  }

  return AtomTable_Index(table, ITEM_TYPELIST, item);
}

int TypeListItem_Set(AtomTable *table, Vector *desc)
{
  int sz = Vector_Size(desc);
  if (sz <= 0)
    return -1;

  int index = TypeListItem_Get(table, desc);
  if (index < 0) {
    int32 indexes[sz];
    for (int i = 0; i < sz; i++) {
      index = TypeItem_Set(table, Vector_Get(desc, i));
      if (index < 0) {assert(0); return -1;}
      indexes[i] = index;
    }
    TypeListItem *item = TypeListItem_New(sz, indexes);
    index = AtomTable_Append(table, ITEM_TYPELIST, item, 1);
  }
  return index;
}

int ProtoItem_Get(AtomTable *table, int32 rindex, int32 pindex)
{
  ProtoItem item = {rindex, pindex};
  return AtomTable_Index(table, ITEM_PROTO, &item);
}

int ProtoItem_Set(AtomTable *table, TypeDesc *desc)
{
  ProtoDesc *proto = (ProtoDesc *)desc;
  int rindex = TypeListItem_Set(table, proto->ret);
  int pindex = TypeListItem_Set(table, proto->arg);
  int index = ProtoItem_Get(table, rindex, pindex);
  if (index < 0) {
    ProtoItem *item = ProtoItem_New(rindex, pindex);
    index = AtomTable_Append(table, ITEM_PROTO, item, 1);
  }
  return index;
}

int TypeItem_Get(AtomTable *table, TypeDesc *desc)
{
  TypeItem item = {0};
  switch (desc->kind) {
  case TYPE_BASE: {
    BaseDesc *base = (BaseDesc *)desc;
    item.kind = TYPE_BASE;
    item.primitive = base->type;
    break;
  }
  case TYPE_KLASS: {
    KlassDesc *klass = (KlassDesc *)desc;
    int pathindex = -1;
    if (klass->path.str) {
      pathindex = StringItem_Get(table, klass->path.str);
      if (pathindex < 0)
        return pathindex;
    }
    int typeindex = -1;
    if (klass->type.str) {
      typeindex = StringItem_Get(table, klass->type.str);
      if (typeindex < 0)
        return typeindex;
    }
    item.kind = TYPE_KLASS;
    item.pathindex = pathindex;
    item.typeindex = typeindex;
    break;
  }
  case TYPE_PROTO: {
    ProtoDesc *proto = (ProtoDesc *)desc;
    int rindex = TypeListItem_Get(table, proto->ret);
    int pindex = TypeListItem_Get(table, proto->arg);
    item.kind = TYPE_PROTO;
    item.protoindex = ProtoItem_Get(table, rindex, pindex);
    break;
  }
  case TYPE_ARRAY: {
    ArrayDesc *array = (ArrayDesc *)desc;
    item.kind = TYPE_ARRAY;
    item.array.dims = array->dims;
    item.array.typeindex = TypeItem_Get(table, array->base);
    break;
  }
  case TYPE_VARG: {
    VargDesc *varg = (VargDesc *)desc;
    item.kind = TYPE_VARG;
    item.varg.typeindex = TypeItem_Get(table, varg->base);
    break;
  }
  default:
    assert(0);
    break;
  }

  return AtomTable_Index(table, ITEM_TYPE, &item);
}

int TypeItem_Set(AtomTable *table, TypeDesc *desc)
{
  TypeItem *item = NULL;
  int index = TypeItem_Get(table, desc);
  if (index < 0) {
    switch (desc->kind) {
    case TYPE_BASE: {
      BaseDesc *base = (BaseDesc *)desc;
      item = TypeItem_Primitive_New(base->type);
      break;
    }
    case TYPE_KLASS: {
      KlassDesc *klass = (KlassDesc *)desc;
      int pathindex = -1;
      if (klass->path.str) {
        pathindex = StringItem_Set(table, klass->path.str);
        assert(pathindex >= 0);
      }
      int typeindex = -1;
      if (klass->type.str) {
        typeindex = StringItem_Set(table, klass->type.str);
        assert(typeindex >= 0);
      }
      item = TypeItem_Klass_New(pathindex, typeindex);
      break;
    }
    case TYPE_PROTO: {
      int protoindex = ProtoItem_Set(table, desc);
      item = TypeItem_Proto_New(protoindex);
      break;
    }
    case TYPE_ARRAY: {
      ArrayDesc *array = (ArrayDesc *)desc;
      int typeindex = TypeItem_Set(table, array->base);
      item = TypeItem_Array_New(array->dims, typeindex);
      break;
    }
    case TYPE_VARG: {
      VargDesc *varg = (VargDesc *)desc;
      int typeindex = TypeItem_Set(table, varg->base);
      item = TypeItem_Varg_New(typeindex);
      break;
    }
    default:
      assert(0);
      break;
    }
    index = AtomTable_Append(table, ITEM_TYPE, item, 1);
  }
  return index;
}

int ConstItem_Get(AtomTable *table, ConstItem *item)
{
  return AtomTable_Index(table, ITEM_CONST, item);
}

static int codeitem_set(AtomTable *table, uint8 *codes, int size)
{
  CodeItem *item = CodeItem_New(codes, size);
  return AtomTable_Append(table, ITEM_CODE, item, 0);
}

char *mapitem_string[] = {
  "map", "string", "type", "typelist", "proto", "const", "locvar",
  "variable", "function", "code",
  "class", "field", "method",
  "trait", "nfunc", "imeth",
};

int mapitem_length(void *o)
{
  UNUSED_PARAMETER(o);
  return sizeof(MapItem);
}

void mapitem_show(AtomTable *table, void *o)
{
  UNUSED_PARAMETER(table);
  MapItem *item = o;
  Log_Printf("  type:%s\n", mapitem_string[item->type]);
  Log_Printf("  offset:0x%x\n", item->offset);
  Log_Printf("  size:%d\n", item->size);
}

void mapitem_write(FILE *fp, void *o)
{
  fwrite(o, sizeof(MapItem), 1, fp);
}

void mapitem_free(void *o)
{
  Mfree(o);
}

int stringitem_length(void *o)
{
  StringItem *item = o;
  return sizeof(StringItem) + item->length * sizeof(char);
}

void stringitem_write(FILE *fp, void *o)
{
  StringItem *item = o;
  fwrite(o, sizeof(StringItem) + item->length * sizeof(char), 1, fp);
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
  return !strcmp(item1->data, item2->data);
}

void stringitem_show(AtomTable *table, void *o)
{
  UNUSED_PARAMETER(table);
  StringItem *item = o;
  Log_Printf("  length:%d\n", item->length);
  Log_Printf("  string:%s\n", item->data);
}

void stringitem_free(void *o)
{
  Mfree(o);
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
  return hash_uint32(item->pathindex + item->typeindex, 32);
}

int typeitem_equal(void *k1, void *k2)
{
  TypeItem *item1 = k1;
  TypeItem *item2 = k2;
  if (item1->kind != item2->kind)
    return 0;
  if (item1->pathindex != item2->pathindex)
    return 0;
  if (item1->typeindex != item2->typeindex)
    return 0;
  return 1;
}

char *array_string(int dims)
{
  char *data = Malloc(dims * 2 + 1);
  int i = 0;
  while (dims-- > 0) {
    data[i] = '['; data[i+1] = ']';
    i += 2;
  }
  data[i] = '\0';
  return data;
}

void typeitem_show(AtomTable *table, void *o)
{
  TypeItem *item = o;
  //char *arrstr = array_string(item->dims);
  if (item->kind == TYPE_KLASS) {
    StringItem *str;
    if (item->pathindex >= 0) {
      str = AtomTable_Get(table, ITEM_STRING, item->pathindex);
      Log_Printf("  pathindex:%d\n", item->pathindex);
      Log_Printf("  (%s)\n", str->data);
    } else {
      Log_Printf("  pathindex:%d\n", item->pathindex);
    }
    if (item->typeindex >= 0) {
      str = AtomTable_Get(table, ITEM_STRING, item->typeindex);
      Log_Printf("  typeindex:%d\n", item->typeindex);
      //Log_Printf("  (%s%s)\n", arrstr, str->data);
    } else {
      Log_Printf("  typeindex:%d\n", item->typeindex);
    }
  } else if (item->kind == TYPE_BASE) {
    char buf[32];
    TypeDesc *desc = TypeDesc_Get_Base(item->primitive);
    TypeDesc_ToString(desc, buf);
    Log_Printf("  (%s)\n", buf);
  } else if (item->kind == TYPE_ARRAY) {

  }
  //free(arrstr);
}

void typeitem_free(void *o)
{
  Mfree(o);
}

int typelistitem_length(void *o)
{
  TypeListItem *item = o;
  return sizeof(TypeListItem) + item->size * sizeof(int32);
}

void typelistitem_write(FILE *fp, void *o)
{
  TypeListItem *item = o;
  fwrite(o, sizeof(TypeListItem) + item->size * sizeof(int32), 1, fp);
}

uint32 typelistitem_hash(void *k)
{
  TypeListItem *item = k;
  uint32 total = 0;
  for (int i = 0; i < item->size; i++)
    total += item->index[i];
  return hash_uint32(total, 32);
}

int typelistitem_equal(void *k1, void *k2)
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

void typelistitem_show(AtomTable *table, void *o)
{
  UNUSED_PARAMETER(table);
  TypeListItem *item = o;
  TypeItem *type;
  for (int i = 0; i < item->size; i++) {
    Log_Puts("  ---------");
    Log_Printf("  index:%d\n", item->index[i]);
    type = AtomTable_Get(table, ITEM_TYPE, item->index[i]);
    typeitem_show(table, type);
  }
}

void typelistitem_free(void *o)
{
  Mfree(o);
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

void protoitem_show(AtomTable *table, void *o)
{
  UNUSED_PARAMETER(table);
  ProtoItem *item = o;
  Log_Printf("  rindex:%d\n", item->rindex);
  Log_Printf("  pindex:%d\n", item->pindex);
}

void protoitem_free(void *o)
{
  Mfree(o);
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
  case CONST_INT:
    hash = hash_uint32((uint32)item->ival, 32);
    break;
  case CONST_FLOAT:
    hash = hash_uint32((intptr_t)item, 32);
    break;
  case CONST_BOOL:
    hash = hash_uint32((intptr_t)item, 32);
    break;
  case CONST_STRING:
    hash = hash_uint32((uint32)item->index, 32);
    break;
  case CONST_UCHAR:
    hash = hash_uint32((uint32)item->uch.val, 32);
    break;
  default:
    assert(0);
    break;
  }

  return hash;
}

int constitem_equal(void *k1, void *k2)
{
  int res = 0;
  ConstItem *item1 = k1;
  ConstItem *item2 = k2;
  if (item1->type != item2->type)
    return 0;
  switch (item1->type) {
  case CONST_INT:
    res = (item1->ival == item2->ival);
    break;
  case CONST_FLOAT:
    res = (item1->fval == item2->fval);
    break;
  case CONST_BOOL:
    res = (item1 == item2);
    break;
  case CONST_STRING:
    res = (item1->index == item2->index);
    break;
  case CONST_UCHAR:
    res = item1->uch.val == item2->uch.val;
    break;
  default:
    assert(0);
    break;
  }
  return res;
}

void constitem_show(AtomTable *table, void *o)
{
  ConstItem *item = o;
  switch (item->type) {
  case CONST_INT:
    Log_Printf("  int:%lld\n", item->ival);
    break;
  case CONST_FLOAT:
    Log_Printf("  float:%.16lf\n", item->fval);
    break;
  case CONST_BOOL:
    Log_Printf("  bool:%s\n", item->bval ? "true" : "false");
    break;
  case CONST_STRING:
    Log_Printf("  index:%d\n", item->index);
    StringItem *str = AtomTable_Get(table, ITEM_STRING, item->index);
    Log_Printf("  (str:%s)\n", str->data);
    break;
  case CONST_UCHAR:
    Log_Printf("  uchar:%s\n", item->uch.data);
    break;
  default:
    assert(0);
    break;
  }
}

void constitem_free(void *o)
{
  Mfree(o);
}

int locvaritem_length(void *o)
{
  UNUSED_PARAMETER(o);
  return sizeof(LocVarItem);
}

void locvaritem_write(FILE *fp, void *o)
{
  fwrite(o, sizeof(LocVarItem), 1, fp);
}

void locvaritem_show(AtomTable *table, void *o)
{
  LocVarItem *item = o;
  StringItem *str1;
  StringItem *str2;
  TypeItem *type;

  Log_Printf("  nameindex:%d\n", item->nameindex);
  str1 = AtomTable_Get(table, ITEM_STRING, item->nameindex);
  Log_Printf("  (%s)\n", str1->data);
  Log_Printf("  typeindex:%d\n", item->typeindex);
  type = AtomTable_Get(table, ITEM_TYPE, item->typeindex);
  if (type->kind == TYPE_KLASS) {
    str2 = AtomTable_Get(table, ITEM_STRING, type->typeindex);
    if (type->pathindex >= 0) {
      str1 = AtomTable_Get(table, ITEM_STRING, type->pathindex);
      Log_Printf("  (%s.%s)\n", str1->data, str2->data);
    } else {
      Log_Printf("  (%s)\n", str2->data);
    }
  } else {
    Log_Printf("  (%c)\n", type->primitive);
  }
  Log_Printf("  index:%d\n", item->index);
}

void locvaritem_free(void *o)
{
  Mfree(o);
}

int varitem_length(void *o)
{
  UNUSED_PARAMETER(o);
  return sizeof(VarItem);
}

void varitem_write(FILE *fp, void *o)
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
    assert(0);
    str = "";
    break;
  }
  return str;
}

void varitem_show(AtomTable *table, void *o)
{
  VarItem *item = o;
  StringItem *str1;
  StringItem *str2;
  TypeItem *type;

  Log_Printf("  nameindex:%d\n", item->nameindex);
  str1 = AtomTable_Get(table, ITEM_STRING, item->nameindex);
  Log_Printf("  (%s)\n", str1->data);
  Log_Printf("  typeindex:%d\n", item->typeindex);
  type = AtomTable_Get(table, ITEM_TYPE, item->typeindex);
  if (type->kind == TYPE_KLASS) {
    str2 = AtomTable_Get(table, ITEM_STRING, type->typeindex);
    if (type->pathindex >= 0) {
      str1 = AtomTable_Get(table, ITEM_STRING, type->pathindex);
      Log_Printf("  (%s.%s)\n", str1->data, str2->data);
    } else {
      Log_Printf("  (%s)\n", str2->data);
    }
  } else if (type->kind == TYPE_BASE) {
    char buf[32];
    TypeDesc *desc = TypeDesc_Get_Base(type->primitive);
    TypeDesc_ToString(desc, buf);
    Log_Printf("  (%s)\n", buf);
  }
  Log_Printf("  konst:%s\n", item->konst ? "true" : "false");

}

void varitem_free(void *o)
{
  Mfree(o);
}

int funcitem_length(void *o)
{
  UNUSED_PARAMETER(o);
  return sizeof(FuncItem);
}

void funcitem_write(FILE *fp, void *o)
{
  fwrite(o, sizeof(FuncItem), 1, fp);
}

void funcitem_show(AtomTable *table, void *o)
{
  FuncItem *item = o;
  StringItem *str;
  Log_Printf("  nameindex:%d\n", item->nameindex);
  str = AtomTable_Get(table, ITEM_STRING, item->nameindex);
  Log_Printf("  (%s)\n", str->data);
  Log_Printf("  protoindex:%d\n", item->protoindex);
  Log_Printf("  codeindex:%d\n", item->codeindex);
}

void funcitem_free(void *o)
{
  Mfree(o);
}

int codeitem_length(void *o)
{
  CodeItem *item = o;
  return sizeof(CodeItem) + sizeof(uint8) * item->size;
}

void codeitem_write(FILE *fp, void *o)
{
  CodeItem *item = o;
  fwrite(o, sizeof(CodeItem) + sizeof(uint8) * item->size, 1, fp);
}


void codeitem_show(AtomTable *table, void *o)
{
  UNUSED_PARAMETER(table);
  UNUSED_PARAMETER(o);
}

void codeitem_free(void *o)
{
  Mfree(o);
}

int classitem_length(void *o)
{
  UNUSED_PARAMETER(o);
  return sizeof(ClassItem);
}

void classitem_write(FILE *fp, void *o)
{
  fwrite(o, sizeof(ClassItem), 1, fp);
}

void classitem_show(AtomTable *table, void *o)
{
  ClassItem *item = o;
  Log_Printf("  classindex:%d\n", item->classindex);
  if (item->superindex >= 0) {
    Log_Printf("  superinfo:\n");
    TypeItem *type = AtomTable_Get(table, ITEM_TYPE, item->superindex);
    typeitem_show(table, type);
  }
}

void classitem_free(void *o)
{
  Mfree(o);
}

int fielditem_length(void *o)
{
  UNUSED_PARAMETER(o);
  return sizeof(FieldItem);
}

void fielditem_write(FILE *fp, void *o)
{
  fwrite(o, sizeof(FieldItem), 1, fp);
}

void fielditem_show(AtomTable *table, void *o)
{
  FieldItem *item = o;
  Log_Printf("  classindex:%d\n", item->classindex);
  StringItem *id = AtomTable_Get(table, ITEM_STRING, item->nameindex);
  stringitem_show(table, id);
  TypeItem *type = AtomTable_Get(table, ITEM_TYPE, item->typeindex);
  typeitem_show(table, type);
}

void fielditem_free(void *o)
{
  Mfree(o);
}

int methoditem_length(void *o)
{
  UNUSED_PARAMETER(o);
  return sizeof(MethodItem);
}

void methoditem_write(FILE *fp, void *o)
{
  fwrite(o, sizeof(MethodItem), 1, fp);
}

void methoditem_show(AtomTable *table, void *o)
{
  MethodItem *item = o;
  Log_Printf("  classindex:%d\n", item->classindex);
  StringItem *str;
  Log_Printf("  nameindex:%d\n", item->nameindex);
  str = AtomTable_Get(table, ITEM_STRING, item->nameindex);
  Log_Printf("  (%s)\n", str->data);
  Log_Printf("  protoindex:%d\n", item->protoindex);
  Log_Printf("  codeindex:%d\n", item->codeindex);
}

void methoditem_free(void *o)
{
  Mfree(o);
}

int traititem_length(void *o)
{
  UNUSED_PARAMETER(o);
  return sizeof(TraitItem);
}

void traititem_write(FILE *fp, void *o)
{
  fwrite(o, sizeof(TraitItem), 1, fp);
}

void traititem_show(AtomTable *table, void *o)
{
  TraitItem *item = o;
  Log_Printf("  classindex:%d\n", item->classindex);
  TypeItem *type = AtomTable_Get(table, ITEM_TYPE, item->classindex);
  typeitem_show(table, type);
  if (item->traitsindex >= 0) {
    TypeListItem *typelist;
    typelist = AtomTable_Get(table, ITEM_TYPELIST, item->traitsindex);
    typelistitem_show(table, typelist);
  }
}

void traititem_free(void *o)
{
  Mfree(o);
}

int nfuncitem_length(void *o)
{
  UNUSED_PARAMETER(o);
  return sizeof(NFuncItem);
}

void nfuncitem_write(FILE *fp, void *o)
{
  fwrite(o, sizeof(NFuncItem), 1, fp);
}

void nfuncitem_show(AtomTable *table, void *o)
{
  NFuncItem *item = o;
  Log_Printf("  classindex:%d\n", item->classindex);
  StringItem *str;
  Log_Printf("  nameindex:%d\n", item->nameindex);
  str = AtomTable_Get(table, ITEM_STRING, item->nameindex);
  Log_Printf("  (%s)\n", str->data);
  Log_Printf("  protoindex:%d\n", item->protoindex);
}

void nfuncitem_free(void *o)
{
  Mfree(o);
}

int imethitem_length(void *o)
{
  UNUSED_PARAMETER(o);
  return sizeof(IMethItem);
}

void imethitem_write(FILE *fp, void *o)
{
  fwrite(o, sizeof(IMethItem), 1, fp);
}

void imethitem_show(AtomTable *table, void *o)
{
  IMethItem *item = o;
  Log_Printf("  classindex:%d\n", item->classindex);
  StringItem *str;
  Log_Printf("  nameindex:%d\n", item->nameindex);
  str = AtomTable_Get(table, ITEM_STRING, item->nameindex);
  Log_Printf("  (%s)\n", str->data);
  Log_Printf("  protoindex:%d\n", item->protoindex);
}

void imethitem_free(void *o)
{
  Mfree(o);
}

typedef int (*item_length_func)(void *);
typedef void (*item_fwrite_func)(FILE *, void *);
typedef uint32 (*item_hash_func)(void *);
typedef int (*item_equal_func)(void *, void *);
typedef void (*item_show_func)(AtomTable *, void *);
typedef void (*item_free_func)(void *);

struct item_funcs {
  item_length_func length;
  item_fwrite_func write;
  item_hash_func   hash;
  item_equal_func  equal;
  item_show_func   show;
  item_free_func   free;
};

struct item_funcs item_func[ITEM_MAX] = {
  {
    mapitem_length,
    mapitem_write,
    NULL,
    NULL,
    mapitem_show,
    mapitem_free,
  },
  {
    stringitem_length,
    stringitem_write,
    stringitem_hash,
    stringitem_equal,
    stringitem_show,
    stringitem_free
  },
  {
    typeitem_length,
    typeitem_write,
    typeitem_hash,
    typeitem_equal,
    typeitem_show,
    typeitem_free
  },
  {
    typelistitem_length,
    typelistitem_write,
    typelistitem_hash,
    typelistitem_equal,
    typelistitem_show,
    typelistitem_free
  },
  {
    protoitem_length,
    protoitem_write,
    protoitem_hash,
    protoitem_equal,
    protoitem_show,
    protoitem_free
  },
  {
    constitem_length,
    constitem_write,
    constitem_hash,
    constitem_equal,
    constitem_show,
    constitem_free
  },
  {
    locvaritem_length,
    locvaritem_write,
    NULL,
    NULL,
    locvaritem_show,
    locvaritem_free
  },
  {
    varitem_length,
    varitem_write,
    NULL,
    NULL,
    varitem_show,
    varitem_free
  },
  {
    funcitem_length,
    funcitem_write,
    NULL,
    NULL,
    funcitem_show,
    funcitem_free
  },
  {
    codeitem_length,
    codeitem_write,
    NULL,
    NULL,
    codeitem_show,
    codeitem_free
  },
  {
    classitem_length,
    classitem_write,
    NULL,
    NULL,
    classitem_show,
    classitem_free
  },
  {
    fielditem_length,
    fielditem_write,
    NULL,
    NULL,
    fielditem_show,
    fielditem_free
  },
  {
    methoditem_length,
    methoditem_write,
    NULL,
    NULL,
    methoditem_show,
    methoditem_free
  },
  {
    traititem_length,
    traititem_write,
    NULL,
    NULL,
    traititem_show,
    traititem_free
  },
  {
    nfuncitem_length,
    nfuncitem_write,
    NULL,
    NULL,
    nfuncitem_show,
    nfuncitem_free
  },
  {
    imethitem_length,
    imethitem_write,
    NULL,
    NULL,
    imethitem_show,
    imethitem_free
  }
};

uint32 Item_Hash(void *key)
{
  AtomEntry *e = key;
  assert(e->type > 0 && e->type < ITEM_MAX);
  item_hash_func fn = item_func[e->type].hash;
  assert(fn);
  return fn(e->data);
}

int Item_Equal(void *k1, void *k2)
{
  AtomEntry *e1 = k1;
  AtomEntry *e2 = k2;
  assert(e1->type > 0 && e1->type < ITEM_MAX);
  assert(e2->type > 0 && e2->type < ITEM_MAX);
  if (e1->type != e2->type)
    return 0;
  item_equal_func fn = item_func[e1->type].equal;
  assert(fn);
  return fn(e1->data, e2->data);
}

void Item_Free(int type, void *data, void *arg)
{
  UNUSED_PARAMETER(arg);
  assert(type >= 0 && type < ITEM_MAX);
  item_free_func fn = item_func[type].free;
  assert(fn);
  return fn(data);
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
  if (name != NULL)
    strncpy(h->pkgname, name, PKG_NAME_MAX-1);
}

KImage *KImage_New(char *pkgname)
{
  KImage *image = Malloc(sizeof(KImage));
  init_header(&image->header, pkgname);
  image->table = AtomTable_New(ITEM_MAX, Item_Hash, Item_Equal);
  return image;
}

void KImage_Free(KImage *image)
{
  AtomTable_Free(image->table, Item_Free, NULL);
  Mfree(image);
}

int KImage_Add_Integer(KImage *image, int64 val)
{
  ConstItem k = {.type = CONST_INT, .ival = val};
  int index = ConstItem_Get(image->table, &k);
  if (index < 0) {
    ConstItem *item = ConstItem_Int_New(val);
    index = AtomTable_Append(image->table, ITEM_CONST, item, 1);
  }
  return index;
}

int KImage_Add_Float(KImage *image, float64 val)
{
  ConstItem k = {.type = CONST_FLOAT, .fval = val};
  int index = ConstItem_Get(image->table, &k);
  if (index < 0) {
    ConstItem *item = ConstItem_Float_New(val);
    index = AtomTable_Append(image->table, ITEM_CONST, item, 1);
  }
  return index;
}

int KImage_Add_Bool(KImage *image, int val)
{
  ConstItem k = {.type = CONST_BOOL, .bval = val};
  int index = ConstItem_Get(image->table, &k);
  if (index < 0) {
    ConstItem *item = ConstItem_Bool_New(val);
    index = AtomTable_Append(image->table, ITEM_CONST, item, 1);
  }
  return index;
}

int KImage_Add_String(KImage *image, char *val)
{
  int32 idx = StringItem_Set(image->table, val);
  assert(idx >= 0);
  ConstItem k = {.type = CONST_STRING, .index = idx};
  int index = ConstItem_Get(image->table, &k);
  if (index < 0) {
    ConstItem *item = ConstItem_String_New(idx);
    index = AtomTable_Append(image->table, ITEM_CONST, item, 1);
  }
  return index;
}

int KImage_Add_UChar(KImage *image, uchar val)
{
  ConstItem k = {.type = CONST_UCHAR, .uch = val};
  int index = ConstItem_Get(image->table, &k);
  if (index < 0) {
    ConstItem *item = ConstItem_UChar_New(val);
    index = AtomTable_Append(image->table, ITEM_CONST, item, 1);
  }
  return index;
}

void KImage_Add_Var(KImage *image, char *name, TypeDesc *desc, int konst)
{
  int type_index = TypeItem_Set(image->table, desc);
  int name_index = StringItem_Set(image->table, name);
  VarItem *varitem = VarItem_New(name_index, type_index, konst);
  AtomTable_Append(image->table, ITEM_VAR, varitem, 0);
}

void KImage_Add_LocVar(KImage *image, char *name, TypeDesc *desc,
                       int pos, int index)
{
  int typeindex = TypeItem_Set(image->table, desc);
  int nameindex = StringItem_Set(image->table, name);
  LocVarItem *item = LocVarItem_New(nameindex, typeindex, pos, index);
  AtomTable_Append(image->table, ITEM_LOCVAR, item, 0);
}

int KImage_Add_Func(KImage *image, char *name, TypeDesc *proto,
                    uint8 *codes, int size)
{
  int nameindex = StringItem_Set(image->table, name);
  int protoindex = ProtoItem_Set(image->table, proto);
  int codeindex = codeitem_set(image->table, codes, size);
  FuncItem *funcitem = FuncItem_New(nameindex, protoindex, codeindex);
  return AtomTable_Append(image->table, ITEM_FUNC, funcitem, 0);
}

void KImage_Add_Class(KImage *image, char *name, Vector *supers)
{
  KlassDesc tmp = {.kind = TYPE_KLASS, .type.str = name};
  int classindex = TypeItem_Set(image->table, (TypeDesc *)&tmp);
  int superindex = TypeListItem_Set(image->table, supers);
  ClassItem *classitem = ClassItem_New(classindex, superindex);
  AtomTable_Append(image->table, ITEM_CLASS, classitem, 0);
}

void KImage_Add_Field(KImage *image, char *clazz, char *name, TypeDesc *desc)
{
  KlassDesc tmp = {.kind = TYPE_KLASS, .type.str = clazz};
  int classindex = TypeItem_Set(image->table, (TypeDesc *)&tmp);

  int nameindex = StringItem_Set(image->table, name);
  int typeindex = TypeItem_Set(image->table, desc);

  FieldItem *fielditem = FieldItem_New(classindex, nameindex, typeindex);
  AtomTable_Append(image->table, ITEM_FIELD, fielditem, 0);
}

int KImage_Add_Method(KImage *image, char *klazz, char *name, TypeDesc *proto,
                      uint8 *codes, int csz)
{
  KlassDesc tmp = {.kind = TYPE_KLASS, .type.str = klazz};
  int classindex = TypeItem_Set(image->table, (TypeDesc *)&tmp);

  int nameindex = StringItem_Set(image->table, name);
  int protoindex = ProtoItem_Set(image->table, proto);
  int codeindex = codeitem_set(image->table, codes, csz);
  MethodItem *methitem = MethodItem_New(classindex, nameindex, protoindex,
                                        codeindex);
  return AtomTable_Append(image->table, ITEM_METHOD, methitem, 0);
}

void KImage_Add_Trait(KImage *image, char *name, Vector *traits)
{
  KlassDesc tmp = {.kind = TYPE_KLASS, .type.str = name};
  int classindex = TypeItem_Set(image->table, (TypeDesc *)&tmp);
  int traitsindex = TypeListItem_Set(image->table, traits);
  TraitItem *traititem = TraitItem_New(classindex, traitsindex);
  AtomTable_Append(image->table, ITEM_TRAIT, traititem, 0);
}

void KImage_Add_NFunc(KImage *image, char *klazz, char *name, TypeDesc *proto)
{
  int classindex = -1;
  if (klazz != NULL) {
    KlassDesc tmp = {.kind = TYPE_KLASS, .type.str = klazz};
    classindex = TypeItem_Set(image->table, (TypeDesc *)&tmp);
  }
  int nameindex = StringItem_Set(image->table, name);
  int protoindex = ProtoItem_Set(image->table, proto);

  NFuncItem *nfuncitem = NFuncItem_New(classindex, nameindex, protoindex);
  AtomTable_Append(image->table, ITEM_NFUNC, nfuncitem, 0);
}

void KImage_Add_IMeth(KImage *image, char *trait, char *name, TypeDesc *proto)
{
  KlassDesc tmp = {.kind = TYPE_KLASS, .type.str = trait};
  int classindex = TypeItem_Set(image->table, (TypeDesc *)&tmp);
  int nameindex = StringItem_Set(image->table, name);
  int protoindex = ProtoItem_Set(image->table, proto);

  IMethItem *imethitem = IMethItem_New(classindex, nameindex, protoindex);
  AtomTable_Append(image->table, ITEM_IMETH, imethitem, 0);
}

void KImage_Get_Consts(KImage *image, getconstfn func, void *arg)
{
  void *data;
  ConstItem *item;
  int size = AtomTable_Size(image->table, ITEM_CONST);
  for (int i = 0; i < size; i++) {
    item = AtomTable_Get(image->table, ITEM_CONST, i);
    if (item->type == CONST_STRING) {
      StringItem *s = AtomTable_Get(image->table, ITEM_STRING, item->index);
      data = s->data;
    } else {
      data = &item->ival;
    }
    func(item->type, data, i, arg);
  }
}

void KImage_Get_Vars(KImage *image, getvarfn func, void *arg)
{
  VarItem *var;
  StringItem *id;
  TypeItem *type;
  TypeDesc *desc;
  int size = AtomTable_Size(image->table, ITEM_VAR);
  for (int i = 0; i < size; i++) {
    var = AtomTable_Get(image->table, ITEM_VAR, i);
    id = AtomTable_Get(image->table, ITEM_STRING, var->nameindex);
    type = AtomTable_Get(image->table, ITEM_TYPE, var->typeindex);
    desc = TypeItem_To_TypeDesc(type, image->table);
    func(id->data, desc, var->konst, arg);
  }
}

void KImage_Get_LocVars(KImage *image, getlocvarfn func, void *arg)
{
  LocVarItem *locvar;
  StringItem *str;
  TypeItem *type;
  TypeDesc *desc;
  int size = AtomTable_Size(image->table, ITEM_LOCVAR);
  for (int i = 0; i < size; i++) {
    locvar = AtomTable_Get(image->table, ITEM_LOCVAR, i);
    str = AtomTable_Get(image->table, ITEM_STRING, locvar->nameindex);;
    type = AtomTable_Get(image->table, ITEM_TYPE, locvar->typeindex);
    desc = TypeItem_To_TypeDesc(type, image->table);
    func(str->data, desc, locvar->pos, locvar->index, arg);
  }
}

void KImage_Get_Funcs(KImage *image, getfuncfn func, void *arg)
{
  FuncItem *funcitem;
  StringItem *str;
  ProtoItem *proto;
  TypeDesc *desc;
  CodeItem *code;
  int size = AtomTable_Size(image->table, ITEM_FUNC);
  for (int i = 0; i < size; i++) {
    funcitem = AtomTable_Get(image->table, ITEM_FUNC, i);
    str = AtomTable_Get(image->table, ITEM_STRING, funcitem->nameindex);
    code = AtomTable_Get(image->table, ITEM_CODE, funcitem->codeindex);
    proto = AtomTable_Get(image->table, ITEM_PROTO, funcitem->protoindex);
    desc = ProtoItem_To_TypeDesc(proto, image->table);
    if (code != NULL)
      func(str->data, desc, i, ITEM_FUNC, code->codes, code->size, arg);
    else
      func(str->data, desc, i, ITEM_FUNC, NULL, 0, arg);
  }
}

void KImage_Get_NFuncs(KImage *image, getfuncfn func, void *arg)
{
  NFuncItem *nfuncitem;
  StringItem *str;
  ProtoItem *proto;
  TypeDesc *desc;
  int size = AtomTable_Size(image->table, ITEM_NFUNC);
  for (int i = 0; i < size; i++) {
    nfuncitem = AtomTable_Get(image->table, ITEM_NFUNC, i);
    str = AtomTable_Get(image->table, ITEM_STRING, nfuncitem->nameindex);
    proto = AtomTable_Get(image->table, ITEM_PROTO, nfuncitem->protoindex);
    desc = ProtoItem_To_TypeDesc(proto, image->table);
    func(str->data, desc, i, ITEM_NFUNC, NULL, 0, arg);
  }
}

void KImage_Finish(KImage *image)
{
  int size, length = 0, offset;
  MapItem *mapitem;
  void *item;

  offset = image->header.header_size;

  for (int i = 1; i < ITEM_MAX; i++) {
    size = AtomTable_Size(image->table, i);
    if (size > 0) offset += sizeof(MapItem);
  }

  for (int i = 1; i < ITEM_MAX; i++) {
    size = AtomTable_Size(image->table, i);
    if (size > 0) {
      offset += length;
      mapitem = MapItem_New(i, offset, size);
      AtomTable_Append(image->table, ITEM_MAP, mapitem, 0);

      length = 0;
      for (int j = 0; j < size; j++) {
        item = AtomTable_Get(image->table, i, j);
        length += item_func[i].length(item);
      }
    }
  }

  image->header.file_size = offset + length;
  image->header.map_size = AtomTable_Size(image->table, ITEM_MAP);
}

static void __image_write_header(FILE *fp, KImage *image)
{
  fwrite(&image->header, image->header.header_size, 1, fp);
}

static void __image_write_item(FILE *fp, KImage *image, int type, int size)
{
  void *o;
  item_fwrite_func write = item_func[type].write;
  assert(write);
  for (int i = 0; i < size; i++) {
    o = AtomTable_Get(image->table, type, i);
    write(fp, o);
  }
}

static void __image_write_items(FILE *fp, KImage *image)
{
  int size;
  for (int i = 0; i < ITEM_MAX; i++) {
    size = AtomTable_Size(image->table, i);
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
    char *cmd = Malloc(strlen(fmt) + strlen(dir));
    sprintf(cmd, fmt, dir);
    int status = system(cmd);
    assert(!status);
    Mfree(cmd);
    Mfree(dir);
    fp = fopen(path, "w");
  }
  return fp;
}

void KImage_Write_File(KImage *image, char *path)
{
  FILE *fp = open_image_file(path, "w");
  assert(fp);
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

KImage *KImage_Read_File(char *path, int unload)
{
#define LOAD(item) (!(unload & (1 << (item))))

  FILE *fp = fopen(path, "r");
  if (!fp) {
    Log_Printf("error: cannot open %s file\n", path);
    return NULL;
  }

  ImageHeader header;
  int sz = fread(&header, sizeof(ImageHeader), 1, fp);
  if (sz < 1) {
    Log_Printf("error: file %s is not a valid .klc file\n", path);
    fclose(fp);
    return NULL;
  }

  if (header_check(&header) < 0) {
    Log_Printf("error: file %s is not a valid .klc file\n", path);
    return NULL;
  }

  KImage *image = KImage_New(NULL);
  assert(image);
  memcpy(&image->header, &header, sizeof(ImageHeader));

  MapItem mapitems[header.map_size];
  sz = fseek(fp, header.map_offset, SEEK_SET);
  assert(sz == 0);
  sz = fread(mapitems, sizeof(MapItem), header.map_size, fp);
  if (sz < (int)header.map_size) {
    Log_Printf("error: file %s is not a valid .klc file\n", path);
    fclose(fp);
    return NULL;
  }

  MapItem *map;
  for (int i = 0; i < nr_elts(mapitems); i++) {
    map = mapitems + i;
    map = MapItem_New(map->type, map->offset, map->size);
    AtomTable_Append(image->table, ITEM_MAP, map, 0);
  }

  for (int i = 0; i < nr_elts(mapitems); i++) {
    map = mapitems + i;
    sz = fseek(fp, map->offset, SEEK_SET);
    assert(sz == 0);
    switch (map->type) {
    case ITEM_STRING:
      if (LOAD(ITEM_STRING)) {
        StringItem *item;
        uint32 len;
        for (int i = 0; i < map->size; i++) {
          sz = fread(&len, 4, 1, fp);
          assert(sz == 1);
          item = VaItem_New(sizeof(StringItem), sizeof(char), len);
          sz = fread(item->data, sizeof(char) * len, 1, fp);
          assert(sz == 1);
          AtomTable_Append(image->table, ITEM_STRING, item, 1);
        }
      }
      break;
    case ITEM_TYPE:
      if (LOAD(ITEM_TYPE)) {
        TypeItem *item;
        TypeItem items[map->size];
        sz = fread(items, sizeof(TypeItem), map->size, fp);
        assert(sz == map->size);
        for (int i = 0; i < map->size; i++) {
          item = Item_Copy(sizeof(TypeItem), items + i);
          AtomTable_Append(image->table, ITEM_TYPE, item, 1);
        }
      }
      break;
    case ITEM_TYPELIST:
      if (LOAD(ITEM_TYPELIST)) {
        TypeListItem *item;
        uint32 len;
        for (int i = 0; i < map->size; i++) {
          sz = fread(&len, 4, 1, fp);
          assert(sz == 1);
          item = VaItem_New(sizeof(TypeListItem), sizeof(int32), len);
          sz = fread(item->index, sizeof(int32) * len, 1, fp);
          assert(sz == 1);
          AtomTable_Append(image->table, ITEM_TYPELIST, item, 1);
        }
      }
      break;
    case ITEM_PROTO: {
      if (LOAD(ITEM_PROTO)) {
        ProtoItem *item;
        ProtoItem items[map->size];
        sz = fread(items, sizeof(ProtoItem), map->size, fp);
        assert(sz == map->size);
        for (int i = 0; i < map->size; i++) {
          item = Item_Copy(sizeof(ProtoItem), items + i);
          AtomTable_Append(image->table, ITEM_PROTO, item, 1);
        }
      }
      break;
    }
    case ITEM_CONST:
      if (LOAD(ITEM_CONST)) {
        ConstItem *item;
        ConstItem items[map->size];
        sz = fread(items, sizeof(ConstItem), map->size, fp);
        assert(sz == map->size);
        for (int i = 0; i < map->size; i++) {
          item = Item_Copy(sizeof(ConstItem), items + i);
          AtomTable_Append(image->table, ITEM_CONST, item, 1);
        }
      }
      break;
    case ITEM_LOCVAR:
      if (LOAD(ITEM_LOCVAR)) {
        LocVarItem *item;
        LocVarItem items[map->size];
        sz = fread(items, sizeof(LocVarItem), map->size, fp);
        assert(sz == map->size);
        for (int i = 0; i < map->size; i++) {
          item = Item_Copy(sizeof(LocVarItem), items + i);
          AtomTable_Append(image->table, ITEM_LOCVAR, item, 0);
        }
      }
      break;
    case ITEM_VAR:
      if (LOAD(ITEM_VAR)) {
        VarItem *item;
        VarItem items[map->size];
        sz = fread(items, sizeof(VarItem), map->size, fp);
        assert(sz == map->size);
        for (int i = 0; i < map->size; i++) {
          item = Item_Copy(sizeof(VarItem), items + i);
          AtomTable_Append(image->table, ITEM_VAR, item, 0);
        }
      }
      break;
    case ITEM_FUNC:
      if (LOAD(ITEM_FUNC)) {
        FuncItem *item;
        FuncItem items[map->size];
        sz = fread(items, sizeof(FuncItem), map->size, fp);
        assert(sz == map->size);
        for (int i = 0; i < map->size; i++) {
          item = Item_Copy(sizeof(FuncItem), items + i);
          AtomTable_Append(image->table, ITEM_FUNC, item, 0);
        }
      }
      break;
    case ITEM_CODE:
      if (LOAD(ITEM_CODE)) {
        CodeItem *item;
        uint32 len;
        for (int i = 0; i < map->size; i++) {
          sz = fread(&len, 4, 1, fp);
          assert(sz == 1);
          if (len > 0) {
            item = VaItem_New(sizeof(CodeItem), sizeof(uint8), len);
            sz = fread(item->codes, sizeof(uint8) * len, 1, fp);
            assert(sz == 1);
            AtomTable_Append(image->table, ITEM_CODE, item, 0);
          }
        }
      }
      break;
    case ITEM_CLASS:
      if (LOAD(ITEM_CLASS)) {
        ClassItem *item;
        ClassItem items[map->size];
        sz = fread(items, sizeof(ClassItem), map->size, fp);
        assert(sz == map->size);
        for (int i = 0; i < map->size; i++) {
          item = Item_Copy(sizeof(ClassItem), items + i);
          AtomTable_Append(image->table, ITEM_CLASS, item, 0);
        }
      }
      break;
    case ITEM_FIELD:
      if (LOAD(ITEM_FIELD)) {
        FieldItem *item;
        FieldItem items[map->size];
        sz = fread(items, sizeof(FieldItem), map->size, fp);
        assert(sz == map->size);
        for (int i = 0; i < map->size; i++) {
          item = Item_Copy(sizeof(FieldItem), items + i);
          AtomTable_Append(image->table, ITEM_FIELD, item, 0);
        }
      }
      break;
    case ITEM_METHOD:
      if (LOAD(ITEM_METHOD)) {
        MethodItem *item;
        MethodItem items[map->size];
        sz = fread(items, sizeof(MethodItem), map->size, fp);
        assert(sz == map->size);
        for (int i = 0; i < map->size; i++) {
          item = Item_Copy(sizeof(MethodItem), items + i);
          AtomTable_Append(image->table, ITEM_METHOD, item, 0);
        }
      }
      break;
    case ITEM_TRAIT:
      if (LOAD(ITEM_TRAIT)) {
        TraitItem *item;
        TraitItem items[map->size];
        sz = fread(items, sizeof(TraitItem), map->size, fp);
        assert(sz == map->size);
        for (int i = 0; i < map->size; i++) {
          item = Item_Copy(sizeof(TraitItem), items + i);
          AtomTable_Append(image->table, ITEM_TRAIT, item, 0);
        }
      }
      break;
    case ITEM_NFUNC:
      if (LOAD(ITEM_NFUNC)) {
        NFuncItem *item;
        NFuncItem items[map->size];
        sz = fread(items, sizeof(NFuncItem), map->size, fp);
        assert(sz == map->size);
        for (int i = 0; i < map->size; i++) {
          item = Item_Copy(sizeof(NFuncItem), items + i);
          AtomTable_Append(image->table, ITEM_NFUNC, item, 0);
        }
      }
      break;
    case ITEM_IMETH:
      if (LOAD(ITEM_IMETH)) {
        IMethItem *item;
        IMethItem items[map->size];
        sz = fread(items, sizeof(IMethItem), map->size, fp);
        assert(sz == map->size);
        for (int i = 0; i < map->size; i++) {
          item = Item_Copy(sizeof(IMethItem), items + i);
          AtomTable_Append(image->table, ITEM_IMETH, item, 0);
        }
      }
      break;
    default:
      assert(0);
      break;
    }
  }

  fclose(fp);
  return image;
}

void header_show(ImageHeader *h)
{
  Log_Printf("magic:%s\n", (char *)h->magic);
  Log_Printf("version:%d.%d.%d\n", h->version[0] - '0', h->version[1] - '0', 0);
  Log_Printf("header_size:%d\n", h->header_size);
  Log_Printf("endian:0x%x\n", h->endian_tag);
  Log_Printf("map_offset:0x%x\n", h->map_offset);
  Log_Printf("map_size:%d\n\n", h->map_size);
  Log_Puts("--------------------");
}

void AtomTable_Show(AtomTable *table)
{
  void *item;
  int size;
  Log_Printf("map:\n");
  size = AtomTable_Size(table, 0);
  for (int j = 0; j < size; j++) {
    Log_Printf("[%d]\n", j);
    item = AtomTable_Get(table, 0, j);
    item_func[0].show(table, item);
  }

  for (int i = 1; i < table->size; i++) {
    if (i == ITEM_CODE)
      continue;
    size = AtomTable_Size(table, i);
    if (size > 0) {
      Log_Puts("--------------------");
      Log_Printf("%s:\n", mapitem_string[i]);
      for (int j = 0; j < size; j++) {
        Log_Printf("[%d]\n", j);
        item = AtomTable_Get(table, i, j);
        item_func[i].show(table, item);
      }
    }
  }
}

void KImage_Show(KImage *image)
{
  if (!image)
    return;
  Log_Puts("\n------show image--------------\n");
  ImageHeader *h = &image->header;
  header_show(h);
  AtomTable_Show(image->table);
  Log_Puts("\n------end of image------------");
}

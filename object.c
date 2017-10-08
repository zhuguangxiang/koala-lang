
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "object.h"
#include "namei.h"
#include "method_object.h"
#include "module_object.h"

static struct object_metainfo *new_metainfo()
{
  struct object_metainfo *meta = malloc(sizeof(*meta));
  meta->nr_fields      = 0;
  meta->nr_methods     = 0;
  meta->namei_table    = NULL;
  meta->methods_vector = NULL;
  return meta;
}

static void free_metainfo(struct object_metainfo *meta)
{
  /*
  if (meta->namei_table)
    hash_table_destroy();
  if (meta->methods_vector)
    vector_destroy();
  */
  free(meta);
}

static struct object_metainfo *klass_get_metainfo(struct object *ob)
{
  struct klass_object *kob = (struct klass_object *)ob;
  if (!kob->ob_meta)
    kob->ob_meta = new_metainfo();
  return kob->ob_meta;
}

static struct hash_table *meta_get_table(struct object_metainfo *meta)
{
  if (!meta->namei_table)
    meta->namei_table = hash_table_create(namei_hash, namei_equal);
  return meta->namei_table;
}

static struct vector *meta_get_vector(struct object_metainfo *meta)
{
  if (!meta->methods_vector)
    meta->methods_vector = vector_create();
  return meta->methods_vector;
}

static int meta_add_method(struct object_metainfo *meta,
                           struct namei *ni, struct object *ob)
{
  int res = hash_table_insert(meta_get_table(meta), &ni->hnode);
  assert(!res);
  ni->offset = ++meta->nr_methods;
  res = vector_set(meta_get_vector(meta), ni->offset, ob);
  assert(!res);
  return 0;
}

static int meta_add_field(struct object_metainfo *meta, struct namei *ni)
{
  int res = hash_table_insert(meta_get_table(meta), &ni->hnode);
  assert(!res);
  ni->offset = ++meta->nr_fields;
  return 0;
}

void free_klass(struct object *ob)
{
  assert(OB_KLASS(ob) == &klass_klass);
  free(ob);
}

struct klass_object klass_klass = {
  OBJECT_HEAD_INIT(&klass_klass),
  .name  = "Klass",
  .bsize = sizeof(struct klass_object),
};

int klass_get(struct object *ko, struct namei *ni,
              struct object **ob, int *index)
{
  assert(OB_KLASS(ko) == &klass_klass);
  struct object_metainfo *meta = ((struct klass_object *)ko)->ob_meta;
  if (meta == NULL || meta->namei_table == NULL) return -1;
  struct hash_node *hnode = hash_table_find(meta_get_table(meta), ni);
  if (hnode == NULL) return -1;
  assert(meta->methods_vector != NULL);
  struct namei *temp = hnode_to_namei(hnode);
  if (index != NULL) *index = temp->offset;
  if (ob != NULL) *ob = vector_get(meta_get_vector(meta), temp->offset);
  return 0;
}

int klass_add_method(struct object *ko, struct namei *ni, struct object *ob)
{
  assert(OB_KLASS(ko) == &klass_klass);
  assert(OB_KLASS(ob) == &method_klass);
  return meta_add_method(klass_get_metainfo(ko), ni, ob);
}

int klass_add_field(struct object *ko, struct namei *ni)
{
  assert(OB_KLASS(ko) == &klass_klass);
  return meta_add_field(klass_get_metainfo(ko), ni);
}

int klass_add_cfunctions(struct object *ko, struct cfunc_struct *funcs)
{
  struct namei *ni;
  struct object *fo;
  struct cfunc_struct *cf = funcs;
  while (cf->name != NULL) {
    ni = new_func_namei(cf->name, cf->signature, cf->access);
    fo = new_cfunc(cf->func);
    klass_add_method(ko, ni, fo);
    ++cf;
  }

  return 0;
}

static struct cfunc_struct klass_functions[] = {
  {"GetFields", "(V)([Okoala/lang.Field;)", 0, NULL},
  {"GetMethods", "(V)([Okoala/lang.Method;)", 0, NULL},
  {"GetField", "(Okoala/lang.String;)(Okoala/lang.Field;)", 0, NULL},
  {"GetMethod", "(Okoala/lang.String;[Okoala/lang.Klass;)(Okoala/lang.Method;)", 0, NULL},
  {"NewInstance", "(V)(V)", 0, NULL},
  {NULL, NULL, 0, NULL}
};

void init_klass_klass(void)
{
  struct object *ko = (struct object *)&klass_klass;
  klass_add_cfunctions(ko, klass_functions);
  struct object *mo = new_module("koala/lang");
  assert(mo);
  module_add(mo, new_klass_namei("Klass", 0), ko);
}

static void klass_visit(struct hlist_head *head, int size, void *arg)
{
  struct hash_node *hnode;
  struct namei *ni;
  for (int i = 0; i < size; i++) {
    hash_list_for_each(hnode, head) {
      ni = hnode_to_namei(hnode);
      namei_display(ni);
    }
    ++head;
  }
}

void klass_display(struct object *ob)
{
  assert(OB_KLASS(ob) == &klass_klass);
  struct klass_object *ko = (struct klass_object *)ob;

  if (ko->ob_meta && ko->ob_meta->namei_table) {
    hash_table_traverse(meta_get_table(klass_get_metainfo(ob)),
                        klass_visit, NULL);
  } else {
    fprintf(stdout, "no metainfo\n");
  }
}

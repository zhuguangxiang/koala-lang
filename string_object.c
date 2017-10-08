
#include <stdlib.h>
#include <string.h>
#include "string_object.h"

struct object *string_from_cstr(char *cstr)
{
  struct string_object *so = malloc(sizeof(*so));
  init_object_head(so, &string_klass);
  so->length  = strlen(cstr);
  so->str     = malloc(so->length + 1);
  strcpy(so->str, cstr);
  return (struct object *)so;
}

struct klass_object string_klass = {
  OBJECT_HEAD_INIT(&klass_klass),
  .name  = "String",
  .bsize = sizeof(struct string_object),
};

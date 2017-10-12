
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "namei.h"
#include "hash.h"

struct namei *new_namei(char *name, int type, char *signature, int access)
{
  struct namei *ni = malloc(sizeof(*ni));
  assert(ni);
  init_hash_node(&ni->hnode, ni);
  ni->name      = name;
  ni->signature = signature;
  ni->type      = type;
  ni->access    = access;
  ni->offset    = 0;
  return ni;
}

void free_namei(struct namei *ni)
{
  free(ni);
}

uint32_t namei_hash(void *key)
{
  struct namei *ni = key;
  return hash_string(ni->name);
}

/*
  Rules:
    1. The name of function cannot be the same with variable's name.
    2. The name of function can be the same with type's name.
    3. The name of variable can be the same with type's name.
    4. The name between functions, variables and types cannot be the same.
*/
int namei_equal(void *key1, void *key2)
{
  struct namei *ni1 = key1;
  struct namei *ni2 = key2;
  int res = strcmp(ni1->name, ni2->name);
  if (res != 0) return 0;

  if (ni1->type == ni2->type) {
      return 1;
  } else {
    return (namei_is_typedef(ni1) || namei_is_typedef(ni2)) ? 0 : 1;
  }
}

char *ni_type_string(int type)
{
  char *val = NULL;

  switch (type) {
    case NT_VAR:
      val = "var";
      break;
    case NT_FUNC:
      val = "func";
      break;
    case NT_KLASS:
      val = "class";
      break;
    default:
      val = "";
      break;
  }

  return val;
}

char *namei_access_string(struct namei *ni)
{
  int bconst = namei_is_constant(ni);
  int bprivate = namei_is_private(ni);

  if (bconst && bprivate)
    return "const private";
  if (!bconst && bprivate)
    return "private";
  if (bconst && !bprivate)
    return "const public";
  if (!bconst && !bprivate)
    return "public";

  return "";
}

void namei_signature_display(int type, char *signature, char *buf)
{
  if (type == NT_FUNC) {
    char *start = signature + 1;
    char *end = start;
    char *out = buf;
    *out++ = '(';
    while (*end != ')') {
      end++;
    }

    while (start < end) {
      if (*start != 'O' && *start != ';' && *start != '[' && *start != 'V' &&
          *start != 'I') {
        *out++ = *start;
      }
      if (*start == ';' && *(start + 1) != ')') {
        *out++ = ',';
        *out++ = ' ';
      }
      if (*start == '[') {
        *out++ = '[';
        *out++ = ']';
      }
      if (*start == 'I') {
        *out++ = 'i';
        *out++ = 'n';
        *out++ = 't';
        if (*(start+1) != ')') {
          *out++ = ',';
          *out++ = ' ';
        }
      }
      start++;
    }

    *out++ = ')';
    *out++ = ' ';

    start += 2;
    end = start;
    while (*end != ')') {
      end++;
    }

    while (start < end) {
      if (*start != 'O' && *start != ';' && *start != '[' && *start != 'V' &&
          *start != 'I') {
        *out++ = *start;
      }
      if (*start == ';' && *(start + 1) != ')') {
        *out++ = ',';
        *out++ = ' ';
      }
      if (*start == '[') {
        *out++ = '[';
        *out++ = ']';
      }
      if (*start == 'I') {
        *out++ = 'i';
        *out++ = 'n';
        *out++ = 't';
        if (*(start+1) != ')') {
          *out++ = ',';
          *out++ = ' ';
        }
      }
      start++;
    }

    *out = '\0';
  } else if (type == NT_VAR) {

  } else {
    assert(0);
  }
}

void namei_display(struct namei *ni)
{
  char *val = NULL;

  switch (ni->type) {
    case NT_VAR:
      val = "var";
      break;
    case NT_FUNC: {
      char buf[256];
      namei_signature_display(NT_FUNC, ni->signature, buf);
      fprintf(stdout, "%s %s%s\n",
              "func", ni->name, buf);
      break;
    }
    case NT_KLASS:
      val = "class";
      break;
    default:
      val = "";
      break;
  }
}

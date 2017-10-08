
#ifndef _KOALA_NAMEI_H_
#define _KOALA_NAMEI_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"
#include "hash_table.h"

#define NT_VAR      1
#define NT_FUNC     2
#define NT_ALIAS    3
#define NT_STRUCT   4
#define NT_INTF     5
#define NT_MODULE   6
#define NT_KLASS    7

#define ACCESS_PRIVATE  0x01
#define ACCESS_RDONLY   0x02

struct namei {
  struct hash_node hnode;
  char *name;
  char *signature;
  uint8_t type;
  uint8_t access;
  int offset;
};

/* The macros & functions for struct namei */
#define hnode_to_namei(node) container_of(node, struct namei, hnode)

static inline int namei_is_typedef(struct namei *ni)
{
  int ty = ni->type;
  return ((ty == NT_ALIAS) || (ty == NT_STRUCT) || (ty == NT_INTF));
}

#define namei_set_private(ni) ((ni)->access |= ACCESS_PRIVATE)
#define namei_is_private(ni)  ((ni)->access & ACCESS_PRIVATE)
#define namei_is_public(ni)   (!(niob_is_private(ni)))

#define namei_set_constant(ni)  ((ni)->access |= ACCESS_RDONLY)
#define namei_is_constant(ni)   ((ni)->access & ACCESS_RDONLY)

/* Exported symbols */
void free_namei(struct namei *ni);
struct namei *new_namei(char *name, int type, char *signature, int access);
uint32_t namei_hash(void *key);
int namei_equal(void *key1, void *key2);
char *namei_type_string(struct namei *ni);
char *namei_access_string(struct namei *ni);
void namei_display(struct namei *ni);

#define new_var_namei(name, signature, access) \
  new_namei(name, NT_VAR, signature, access)

#define new_func_namei(name, signature, access) \
  new_namei(name, NT_FUNC, signature, access)

#define new_alias_namei(name, signature, access) \
  new_namei(name, NT_ALIAS, signature, access)

#define new_struct_namei(name, access) \
  new_namei(name, NT_STRUCT, NULL, access)

#define new_intf_namei(name, access) \
  new_namei(name, NT_INTF, NULL, access)

#define new_module_namei(name) \
  new_namei(name, NT_MODULE, NULL, 0)

#define new_klass_namei(name, access) \
  new_namei(name, NT_KLASS, NULL, 0)

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_NAMEI_H_ */

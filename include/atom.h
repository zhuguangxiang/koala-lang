/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 *
 * An atom is a null-terminated string cached in internal hashmap.
 * With null-terminated, it's convenient to operate it like c-string.
 */

#ifndef _KOALA_ATOM_H_
#define _KOALA_ATOM_H_

#ifdef  __cplusplus
extern "C" {
#endif

/* New an atom string with null-terminated string. */
char *atom(char *str);

/* New an atom string with length-ed string. */
char *atom_nstring(char *str, int len);

/* Concat 'varc' null-terminated strings into one atom string. */
char *atom_vstring(int vargc, ...);

/* Initialize atom string internal hashmap */
void init_atom(void);

/* Free hashmap and atom string memory */
void fini_atom(void);

#ifdef  __cplusplus
}
#endif

#endif /* _KOALA_ATOM_H_ */

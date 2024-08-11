/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * An Atom is a null-terminated string cached in internal hashmap.
 * With null-terminated, it's convenient to operate it like c-string.
 */

#ifndef _KOALA_ATOM_H_
#define _KOALA_ATOM_H_

#ifdef __cplusplus
extern "C" {
#endif

/* New an atom string with null-terminated string. */
char *atom(char *str);

/* New an atom string with length-ed string. */
char *atom_nstr(char *str, int len);

/* Concat 'argc' null-terminated strings into one atom string. */
char *atom_concat(int argc, ...);

/* Initialize atom string internal hashmap. */
void init_atom(void);

/* Free hashmap and atom string memory. */
void fini_atom(void);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_ATOM_H_ */

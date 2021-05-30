/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
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

/* New an Atom string with null-terminated string. */
char *atom(char *str);

/* New an Atom string with length-ed string. */
char *atom_str(char *str, int len);

/* Concat 'arc' null-terminated strings into one Atom string. */
char *atom_vstr(int argc, ...);

/* Initialize Atom string internal hashmap */
void init_atom(void);

/* Free hashmap and Atom string memory */
void fini_atom(void);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_ATOM_H_ */

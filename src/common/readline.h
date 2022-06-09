/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2018-2022 James <zhuguangxiang@gmail.com>
 */

#ifndef _KOALA_READLINE_H_
#define _KOALA_READLINE_H_

#ifdef __cplusplus
extern "C" {
#endif

/* read line with prompt */
int readline(char *prompt, char *buf, int len);

/* initialize termios */
void init_readline(void);

/* restore termios */
void fini_readline(void);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_READLINE_H_ */

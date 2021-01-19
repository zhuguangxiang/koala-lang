/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangXiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_READLINE_H_
#define _KOALA_READLINE_H_

#ifdef __cplusplus
extern "C" {
#endif

void init_readline(void);
void fini_readline(void);
int readline(char *prompt, char *buf, int len);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_READLINE_H_ */

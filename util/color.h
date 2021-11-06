/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_COLOR_H_
#define _KOALA_COLOR_H_

#ifdef __cplusplus
extern "C" {
#endif

#define BOLD_COLOR(x) "\033[1m" x "\x1b[0m"
#define RED_COLOR(x)  "\x1b[31m" x "\x1b[0m"

#define DBG_COLOR   "\x1b[36mdebug:\x1b[0m "
#define WARN_COLOR  "\x1b[35mwarning:\x1b[0m "
#define ERR_COLOR   "\x1b[31merror:\x1b[0m "
#define PANIC_COLOR "\x1b[31mpanic:\x1b[0m "

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_COLOR_H_ */

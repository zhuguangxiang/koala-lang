/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "util/buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

int main(int argc, char *argv[])
{
    BUF(buf);

    char *s = "Hello, Koala";
    buf_write_str(&buf, s);
    buf_write_str(&buf, s);
    buf_write_str(&buf, s);
    assert(buf.len == 36 && buf.size == 64);

    buf_write_char(&buf, 'O');
    buf_write_char(&buf, 'K');
    assert(buf.len == 38 && buf.size == 64);

    s = BUF_STR(buf);
    assert(!strcmp(s, "Hello, KoalaHello, KoalaHello, KoalaOK"));

    FINI_BUF(buf);
    return 0;
}

#ifdef __cplusplus
}
#endif

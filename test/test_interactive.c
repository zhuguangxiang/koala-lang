/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 */

void kl_cmdline(void);

#ifdef __cplusplus
extern "C" {
#endif

int main(int argc, char *argv[])
{
    kl_cmdline();
    return 0;
}

#ifdef __cplusplus
}
#endif

/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <math.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

void print_float(double d)
{
    char buf[128] = { 0 };
    // if (isnan(d)) {
    //     printf("NAN");
    //     return;
    // }

    sprintf(buf, "%g", d);
    // try to remove the trailing zeros
    size_t ccLen = strlen(buf);
    for (int i = (int)(ccLen - 1); i >= 0; i--) {
        if (buf[i] == '0')
            buf[i] = '\0';
        else
            break;
    }

    printf("%s\n", buf);
}

int main(int argc, char *argv[])
{
    print_float(13.010000000000001);
    print_float(13.949999999999999);
    print_float(13.9400001);
    print_float(13.94000000001);
    print_float(13.94100000);
    print_float(0.1 + 0.2);
    print_float(0.000035);
    print_float(0.0 / 0.0);
    return 0;
}

#ifdef __cplusplus
}
#endif

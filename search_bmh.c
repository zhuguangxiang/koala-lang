
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct _String {
    int len;
    char *data;
} String;

#define length(s)    ((s)->len)
#define charAt(s, i) ((s)->data[i])

static int *computeLastOcc(String *P)
{
    int *lastOcc = malloc(256 * sizeof(int)); // assume ASCII character set

    for (int i = 0; i < 256; i++) {
        lastOcc[i] = -1; // initialize all elements to -1
    }

    for (int i = 0; i < length(P) - 1; i++) {
        // Don't use the last char to compute lastOcc[]
        lastOcc[charAt(P, i)] = i;
    }

    return lastOcc;
}

int BMH(String *T, String *P)
{
    int *lastOcc;
    int i0, j, m, n;

    n = length(T);
    m = length(P);

    lastOcc = computeLastOcc(P); // Find last occurrence of all characters in P

    i0 = 0; // Line P up at T[0]

    while (i0 <= (n - m)) {
        j = m - 1; // Start at the last char in P

        while (charAt(P, j) == charAt(T, i0 + j)) {
            j--; // Check "next" (= previous) character

            if (j < 0) return (i0); // P found !
        }

        /* ==========================================================
       The character in T aligned with P[m-1] is: T[i0+(m-1))]
       Always use character T[i0 + (m-1)] to find the shift
       ========================================================== */
        i0 = i0 + (m - 1) -
             lastOcc[charAt(T, i0 + (m - 1))]; // Use last character: j = (m-1)
        // printf("%d, %d, %d\n", j, i0 + j, i0 + (m - 1));
    }

    return -1; // no match
}

void test(void)
{
    char *tests[] = {
        "abacaxbaccabacbbaabb",
        "abacbb",
        "ababcabcaabcbaabc",
        "ababcabababc",
        "ababcabcaabcbaabc",
        "cabc",
        "HA",
        "HAHAHA",
        "WQN",
        "WQN",
        "ADDAADAADDAAADAAD",
        "DAD",
        "BABABABABABABABABB",
        "BABABB",
        "aaaaaaaaaaaaaaaaaaaaaaaaaaab",
        "aaaaaaaaaaaaaaab",
    };

    int len = sizeof(tests) / sizeof(void *);
    for (int i = 0; i < len; i += 2) {
        String T = { strlen(tests[i]), tests[i] };
        String P = { strlen(tests[i + 1]), tests[i + 1] };
        int r = BMH(&T, &P);
        printf("Found %s at pos: %d\n", P.data, r);
    }
}

typedef struct _Value {
    int tag;
    union {
        int64_t ival;
        double fval;
    };
} Value;

#include <unistd.h>

int main(int argc, char *argv[])
{
    test();
    // int v = __builtin_ffs(0b0010);
    // printf("%d\n", v);
    Value val = { .tag = 100 };
    printf("%d, %ld\n", val.tag, val.ival);

    // float f = 3.14111111111112222;
    // printf("%g\n", f);
    // printf("%.6g\n", 359.013);

    double f = 359.01335;
    printf("%g\n", round(f * 1000.0) / 1000.0);

    char buf[2] = { 0, 0x38 };
    write(1, buf, 2);
    putchar('\n');
    return 0;
}

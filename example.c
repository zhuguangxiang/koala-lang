
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

struct Foo {
    int v1;
    int v2;
};

__thread int foo;

int getFoo()
{
    return foo;
}

typedef struct ShadowStack {
    struct ShadowStack *next;
    int size;
    void **objects[0];
} ShadowStack;

int add(int *a, int b)
{
    return *a + b;
}

void keep(void)
{
    ShadowStack s;
    struct Foo foo;
}
// clang -S -emit-llvm example.c
// llc -filetype=obj -relocation-model=pic sum.ll
int main()
{
    ShadowStack sk;
    // memset(&sk, 0, sizeof(sk));
    void *self = dlopen(NULL, RTLD_LAZY);
    void *ptr = dlsym(self, "pthread_create");
    printf("extern memset : %p\n", ptr);

    FILE *fp;
    char buf[80];
    fp = popen("nm a.out  | grep U | grep dlopen2", "r");
    fgets(buf, sizeof(buf), fp);
    printf("%s\n", buf);
    pclose(fp);
    return 0;
}

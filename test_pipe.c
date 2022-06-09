
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct {
    int a;
    int b;
    int c;
    int d;
} Msg;

void client(char *request_fifo)
{
    int fd = open(request_fifo, O_RDONLY);
    Msg m;
    while (1) {
        read(fd, &m, sizeof(m));
        printf("%d,%d,%d,%d\n", m.a, m.b, m.c, m.d);
        return;
    }
}

int main()
{
    mkfifo("fifo1", 0644);
    pid_t pid = fork();
    if (pid == 0) {
        client("fifo1");
    } else {
        // master process
        int fd = open("fifo1", O_WRONLY);
        Msg m;
        while (1) {
            m.a = 1;
            m.b = 2;
            m.c = 3;
            m.d = 4;
            write(fd, &m, sizeof(m));
            return 0;
        }
    }
    return 0;
}

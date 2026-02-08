
#include <errno.h>
#include <fcntl.h> /* For O_* constants */
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */
#include <unistd.h>

int main()
{
    int fd = shm_open("tempfile", O_RDWR | O_CREAT, 0666);
    ftruncate(fd, 1024);
    printf("fd: %d\n", fd);
    FILE *fp = fdopen(fd, "w");
    if (fp == NULL) {
        printf("%s\n", strerror(errno));
        return -1;
    }
    fwrite("hello", 5, 1, fp);
    fclose(fp);
    // shm_unlink("tempfile");
    return 0;
}

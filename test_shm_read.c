
#include <fcntl.h> /* For O_* constants */
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */
#include <sys/wait.h>
#include <unistd.h>

int main()
{
    pid_t pid = fork();
    if (pid == 0) {
        // sub process
        int r = system("./test_shm_open");
        _exit(r);
    } else {
        int status;
        waitpid(pid, &status, 0);
        int fd = shm_open("tempfile", O_RDWR | O_CREAT, 0600);
        FILE *fp = fdopen(fd, "r");
        char buf[16] = { 0 };
        fread(buf, 16, 1, fp);
        printf("%s\n", buf);
        fclose(fp);
        shm_unlink("tempfile");
    }
}

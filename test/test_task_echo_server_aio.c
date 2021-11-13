/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#define _POSIX_C_SOURCE 200112L
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "task/task.h"
#include "util/common.h"

/*
gcc -g test/test_task_echo_server_aio.c -I./ -lkoala-task -L./build/task
*/
int start_server(const char *host, const char *port)
{
    struct addrinfo hints = {}, *res;
    hints.ai_family = AF_UNSPEC; // use IPv4 or IPv6, whichever
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // fill in my IP for me
    getaddrinfo(host, port, &hints, &res);

    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        return -1;
    }
    if (bind(sockfd, res->ai_addr, res->ai_addrlen)) {
        close(sockfd);
        return -1;
    }
    if (listen(sockfd, 100)) {
        close(sockfd);
        return -1;
    }
    return sockfd;
}

void *client_routine(void *param)
{
    const int sock = PTR2INT(param);
    char buffer[256];
    ssize_t num_read;
    while ((num_read = read(sock, buffer, sizeof(buffer))) > 0) {
        if (!strncmp(buffer, "exit", 4)) {
            write(sock, "bye\n", 4);
            break;
        } else {
            if (num_read != write(sock, buffer, num_read)) {
                break;
            }
        }
    }
    close(sock);
    return NULL;
}

int main(int argc, char *argv[])
{
    init_procs(6);

    const char *host = "127.0.0.1";
    const char *port = "10001";

    const int server_socket = start_server(host, port);
    if (server_socket < 0) {
        printf("failed to create socket. errno: %d\n", errno);
        return errno;
    }

    // Block until a new client appears. Spawn a new fiber for each client.
    int sock;
    while ((sock = accept(server_socket, NULL, NULL)) >= 0) {
        task_create(client_routine, INT2PTR(sock), NULL);
    }

    return 0;
}


#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include "task.h"

int start_server(const char* host, const char* port)
{
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;// use IPv4 or IPv6, whichever
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;// fill in my IP for me
    getaddrinfo(host, port, &hints, &res);

    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(sockfd < 0) {
        return -1;
    }
    if(bind(sockfd, res->ai_addr, res->ai_addrlen)) {
        close(sockfd);
        return -1;
    }
    if(listen(sockfd, 100)) {
        close(sockfd);
        return -1;
    }
    return sockfd;
}

void* client_routine(void* param)
{
    const int sock = (int)(intptr_t)param;
    char buffer[256];
    ssize_t num_read;
    while((num_read = read(sock, buffer, sizeof(buffer))) > 0) {
        if(num_read != write(sock, buffer, num_read)) {
            break;
        }
    }
    close(sock);
    return NULL;
}

int main(int argc, char *argv[])
{
  task_scheduler_init(6);

  const char* host = "127.0.0.1";
  const char* port = "10000";

  const int server_socket = start_server(host, port);
  if(server_socket < 0) {
      printf("failed to create socket. errno: %d\n", errno);
      return errno;
  }

  // Block until a new client appears. Spawn a new fiber for each client.
  int sock;
  task_attr_t attr = {.stacksize = 8196};
  while((sock = accept(server_socket, NULL, NULL)) >= 0) {
      task_t* task = task_create(&attr, &client_routine, (void*)(intptr_t)sock);
      printf("task-%lu created", task->id);
  }

  return 0;
}

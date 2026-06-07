#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#define PORT 8080
#define MAX_EVENTS 2

int main() {
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in client_addr;

    client_addr.sin_port = htons(PORT);
    client_addr.sin_family = AF_INET;

    inet_pton(AF_INET, "127.0.0.1", &client_addr.sin_addr);

    if (connect(client_socket, (struct sockaddr*) &client_addr, sizeof(client_addr)) < 0) {
        perror("Connection error");
        exit(1);
    }

    int ep = epoll_create1(0);

    struct epoll_event ev, events[MAX_EVENTS];

    ev.events = EPOLLIN;
    ev.data.fd = client_socket;
    epoll_ctl(ep, EPOLL_CTL_ADD, client_socket, &ev);

    ev.events = EPOLLIN;
    ev.data.fd = STDIN_FILENO;
    epoll_ctl(ep, EPOLL_CTL_ADD, STDIN_FILENO, &ev);
    
    char buffer[1024] = {0};

    while (1) {
        int eventsCount = epoll_wait(ep, events, MAX_EVENTS, -1);

        for (int i = 0; i < eventsCount; ++i) {
            if (events[i].data.fd == client_socket) {
                int bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

    if (bytes_read <= 0) {
    printf("Connection closed or error\n");
    break;
    }
    buffer[bytes_read] = '\0';
    printf(" -> %s\n", buffer);
            } else if (events[i].data.fd == STDIN_FILENO) {
                if (fgets(buffer, sizeof(buffer), stdin)) {
                    send(client_socket, buffer, strlen(buffer), 0);
                }
            }  
        }
    }

    return 0;
}
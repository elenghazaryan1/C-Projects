#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define MAX_CLIENTS 100

int clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void broadcast_message(char *message, int sender_fd) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; ++i) {

        if (clients[i] != sender_fd) {
            if (send(clients[i], message, strlen(message), 0) < 0) {
                perror("Message send error");
                continue;
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(void *arg) {
    int clientSocket = *((int*) arg);
    char buffer[1024] = {0};

    while(1) {
    int bytesRead = 0;
    
    if ((bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0)) < 0) {
        break;
    }

        buffer[bytesRead] = '\0';
        broadcast_message(buffer, clientSocket);
    }

        pthread_mutex_lock(&clients_mutex);

        for (int i = 0; i < client_count; ++i) {
            if (clients[i] == clientSocket) {
                clients[i] = clients[client_count - 1];
                client_count--;
                close(clientSocket);
                break;
            }
        }

        pthread_mutex_unlock(&clients_mutex);
    
    close(clientSocket);
    return NULL;
}

int main() {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    int* new_sock;

    if (serverSocket < 0) {
        perror("Socket open error");
        exit(1);
    }

    struct sockaddr_in server_addr;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
        perror("Bind error");
        exit(1);
    }

    listen(serverSocket, 10);
    printf("Server running on PORT: %d\n", PORT);

    while (1) {
        int client_fd = accept(serverSocket, NULL, NULL);

        new_sock = (int*)malloc(sizeof(int));

        *new_sock = client_fd;

        pthread_mutex_lock(&clients_mutex);

        clients[client_count++] = client_fd;

        pthread_mutex_unlock(&clients_mutex);

        pthread_t tid;

        pthread_create(&tid, NULL, handle_client, new_sock);

        pthread_detach(tid);
    }

    return 0;
}
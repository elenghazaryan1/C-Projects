#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#define PORT 8080
#define SIZE 1024

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
typedef struct {
    char name[50]; 
} Product;

void* handle_client(void* arg);

int main() {
    char buffer[SIZE];

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_fd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if((bind(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr))) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if((listen(socket_fd, SOMAXCONN)) < 0) {
        perror("listening failed");
        exit(EXIT_FAILURE);
    }

    while(1) {
        int client_fd = accept(socket_fd, NULL, NULL);
        if(client_fd < 0) {
            perror("client accepting failed");
            exit(EXIT_FAILURE);
        }
        int *n = malloc(sizeof(int));
        *n = client_fd;

        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_client, (void*)n) != 0) {
            printf("thread creation failed\n");
        }  
        pthread_detach(thread);
    }

    return 0;
}

void* handle_client(void* arg) {
    
    FILE* file_fd;
    int* conn = (int*)arg;
    char buffer[SIZE];
    char line[SIZE];

    int readBuf = recv(*conn, buffer, sizeof(buffer) - 1, 0);

    if (readBuf <= 0) {
        perror("receiving failed");
        close(*conn);
        free(arg);
        return NULL;
    }
    buffer[readBuf] = '\0';
    buffer[strcspn(buffer, "\r\n")] = 0;

    if(strncmp(buffer, "POST", 4) == 0) {
        char* product_name = buffer + strlen("POST");
        while (*product_name == ' ') product_name++;
      
        int name_len = strlen(product_name);
        if (strlen(product_name) == 0 || name_len >= 50) {
            send(*conn, "Invalid product name length\n", 28, 0);
            close(*conn); 
            free(arg);   
            return NULL; 
        }
        
        Product new_prod;
        memset(&new_prod, 0, sizeof(Product));
        strncpy(new_prod.name, product_name, sizeof(new_prod.name) - 1);

        pthread_mutex_lock(&mutex);
        file_fd = fopen("./product.txt", "a");
        if (file_fd == NULL) {
            pthread_mutex_unlock(&mutex);
            send(*conn, "DB error", 8, 0);
            close(*conn);
            free(arg);
            return NULL;
        }

        fprintf(file_fd, "%s\n", new_prod.name);  
        fflush(file_fd);
        fclose(file_fd);
        send(*conn, "Product added successfully\n", 27, 0);
        pthread_mutex_unlock(&mutex);
        close(*conn);     
        free(arg);
        return NULL;

    } else if(strncmp(buffer, "GET", 3) == 0) {
        char* par = buffer + 3;

        while(*par == ' ') {
            par++;
        }

        pthread_mutex_lock(&mutex);
        file_fd = fopen("./product.txt", "r");
        if (file_fd == NULL) {
            pthread_mutex_unlock(&mutex);
            send(*conn, "DB error", 8, 0);
            close(*conn);
            free(arg);
            return NULL;
        }
        
        if(strlen(par) == 0) {
            char response[SIZE] = {0};
            char line[50];
            while(fgets(line, sizeof(line), file_fd) != NULL) {
                line[strcspn(line, "\r\n")] = 0;
                strncat(response, line, SIZE - strlen(response) - 1);
                strncat(response, "\n", SIZE - strlen(response) - 1);
            }
            
            if(strlen(response) == 0) {
                send(*conn, "There are no products.\n", 23, 0);
            } else {
                send(*conn, response, strlen(response), 0);
            }
        } else {    
            char line[50];
            int found = 0;
            while(fgets(line, sizeof(line), file_fd) != NULL) {
                line[strcspn(line, "\r\n")] = 0;
                if(strcmp(line, par) == 0) {
                    found = 1;
                    break;
                }
            }
            if(found) {
                char response[50];
                snprintf(response, sizeof(response), "%s\n", line);
                send(*conn, response, strlen(response), 0);
            } else {
                send(*conn, "Not found", 9, 0);
            }
        }

        fclose(file_fd);
        pthread_mutex_unlock(&mutex);

        close(*conn);
        free(arg);
        return NULL;

    } else {
        send(*conn, "Unknown command\n", 16, 0);
        close(*conn);
        free(arg);
        return NULL;
    }
}
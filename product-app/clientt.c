#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#define PORT 8080
#define SIZE 1024

int main() {

    char buffer[SIZE];
    while(1) {
            printf("Enter command (or 'quit' to exit)\n");  
        
        if((fgets(buffer, sizeof(buffer), stdin)) == NULL) {
            perror("Reading from input failed.\n");
            exit(EXIT_FAILURE);
        }
        buffer[strcspn(buffer, "\r\n")] = 0;

        if(strcmp(buffer, "quit") == 0) {
            break;
        }
    int socket_fd  = socket(AF_INET,SOCK_STREAM,0);
        if(socket_fd < 0 ) {
            perror("socket creation failed");
            exit(EXIT_FAILURE);
        }


        struct sockaddr_in client_addr;

        client_addr.sin_family = AF_INET;
        client_addr.sin_port = htons(PORT);

        inet_pton(AF_INET,"127.0.0.1",&client_addr.sin_addr);

        if((connect(socket_fd,(struct sockaddr*)&client_addr,sizeof(client_addr))) < 0) {
            perror("connecting to server failed");
            exit(EXIT_FAILURE);
        }
        
        
        if((send(socket_fd,buffer,strlen(buffer),0)) < 0) {
            perror("sending failed");
            close(socket_fd);
            exit(EXIT_FAILURE);
        }
        memset(buffer,0,SIZE);
        int rec = recv(socket_fd,buffer,sizeof(buffer) - 1, 0);
        if(rec < 0) {
            printf("Receiving failed\n");
            exit(EXIT_FAILURE);
        } else if(rec  == 0){
            printf("Server closed connection\n");
        } else {
            buffer[rec] = '\0';
            printf("SERVER RESPONSE: %s\n", buffer);
        }
    
        close(socket_fd);
    }
        return 0;
}

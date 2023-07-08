#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>

#define PORT 5001
#define SIZE 1024

char FILE_FOUND[20] = "FILE_FOUND";
char FILE_NOT_FOUND[20] = "FILE_NOT_FOUND";
char GET[20] = "GET";
char UPDATE[20] = "UPDATE";
char START[20] = "START_SENDING";


typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;

typedef struct manifest_header {
    char method[20];
    char name[100];
} manifest_header;

void handleGet(int clientSockFD, char* filename) {
     /* 
        custom search file impl
     */

    // "./manifest-files/filename.txt"
    char buffer[SIZE];
    char fileDir[100] = "./manifest-files/";
    char* fileNamePtr = strcat(fileDir, filename);

    FILE* fp = fopen(fileNamePtr, "r");

    if(fp == NULL) {
        char alternateFileDir[100] = "./receive/";
        char* alternateFileNamePtr = strcat(alternateFileDir, filename);
        fp = fopen(alternateFileNamePtr, "r");
    }

    if(fp == NULL) {
        send(clientSockFD, FILE_NOT_FOUND, strlen(FILE_NOT_FOUND), 0);
        printf("\n%s not found\n", filename);
    } else {
        printf("\n%s found\n", filename);

        send(clientSockFD, FILE_FOUND, strlen(FILE_FOUND), 0);

        int recvCount = recv(clientSockFD, buffer, SIZE, 0);
        buffer[recvCount] = '\0';

        if(strcmp(buffer, START) == 0) {
            memset(buffer, '\0', SIZE);

            printf("\nSending %s manifest file...\n", filename);

            while(fgets(buffer, SIZE, fp) != NULL) {
                send(clientSockFD, buffer, strlen(buffer), 0);
            }

            printf("Successfully sent the %s manifest file...\n", filename);
        }
    }

    fclose(fp);
}

void handleUpdate(int clientSockFD, char* filename) {

    // "./manifest-files/filename.txt"
    char buffer[SIZE];
    char fileDir[100] = "./manifest-files/";
    char* fileNamePtr = strcat(fileDir, filename);

    FILE* fp = fopen(fileNamePtr, "w");

    if(fp == NULL) {
        char alternateFileDir[100] = "./receive/";
        char* alternateFileNamePtr = strcat(alternateFileDir, filename);
        fp = fopen(alternateFileNamePtr, "w");
    }

    if(fp == NULL) {
        send(clientSockFD, FILE_NOT_FOUND, strlen(FILE_NOT_FOUND), 0);
        printf("\n%s not found\n", filename);
    } else {
        printf("\n%s found\n", filename);

        send(clientSockFD, FILE_FOUND, strlen(FILE_FOUND), 0);

        printf("\nReceiving updated %s manifest file...\n", filename);

        int recvCount;
        while( (recvCount = recv(clientSockFD, buffer, SIZE, 0)) > 0) {
            buffer[recvCount] = '\0';
            fputs(buffer, fp);
            memset(buffer, '\0', SIZE);
        }
        
        printf("Successfully received the updated %s manifest file...\n", filename);
    }

    fclose(fp);
}


void handleNewConnection(int sockFD) {
    sockaddr_in clientSockAddr;
    int clientSockAddrLen = sizeof(clientSockAddr);
    int clientSockFD = accept(sockFD, (sockaddr*) &clientSockAddr, &clientSockAddrLen);
            
    if(clientSockFD < 0) {
        printf("\nFailed to establish new connection\n");
        return;
    }

    manifest_header manifestHeader;
    int recvCount = recv(clientSockFD, &manifestHeader, sizeof(manifestHeader), 0);
    
    if(strcmp(manifestHeader.method, GET) == 0) {
        handleGet(clientSockFD, manifestHeader.name);
    } else if(strcmp(manifestHeader.method, UPDATE) == 0) {
        handleUpdate(clientSockFD, manifestHeader.name);
    }

    close(clientSockFD);
}

int main() {
    int sockFD = socket(AF_INET, SOCK_STREAM, 0);    

    int opt = 1;
    int status = setsockopt(sockFD, SOL_SOCKET, SO_REUSEPORT | SO_REUSEADDR, &opt, sizeof(opt));

    if(status < 0) {
        printf("\nFailed to set socket options\n");
        exit(EXIT_FAILURE);
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    status = bind(sockFD, (sockaddr*) &serverAddress, sizeof(serverAddress));

    if(status < 0) {
        printf("\nFailed to bind socket\n");
        exit(EXIT_FAILURE);
    }

    status = listen(sockFD, 5);

    if(status < 0) {
        printf("\nFailed to listen from the socket\n");
        exit(EXIT_FAILURE);
    }

    while(1) {
        printf("\nWaiting for the client to connect...\n");
        handleNewConnection(sockFD);
    }

    return 0;
}
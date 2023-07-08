#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>

#define PORT 5000
#define ANCHOR_NODE_PORT 5001
#define SIZE 1024

char FILE_FOUND[20] = "FILE_FOUND";
char FILE_NOT_FOUND[20] = "FILE_NOT_FOUND";
char GET[20] = "GET";
char UPDATE[20] = "UPDATE";
char START[20] = "START_SENDING";

typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;

char* getManifestFileInput() {
    char* filename = (char*) malloc(100 * sizeof(char));
    printf("\nEnter filename: ");
    scanf("%s", filename);

    return filename;
}

char* getIpAddress() {
    system("ifconfig | grep -w inet > ipaddr.txt");

    char* ipAddressFileName = "ipaddr.txt";

    FILE* fp = fopen(ipAddressFileName, "r");

    char buffer[SIZE];
    char* ipAddress = (char*) malloc(50 * sizeof(char));

    while(fgets(buffer, SIZE, fp) != NULL) {
        char* cPtr = buffer;

        while(*cPtr == ' ') {
            cPtr++;
        }

        char* leftTrimBuffer = cPtr;

        while(*cPtr != ' ' && *cPtr != '\0') {
            cPtr++;
        }
        *cPtr = '\0';

        if(strcmp(leftTrimBuffer, "inet") == 0) {
            cPtr++;
            char* ipAddrPtr = cPtr;

            while(*cPtr != ' ' && *cPtr != '\0') {
                cPtr++;
            }
            *cPtr = '\0';

            if(strcmp(ipAddrPtr, "127.0.0.1") != 0) {
                strcpy(ipAddress, ipAddrPtr);
                break;
            }
        }

    }

    fclose(fp);
    remove(ipAddressFileName);

    return ipAddress;
}

void handleUpload(int sockFD) {
    sockaddr_in clientSockAddr;
    int clientSockAddrLen = sizeof(clientSockAddr);

    int clientSockFD = accept(sockFD, (sockaddr*) &clientSockAddr, &clientSockAddrLen);
    
    if(clientSockFD < 0) {
        printf("\nFailed to accept from the socket\n");
        exit(EXIT_FAILURE);
    }

    char buffer[SIZE];

    int recvCount = recv(clientSockFD, buffer, SIZE, 0);
    buffer[recvCount] = '\0';

    char chunkFileName[100];
    strcpy(chunkFileName, buffer);

    char chunkDir[100] = "./chunk-files/";
    char* chunkFilePath = strcat(chunkDir, chunkFileName);

    FILE* fp = fopen(chunkFilePath, "r");

    memset(buffer, '\0', SIZE);

    if(fp == NULL) {
        char alternateChunkDir[100] = "./receive/chunks/";
        char* alternateChunkFilePath = strcat(alternateChunkDir, chunkFileName);

        fp = fopen(alternateChunkFilePath, "r");
    }


    if(fp == NULL) {
        send(clientSockFD, FILE_NOT_FOUND, strlen(FILE_NOT_FOUND), 0);
        printf("\n%s not found\n", chunkFileName);
    } else {

        send(clientSockFD, FILE_FOUND, strlen(FILE_FOUND), 0);
        printf("\n%s found\n", chunkFileName);
    
        recvCount = recv(clientSockFD, buffer, SIZE, 0);
        buffer[recvCount] = '\0';

        if(strcmp(buffer, START) == 0) {
            memset(buffer, '\0', SIZE);
            
            printf("Sending %s\n", chunkFileName);
            
            while(fgets(buffer, SIZE, fp) != NULL) {
                send(clientSockFD, buffer, strlen(buffer), 0);
                memset(buffer, '\0', SIZE);
            }
        
            printf("Successfully sent %s\n", chunkFileName);
        }
    }

    fclose(fp);
    close(clientSockFD);
}

void handleServerConnection() {
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
        handleUpload(sockFD);
    }

    close(sockFD);
}

void handleUpdateManifestFile(char* manifestFile);

void handleDownload(char* manifestFile) {
    char buffer[SIZE];
    int status;
    int recvCount;

    if(manifestFile == NULL) {
        printf("\nFile not found...\n");
        exit(EXIT_FAILURE);
    }
    
    char fileDir[100] = "./receive/";
    char* fileNamePtr = strcat(fileDir, manifestFile);
    FILE* fp = fopen(fileNamePtr, "r");

    memset(buffer, '\0', SIZE);
    int isChunk = 0;

    int chunkCount = 1;
    char chunkArr[100][100];
    int chunkArrCount = 0;

    char outputFileDir[100] = "./receive/output-";
    char* outputFile = strcat(outputFileDir, manifestFile);
    FILE* outFp = fopen(outputFile, "w");

    int hasDataSaved = 0;

    while(fgets(buffer, SIZE, fp) != NULL) {
        char lineStr[SIZE];
        strcpy(lineStr, buffer);
        char* lineStrPtr = lineStr;

        char orgBuffer[SIZE];
        strcpy(orgBuffer, buffer);

        char* line = buffer;

        char chunkFileName[100];
        char chunkFileNameWithExt[100];

        char* cPtr = line;

        while(*cPtr != ' ' && *cPtr != '\0') {
            cPtr++;
        }

        *cPtr = '\0';
        strcpy(chunkFileName, line);
        strcpy(chunkFileNameWithExt, chunkFileName);
        strcat(chunkFileNameWithExt, ".txt");

        if(strcmp(chunkFileName, "filename") == 0) {
            // default chunk line
            fputs(lineStr, outFp);

            memset(buffer, '\0', SIZE);
            continue;
        }


        line = orgBuffer;

        while(1) {
            char* cPtr = line;

            while(*cPtr != ' ' && *cPtr != '\0') {
                cPtr++;
            }

            *cPtr = '\0';

            if(strncmp(line, chunkFileName, strlen(chunkFileName)) == 0) {
                isChunk = 1;
            }
            if(isChunk && strncmp(line, chunkFileName, strlen(chunkFileName)) != 0) {
                /* ipAddr of chunk */
                int chunkSockFD = socket(AF_INET, SOCK_STREAM, 0);
                
                sockaddr_in chunkAddr;
                chunkAddr.sin_family = AF_INET;
                chunkAddr.sin_port  = htons(PORT);
                chunkAddr.sin_addr.s_addr  = inet_addr(line);


                status = connect(chunkSockFD, (sockaddr*) &chunkAddr, sizeof(chunkAddr));

                if(status < 0) {
                    printf("\nFailed to connect to the %s ip address %s\n", chunkFileName, line);
                    printf("Fetching from the next ip address if exists...\n");
                    ++cPtr;

                    if(*cPtr != '\0') {
                        line = cPtr;
                    } else {
                        break;
                    }
                    continue;
                }

                char chunkBuffer[SIZE];

                send(chunkSockFD, chunkFileNameWithExt, strlen(chunkFileNameWithExt), 0);

                recvCount = recv(chunkSockFD, chunkBuffer, SIZE, 0);
                chunkBuffer[recvCount] = '\0';

                if(strcmp(chunkBuffer, FILE_NOT_FOUND) == 0) {
                    printf("\n%s not found at ip address %s\n", chunkFileName, line);
                    printf("Fetching from the next ip address if exists...\n");
                    
                    ++cPtr;

                    if(*cPtr != '\0') {
                        line = cPtr;
                    } else {
                        break;
                    }
                    continue;
                } else {
                    printf("\n%s found at ip address %s\n", chunkFileName, line);
                }

                printf("\nDownloading %s from ip address: %s\n", chunkFileName, line);

                strcpy(chunkArr[chunkArrCount++], chunkFileNameWithExt);

                memset(chunkBuffer, '\0', SIZE);
                
                char tempDir[100] = "./receive/chunks/temp-";
                char* chunkDir = strcat(tempDir, chunkFileNameWithExt);

                FILE* fp = fopen(chunkDir, "w");

                int hasDataReceived = 0;

                send(chunkSockFD, START, strlen(START), 0);

                while( (recvCount = recv(chunkSockFD, chunkBuffer, SIZE, 0)) > 0 ) {
                    chunkBuffer[recvCount] = '\0';

                    fputs(chunkBuffer, fp);
                    memset(chunkBuffer, '\0', SIZE);
                    hasDataReceived = 1;
                    hasDataSaved = 1;
                }
                
                fclose(fp);
                close(chunkSockFD);

                char savedDir[100] = "./receive/chunks/";
                rename(chunkDir, strcat(savedDir, chunkFileNameWithExt));

                printf("Successfully downloaded %s from ip address: %s\n", chunkFileName, line);

                if(hasDataReceived) {
                
                    lineStrPtr += strlen(lineStr);
                    lineStrPtr--;
                    if(*lineStrPtr == '\n') {
                        *lineStrPtr = '\0';
                    }

                    // appending current ip address to the chunk line
                    
                    strcat(lineStr, " "); 
                    strcat(lineStr, getIpAddress()); 
                    lineStrPtr = lineStr;
                    lineStrPtr += strlen(lineStr);
                    *lineStrPtr = '\n';
                    lineStrPtr++;
                    *lineStrPtr = '\0';

                    break;
                }
            }

            ++cPtr;

            if(*cPtr != '\0') {
                line = cPtr;
            } else {
                break;
            }
        }
        
        // default chunk line
        fputs(lineStr, outFp);

        isChunk = 0;
        memset(buffer, '\0', SIZE);
    }
    fclose(outFp);
    fclose(fp);

    if(hasDataSaved) {
        int pid = fork();
        if(pid == 0) {
            char c;
            scanf("%c", &c);
            
            char outFileDir[100] = "./receive/complete/";
            char* outFilePath = strcat(outFileDir, manifestFile);
            FILE* outFp = fopen(outFilePath, "w");
            
            printf("\nMerging chunks:\n");

            for(int i = 0; i < chunkArrCount; i++) {
                printf("%s\n", chunkArr[i]);

                char inFileDir[100] = "./receive/chunks/";
                char* inFilePath = strcat(inFileDir, chunkArr[i]);
                FILE* inFp = fopen(inFilePath, "r");
                
                char chunkBuffer[SIZE];

                while(fgets(chunkBuffer, SIZE, inFp) != NULL) {
                    fputs(chunkBuffer, outFp);
                    memset(chunkBuffer, '\0', SIZE);
                }

                fclose(inFp);

            }
            fclose(outFp);

            printf("\nSuccessfully merged the chunk files and saved the merged file at ./receive/complete/%s\n", manifestFile);
            
            char temp[100] = "output-";
            handleUpdateManifestFile(strcat(temp, manifestFile));

        } else {
            int status;
            int childPid = wait(&status);
            printf("\nChild PID: %d handleUpdateManifestFile terminated with exit status: %d\n", childPid, WEXITSTATUS(status));
        }
    }
}

typedef struct manifest_header {
    char method[20];
    char name[100];
} manifest_header;

int createTCPSocket() {
    return socket(AF_INET, SOCK_STREAM, 0);
}

int establishTCPConnection(int sockFD, int port, char* ipAddress) {
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = inet_addr(ipAddress);

    return connect(sockFD, (sockaddr*) &serverAddress, sizeof(serverAddress));
}

void handleUpdateManifestFile(char* manifestFile) {
    int sockFD = createTCPSocket();
    int status = establishTCPConnection(sockFD, ANCHOR_NODE_PORT, "127.0.0.1");

    if(status < 0) {
        printf("\nFailed to connect to the anchor node socket\n");
        return;
    }

    char orgFileName[100];
    strcpy(orgFileName, manifestFile);

    char* filePtr = manifestFile;

    while(*filePtr != '-') {
        filePtr++;
    }
    *filePtr = '\0';
    filePtr++;

    manifest_header manifestHeader;
    strcpy(manifestHeader.method, UPDATE);
    strcpy(manifestHeader.name, filePtr);

    send(sockFD, &manifestHeader, sizeof(manifestHeader), 0);

    char buffer[SIZE];

    int recvCount = recv(sockFD, buffer, SIZE, 0);
    buffer[recvCount] = '\0';

    if(strcmp(buffer, FILE_NOT_FOUND) == 0) {
        printf("\n%s not found at anchor node...\n", filePtr);
        close(sockFD);
        return;
    } else {
        printf("\n%s manifest file found at anchor node...\n", filePtr);
    }

    char fileDir[100] = "./receive/";
    char* filePath = strcat(fileDir, orgFileName);
    FILE* fp = fopen(filePath, "r");
        
    printf("\nSending updated %s manifest file to anchor node...\n", filePtr);

    while(fgets(buffer, SIZE, fp) != NULL) {
        send(sockFD, buffer, strlen(buffer), 0);
        memset(buffer, '\0', SIZE);
    }

    printf("Successfully sent the updated %s manifest file to anchor node...\n", filePtr);
    
    fclose(fp);
    close(sockFD);
    char oldFile[100] = "./receive/";
    char newFile[100] = "./receive/";
    rename(strcat(oldFile, orgFileName), strcat(newFile, filePtr));
}

void handleGetManifestFile(char* manifestFile) {
    int sockFD = createTCPSocket();
    int status = establishTCPConnection(sockFD, ANCHOR_NODE_PORT, "127.0.0.1");

    if(status < 0) {
        printf("\nFailed to connect to the anchor node socket\n");
        return;
    }

    manifest_header manifestHeader;
    strcpy(manifestHeader.method, GET);
    strcpy(manifestHeader.name, manifestFile);

    send(sockFD, &manifestHeader, sizeof(manifestHeader), 0);

    char buffer[SIZE];

    int recvCount = recv(sockFD, buffer, SIZE, 0);
    buffer[recvCount] = '\0';

    if(strcmp(buffer, FILE_NOT_FOUND) == 0) {
        printf("\n%s not found at anchor node...\n", manifestFile);
        return;
    } else {
        printf("\n%s found at anchor node...\n", manifestFile);
    }

    char fileDir[100] = "./receive/temp-";
    char* filePath = strcat(fileDir, manifestFile);
    FILE* fp = fopen(filePath, "w");
    
    memset(buffer, '\0', SIZE);

    send(sockFD, START, strlen(START), 0);

    printf("\nDownloading %s manifest file from anchor node...\n", manifestFile);

    while((recvCount = recv(sockFD, buffer, SIZE, 0)) > 0) {
        buffer[recvCount] = '\0';
        fputs(buffer, fp);
        memset(buffer, '\0', SIZE);
    }

    printf("Successfully downloaded %s manifest file from anchor node...\n", manifestFile);

    fclose(fp);
    close(sockFD);
    
    char savedDir[100] = "./receive/";
    rename(filePath, strcat(savedDir, manifestFile));
}

int main() {
    int pid1 = fork();
    int pid2;

    if(pid1 == 0) {
        printf("\nPID: %d handleServerConnection\n", getpid());
        handleServerConnection();
    } else {
        pid2 = fork();

        if(pid2 == 0) {
            char* manifestFile = getManifestFileInput();
            handleGetManifestFile(manifestFile);
            
            printf("\nPID: %d handleDownload\n", getpid());
            handleDownload(manifestFile);
        } else {
            int status;
            for(int i = 0; i < 2; i++) {
                int childPid = wait(&status);
            }
        }
    }
    
    return 0;
}
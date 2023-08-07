#include <stdio.h>
#include <sys/socket.h>
#include<netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <malloc.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>

struct sockaddr_in* createIpv4Address(char *ip, int port);

int createTCPIpv4Socket();

struct AcceptedSocket* acceptIncomingConnection(int serverSocketFD);

void receiveAndPrintIncomingData(int socketFD);

void startAcceptingConnections(int serverSocketFD);

void acceptNewConnectionAndReceiveAndPrint(int serverSocketFD);

void receiveAndPrintIncomingDataOnSeparateThread(struct AcceptedSocket *pSocket);

void sendReceivedMessage(char *buffer, int socketFD);

struct sockaddr_in* createIpv4Address(char *ip, int port) {
    struct sockaddr_in* address = malloc(sizeof(struct sockaddr_in));
    address->sin_family=AF_INET;
    address->sin_port= htons(port);
    if(strlen(ip)==0)
        address->sin_addr.s_addr = INADDR_ANY;
    else
        inet_pton(AF_INET,ip,&address->sin_addr.s_addr);
    return address;
}

int createTCPIpv4Socket() { return socket(AF_INET, SOCK_STREAM, 0); }

struct AcceptedSocket{
    int acceptedSocketFD;
    struct sockaddr_in address;
    int error;
    bool acceptedSuccessfully;
};


struct AcceptedSocket acceptedSockets[10];
int acceptedSocketsCount = 0;


void startAcceptingConnections(int serverSocketFD) {
    while (true){
        struct AcceptedSocket* clientSocket = acceptIncomingConnection(serverSocketFD);
        acceptedSockets[acceptedSocketsCount++] = *clientSocket;
        receiveAndPrintIncomingDataOnSeparateThread(clientSocket);

    }


}



void receiveAndPrintIncomingDataOnSeparateThread(struct AcceptedSocket *pSocket) {
    pthread_t id;
    pthread_create(&id,NULL, receiveAndPrintIncomingData,pSocket->acceptedSocketFD);

}

void receiveAndPrintIncomingData(int socketFD) {
    char buffer[1024];

    while (true){
        size_t amountReceived = recv(socketFD,buffer,1024,0);
        if(amountReceived>0){
            buffer[amountReceived]=0;
            printf("%s\n",buffer);
            sendReceivedMessage(buffer,socketFD);
        }
        if(amountReceived==0)
            break;
    }
    close(socketFD);
}

void sendReceivedMessage(char *buffer, int socketFD) {
    for(int i = 0; i<acceptedSocketsCount;i++){
        if(acceptedSockets[i].acceptedSocketFD!=socketFD){
            send(acceptedSockets[i].acceptedSocketFD,buffer, strlen(buffer),0);
        }
    }
}

struct AcceptedSocket* acceptIncomingConnection(int serverSocketFD) {
    struct sockaddr_in clientAddress;
    int clientAddressSize = sizeof(struct sockaddr_in);
    int clientSocketFD = accept(serverSocketFD,&clientAddress,&clientAddressSize);
    struct AcceptedSocket* acceptedSocket = malloc(sizeof(struct AcceptedSocket));
    acceptedSocket->address = clientAddress;
    acceptedSocket->acceptedSocketFD = clientSocketFD;
    acceptedSocket->acceptedSuccessfully = clientSocketFD>0;
    if (!acceptedSocket->acceptedSuccessfully){
        acceptedSocket->error=clientSocketFD;
    }
    return acceptedSocket;
}


int main() {
    int serverSocketFD = createTCPIpv4Socket();
    struct sockaddr_in *serverAddress = createIpv4Address("",2000);

    int result = bind(serverSocketFD,serverAddress, sizeof(*serverAddress));
    if (result==0)
        printf("Socket was bound successfully\n");

    int listenResult = listen(serverSocketFD,10);

    startAcceptingConnections(serverSocketFD);

    shutdown(serverSocketFD,SHUT_RDWR);
    return 0;
}
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<pthread.h>
#include<unistd.h>
#include<math.h>
#include "chatbookUtility.h"
#include "threadPoolList.h"
#include "serverStructure.h"
#include "readWriteLock.h"



//list to store connected socket file description
struct ThreadPoolList* listClientFd;
struct ThreadPoolList* listLocFd;

//locks for location server thread pool
pthread_mutex_t mutexThreadPool;
pthread_cond_t condThreadPool;

//locks for client thread pool
pthread_mutex_t mutexClientThreadPool;
pthread_cond_t condClientThreadPool;

#define BUFF_WIN_SIZE 1024
#define NUM_OF_CLIENT 4//100
#define NUM_LOC_SERV 1//10
const float LOC_RADUIS = 0.1;
char ip[16];




float distance(float x1, float x2, float y1, float y2) {
    float dis = sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
    return dis;
}

int isPartOf(float x1, float x2, float y1, float y2, float radius) {
    if (((x1 - x2)*(x1 - x2) + (y1 - y2)*(y1 - y2)) <= (radius * radius)) return 1;
    else return 0;
}

int getLoc(float* loc, const char* buffer) {// return number of charater read
    char* location;
    char* endptr;

    int len = extractStr(buffer, &location);

    *loc = strtof(location, &endptr);
    free(location);
    return len;
}






// location server thread pool
void* locServThreads(void* arg) {
    while (1) {
        int connectedSocket;

        pthread_mutex_lock(&mutexThreadPool);
        connectedSocket = popInList(listLocFd);
        if (connectedSocket == -1) {
            pthread_cond_wait(&condThreadPool, &mutexThreadPool);
            connectedSocket = popInList(listLocFd); 
        }
        pthread_mutex_unlock(&mutexThreadPool);

        char buffer[1024];
        int buffRead = 0;
        struct ServerNode* block = createServerNode();

        int buffRecv = recv(connectedSocket, buffer, BUFF_WIN_SIZE, 0);

        printf("buffer receive from location server %d\n", buffRecv);
        for (int i = 0; i<buffRecv; i++) {
            printf("%c ", buffer[i]);
        }
        printf("\n\n");

        buffRead += extractStr(buffer, &(block->ip));
        buffRead += getLoc(&(block->lat), buffer + buffRead);
        buffRead += getLoc(&(block->lon), buffer + buffRead);

        printf("ip of loc, %s\n", block->ip);
        printf("lat of loc, %f\n", block->lat);
        printf("lon of loc, %f\n", block->lon);


        writeLock();
        pushServerNode(block);
        writeUnlock();

        if (buffRead == buffRecv) recv(connectedSocket, buffer, BUFF_WIN_SIZE, 0);

        free(block->ip);
        block->ip = NULL;
        close(connectedSocket);

        printf("location server shutting down\n");
        fflush(stdout);

        writeLock();
        popServerNode(block);
        writeUnlock();

    }

}






// client thread pool
void* clientThreads(void* arg) {
    while (1) {
        int connectedSocket;

        pthread_mutex_lock(&mutexClientThreadPool);
        connectedSocket = popInList(listClientFd);
        if (connectedSocket == -1) {
            pthread_cond_wait(&condClientThreadPool, &mutexClientThreadPool);
            connectedSocket = popInList(listClientFd); 
        }
        pthread_mutex_unlock(&mutexClientThreadPool);

        float lat, lon;
        char buffer[BUFF_WIN_SIZE];
        int buffRead = 0;

        int buffRecv = recv(connectedSocket, buffer, BUFF_WIN_SIZE, 0);
        buffRead += getLoc(&lat, buffer + buffRead);
        buffRead += getLoc(&lon, buffer + buffRead);

        printf("\nlocation asked %f, %f.\n", lat, lon);
        fflush(stdout);

        readLock();
        if (sHead == NULL) {
            buffer[0] = 'n';
            send(connectedSocket, buffer, 1, 0);
            close(connectedSocket);
            return NULL;
        }

        struct ServerNode* curr = sHead;
        float minDis = distance(lat, curr->lat, lon, curr->lon);
        struct ServerNode* minNode = curr;
        curr = curr->next;

        while (curr != NULL) {
            float dis = distance(lat, curr->lat, lon, curr->lon); 
            if (dis < minDis) {
                minDis = dis;
                minNode = curr;
            }
            curr = curr->next;
        }
        readUnlock();

        if(isPartOf(lat, minNode->lat, lon, minNode->lon, LOC_RADUIS)) {
            buffer[0] = 'y';

            for (int i = 0; i<strlen(minNode->ip)+1; i++) {
                buffer[i+1] = (minNode->ip)[i];
            }

            send(connectedSocket, buffer, strlen(minNode->ip) + 2, 0); // sending flag y and ip of location server to client
            printf("y flag and ip address: %s sent\n", minNode->ip);
            fflush(stdout);
        }
        else {
            buffer[0] = 'n';
            send(connectedSocket, buffer, 1, 0); // sending flag n
            printf("n flag sent\n");
            fflush(stdout);
        }

        close(connectedSocket);

    }
}







void* clientServRoutine(void* arg) {
    pthread_mutex_init(&mutexClientThreadPool, NULL);
    pthread_cond_init(&condClientThreadPool, NULL);
    initThreadPoolList(&listClientFd);

    
    pthread_t client_t[NUM_OF_CLIENT];
    for (int i = 0; i<NUM_OF_CLIENT; i++) {
       pthread_create(&client_t[i], NULL, &clientThreads, NULL); 
    }
    printf("\nclient server thread pool created\n\n");

    // openning chatbook server for clients
    int sockFd = inSocketServ(NUM_OF_CLIENT, 8001, ip);
    
    struct sockaddr_in clientAddress;
    int clientAddressSize = sizeof(clientAddress);

    while (1) {

        int connectedSocket = accept(sockFd, (struct sockaddr*) &clientAddress, &clientAddressSize);
        printf("\naccepting a client\n");
        fflush(stdout);

        pthread_mutex_lock(&mutexClientThreadPool);
        pushInList(listClientFd, connectedSocket);
        pthread_mutex_unlock(&mutexClientThreadPool);
        pthread_cond_signal(&condClientThreadPool);       
    }// break condition absent

    destThreadPoolList(listClientFd);
    pthread_mutex_destroy(&mutexClientThreadPool);
    pthread_cond_destroy(&condClientThreadPool);

}





void* locServRoutine(void* arg) {
    initReadWrite();
    pthread_mutex_init(&mutexThreadPool, NULL);
    pthread_cond_init(&condThreadPool, NULL);
    initThreadPoolList(&listLocFd);

    pthread_t serv_t[NUM_LOC_SERV];
    for (int i = 0; i<NUM_LOC_SERV; i++) {
        pthread_create(&serv_t[i], NULL, &locServThreads, NULL);
    }
    printf("\nlocation server thread pool created\n\n");


    // openning chatbook server for location servers
    int sockFd = inSocketServ(NUM_LOC_SERV, 8002, ip);

    struct sockaddr_in clientAddress;
    int clientAddressSize = sizeof(clientAddress);

    while (1) {
        int connectedSocket = accept(sockFd, (struct sockaddr*) &clientAddress, &clientAddressSize);
        printf("\naccepted a location server\n");
        fflush(stdout); 

        pthread_mutex_lock(&mutexThreadPool);
        pushInList(listLocFd, connectedSocket);
        pthread_mutex_unlock(&mutexThreadPool);
        pthread_cond_signal(&condThreadPool);
    }// break condition absent

    destroyReadWrite();
    pthread_mutex_destroy(&mutexThreadPool);
    pthread_cond_destroy(&condThreadPool);
    destThreadPoolList(listLocFd);

}






int main() {
    
    printf("Enter IP address: ");
	scanf("%15s", ip);

    pthread_t client_t;
    pthread_create(&client_t, NULL, &clientServRoutine, NULL);
    pthread_t serv_t;
    pthread_create(&serv_t, NULL, &locServRoutine, NULL);

    pthread_join(client_t, NULL);
    pthread_join(serv_t, NULL);

    return 0;
}

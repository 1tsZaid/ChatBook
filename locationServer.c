#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<pthread.h>
#include<unistd.h>
#include<semaphore.h>
#include "chatbookUtility.h"
#include "threadPoolList.h"






#define BUFF_WIN_SIZE 1024
#define NUM_OF_CLIENT 5 //50

//list to store connected socket file description
struct ThreadPoolList* listFd;

// locks for thread pool
pthread_mutex_t mutexThreadPool;
pthread_cond_t condThreadPool;

pthread_mutex_t mutexMsg;// mutex for message transmission
pthread_mutex_t mutexVar;// mutex for sending number variable

sem_t* semConsumer[NUM_OF_CLIENT];
sem_t semProducer;

char* data;
int dataSize;
int socketName;
int sendingNum;







char* msgGenerate(const char* name, const char* buffer, int read) {
    int lenName = strlen(name);
    char* recvData = (char*) malloc(lenName + read + 2);
    
    for (int i = 0; i<lenName; i++) {
        recvData[i] = name[i];
    }

    recvData[lenName] = ':';
    recvData[lenName+1] = ' ';

    for (int i = 0; i<read; i++) {
        recvData[i + lenName + 2] = buffer[i];
    }

    return recvData;
}

int isQuit() {
    int i;
    for (i = 0; i<dataSize; i++) {
        if (data[i] == ':') break;
    }
    
    if (data[i+2] == 'q' && data[i+3] == '\n') return 1;
    else return 0;
}






// thread for sending back messages
void* sendingThread(void* arg) {
    int connectedSocket = *(int*) arg;
    int consIndex;

    pthread_mutex_lock(&mutexVar);
    for (int i = 0; i<NUM_OF_CLIENT; i++) {
        if (semConsumer[i] == NULL) {
            consIndex = i;
            semConsumer[consIndex] = malloc(sizeof(sem_t));
            break;
        }
    }
    sem_init(semConsumer[consIndex], 0, 1);
    sem_wait(semConsumer[consIndex]);
    pthread_mutex_unlock(&mutexVar); 


    while (1) {
        sem_wait(semConsumer[consIndex]);
        int flag = 0;
        if (socketName != connectedSocket) send(connectedSocket, data, dataSize, 0);
        else flag = isQuit();

        pthread_mutex_lock(&mutexVar);
        sendingNum--;
        
        if (sendingNum == 0) sem_post(&semProducer); 
        pthread_mutex_unlock(&mutexVar);

        if (flag) break;
    }

    sem_destroy(semConsumer[consIndex]);
    free(semConsumer[consIndex]);

    pthread_mutex_lock(&mutexVar); 
    semConsumer[consIndex] = NULL;
    pthread_mutex_unlock(&mutexVar);  
}

// client thread pool
void* clientThreads(void* arg) {
    while (1) {

        int connectedSocket;

        pthread_mutex_lock(&mutexThreadPool);
        connectedSocket = popInList(listFd);
        if (connectedSocket == -1) {
            pthread_cond_wait(&condThreadPool, &mutexThreadPool);
            connectedSocket = popInList(listFd); 
        }
        pthread_mutex_unlock(&mutexThreadPool);
        

        char buffer[BUFF_WIN_SIZE];
        int buffRecv = recv(connectedSocket, buffer, BUFF_WIN_SIZE, 0); // receive name information from the client


        // saving name information
        char* name = malloc(buffRecv * sizeof(char));
        int namePreLine = strlen(name) + 2;
        strcpy(name, buffer);

        buffer[0] = 'y';
        send(connectedSocket, buffer, 1, 0);
        
        // creating a thread to send back messages
        pthread_t transfer;
        pthread_create(&transfer, NULL, &sendingThread, (void*) &connectedSocket);


        // logic for receiving messages of a single client per thread
        while (1) {

            buffRecv = recv(connectedSocket, buffer, BUFF_WIN_SIZE, 0); // receive chat message from the client

            if (buffRecv == -1 || buffRecv == 0) {
                buffer[0] = 'q';
                buffer[1] = '\n';
                buffRecv = 2;
            }

            // generating data for the group chat
            char* recvData = msgGenerate(name, buffer, buffRecv);
            for (int i = 0; i<buffRecv + namePreLine; i++) {
                printf("%c", recvData[i]);
            }           

            // =========================== sending message to other group members =========================
            pthread_mutex_lock(&mutexMsg);
            data = recvData;
            dataSize = buffRecv + namePreLine;
            socketName = connectedSocket;

            pthread_mutex_lock(&mutexVar);
            sendingNum = 0;
            for (int i = 0; i<NUM_OF_CLIENT; i++) {
                if (semConsumer[i] != NULL) {
                    sendingNum++;
                    sem_post(semConsumer[i]);
                }
            }
            pthread_mutex_unlock(&mutexVar);

            sem_wait(&semProducer);
            pthread_mutex_unlock(&mutexMsg);
            // ============================= group members received the message ===========================

            free(recvData);
            if (buffer[0] == 'q' && buffer[1] == '\n') break;

            buffer[0] = 'y';
            buffer[1] = '\0';
            send(connectedSocket, buffer, 2, 0); 

        }

        free(name);
        close(connectedSocket);

    }

}






int main(int argc, char** argv) {
    if (argc < 3) {
		printf("location information is missing!\n");
		exit(0);
	}

	printf("Enter IP address: ");
	char ip[16];
	scanf("%15s", ip);




    // =============================== connecting to chatbook server ===================================
    int servSockFd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in address = inSocketAddress(8002, "127.0.0.1");

    int result = connect(servSockFd, (struct sockaddr*) &address, sizeof(address));
	if (result == 0) printf("connection was successful with chatbook server\n");
	else {
		printf("connection was unsuccessful\n");
		exit(0);
	}

    // sending ip address, latitude, longitude information to the chatbook server
    int len1 = strlen(ip) + 1;
    int len2 = strlen(argv[1]) + 1;
    char* msgtemp = stringStream(ip, len1, argv[1], len2);

    len1 = len1 + len2;
    len2 = strlen(argv[2]) + 1;
    char* msg = stringStream(msgtemp, len1, argv[2], len2);
    free(msgtemp);

    printf("\n");
    for(int i = 0; i<len1+len2; i++) {
        printf("%c", msg[i]);
    }
    printf("\n");

    send(servSockFd, msg, len1+len2, 0);
    free(msg);




    // ================== openning location server =========================
    int sockFd = inSocketServ(NUM_OF_CLIENT, 8003, ip);

	struct sockaddr_in clientAddress;
	int clientAddressSize = sizeof(clientAddress);

    initThreadPoolList(&listFd);
    
    pthread_mutex_init(&mutexThreadPool, NULL);
    pthread_cond_init(&condThreadPool, NULL);

    pthread_mutex_init(&mutexMsg, NULL);
    pthread_mutex_init(&mutexVar, NULL);

    sem_init(&semProducer, 0, 1);
    sem_wait(&semProducer);

    pthread_t client_t[NUM_OF_CLIENT];
    for (int i = 0; i<NUM_OF_CLIENT; i++) {
        semConsumer[i] = NULL;
        pthread_create(&client_t[i], NULL, &clientThreads, NULL);
    }
    printf("\nclient thread pool created\n");


    while (1) {
        int connectedSocket = accept(sockFd, (struct sockaddr*) &clientAddress, &clientAddressSize);
        printf("\naccepted a client\n\n");
        fflush(stdout);
        
        pthread_mutex_lock(&mutexThreadPool);
        pushInList(listFd, connectedSocket);
        pthread_mutex_unlock(&mutexThreadPool);
        pthread_cond_signal(&condThreadPool);

    } // break condition absent

    sem_destroy(&semProducer);

    pthread_mutex_destroy(&mutexVar);
    pthread_mutex_destroy(&mutexMsg);

    pthread_mutex_destroy(&mutexThreadPool);
    pthread_cond_destroy(&condThreadPool);

    destThreadPoolList(listFd);
	
    return 0;
}

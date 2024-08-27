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



#define BUFF_WIN_SIZE 1024


sem_t monitor;



// group chat messages receiving thread
void* recRoutine(void* arg) {
	int connectedSocket = *(int*) arg;
	char buffer[BUFF_WIN_SIZE];

	while (1) {
		// printing chat messages
		int buffRecv = recv(connectedSocket, buffer, BUFF_WIN_SIZE, 0);
		for (int i = 0; i<buffRecv; i++) {
			if (buffer[i] == 'y' && buffer[i+1] == '\0') { // acknowledgment => y\0
				sem_post(&monitor);
				i++;
				continue;
			}
			printf("%c", buffer[i]);
		}

	}
}




int main(int argc, char** argv) {
	if (argc < 3) {
		printf("Incomplete location information!\n");
		exit(0);
	}

	printf("Enter your name: ");
	char name[30];
	scanf("%29s", name);

	sem_init(&monitor, 0, 1);




	// =================== connecting to chatbook server ====================
	int sockFd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in address = inSocketAddress(8001, "127.0.0.1");

	int result = connect(sockFd, (struct sockaddr*) &address, sizeof(address));
	if (result == 0) printf("connection was successful with chatbook server\n");
	else {
		printf("connection was unsuccessful\n");
		exit(0);
	}

	//send lat, lon to chatbook server
	int len1 = strlen(argv[1]) + 1;
    int len2 = strlen(argv[2]) + 1;
    char* msg = stringStream(argv[1], len1, argv[2], len2);

	send(sockFd, msg, len1+len2, 0);
	free(msg);


	//receive flags y or n 
	char buffer[BUFF_WIN_SIZE];
	recv(sockFd, buffer, BUFF_WIN_SIZE, 0);

	if (buffer[0] == 'n') {
		printf("no location server available!\n");
		close(sockFd);
		exit(0);
	}
	
	char* ip;
	extractStr(buffer+1, &ip);

	printf("location server ip address is %s \n", ip);
	fflush(stdout);

	close(sockFd);
	// ======================= connection end with chatbook server =====================




	// ========================== connecting to location server =========================
	sockFd = socket(AF_INET, SOCK_STREAM, 0);
	address = inSocketAddress(8003, ip);

	result = connect(sockFd, (struct sockaddr*) &address, sizeof(address));
	if (result == 0) printf("connection was successful with location server\n");
	else {
		printf("connection was unsuccessful\n");
		exit(0);
	}
	free(ip);

	send(sockFd, name, strlen(name)+1, 0);
	recv(sockFd, buffer, 1, 0);
	getchar();	




	// ============================== start group chat ===============================
	printf("\nLocation group chat joined\n\n");
	sem_wait(&monitor);
	strcpy(buffer, "joined\n");
	send(sockFd, buffer, 7, 0);

	pthread_t rec; 
	pthread_create(&rec, NULL, &recRoutine, (void*) &sockFd); // thead for receiving group chat msg from location server

	// sending message to location server
	char* line;
	while (1) {
		line = NULL;
		size_t len = 0;

		size_t read = getline(&line, &len, stdin);
		if (read == -1) {  // Check for error in getline
            perror("getline");
            break;
        }
		else if (read > BUFF_WIN_SIZE) {
			printf("message size limit exceed\n");
			continue;
		}

		// Check if the input is "q\n"
		if (line[0] == 'q' && line[1] == '\n') {
			send(sockFd, line, 2, 0); //chatting ends with q
			free(line);
			break;
		}

		send(sockFd, line, read, 0);
		sem_wait(&monitor);

		free(line);

	}
	close(sockFd);

	sem_destroy(&monitor);

	return 0;
}

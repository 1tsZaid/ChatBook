#ifndef C_B_UTILITY_H
#define C_B_UTILITY_H

struct sockaddr_in inSocketAddress(short port,const char* ip) {

	struct sockaddr_in address;
	address.sin_port = htons(port);
	address.sin_family = AF_INET;

    if (ip == NULL) {
        address.sin_addr.s_addr = INADDR_ANY;
    }
    else {

        if (inet_pton(AF_INET, ip, &address.sin_addr.s_addr)<=0) {
            printf("Invalid address. Address not supported\n");
            exit(0);
        }

    }

	return address;
}

int inSocketServ(int listenSize, int port, const char* ip) {
    int sockFd = socket(AF_INET, SOCK_STREAM, 0); 

    struct sockaddr_in address = inSocketAddress(port, ip);
    printf("ip address for socket, %s\n", ip);

    int result = bind(sockFd, (struct sockaddr*) &address, sizeof(address));
    if (result == 0) printf("socket binded successfully on port: %d\n", port);

    int listenRes = listen(sockFd, listenSize);
    if (listenRes == 0) printf("socket listening successfully on port: %d\n", port);
    return sockFd;
}






int extractStr(const char* buffer, char** ptrStr) { // returns the number of charater read (with null char)
    int len = strlen(buffer) + 1;
    *ptrStr = malloc(len * sizeof(char));

    for (int i = 0; i<len; i++) {
        (*ptrStr)[i] = buffer[i];
    }
    return len;
}

int isNullChar(const char* buffer, int size) {
    for (int i = 0; i<size; i++) {
        if(buffer[i] == '\0') return 1;
    }
    
    return 0;
}

char* stringStream(const char* s1, int len1, const char* s2, int len2) { // len1 and len2 is the number of indexes in s1 and s2 (with null char)
    char* str;
    int len = len1 + len2;
    
    str = (char*) malloc(len * sizeof(char));
    int index = 0;
    int i;

    for (i = 0; i< len1; i++) {
        str[i+index] = s1[i];
    }
    index = i;
    
    for (i = 0; i< len2; i++) {
        str[i+index] = s2[i];
    }

    return str;
}

#endif
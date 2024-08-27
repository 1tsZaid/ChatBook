#ifndef READ_WRITE
#define READ_WRITE
#include <semaphore.h>

pthread_mutex_t lock;

pthread_mutex_t readVarMutex;
int readCount = 0;

pthread_mutex_t writeVarMutex;
int writeCount = 0;

sem_t semaphore;

void initReadWrite() {
    pthread_mutex_init(&readVarMutex, NULL);
    pthread_mutex_init(&writeVarMutex, NULL);
    pthread_mutex_init(&lock, NULL);
    sem_init(&semaphore, 0, 1);
}

void destroyReadWrite() {
    pthread_mutex_destroy(&readVarMutex);
    pthread_mutex_destroy(&writeVarMutex);
    pthread_mutex_destroy(&lock);
    sem_destroy(&semaphore);
}

void readLock() {
    while (writeCount > 0) {
        sem_wait(&semaphore);
    }
    if (readCount == 0) pthread_mutex_lock(&lock);

    pthread_mutex_lock(&readVarMutex);
    readCount++;
    pthread_mutex_unlock(&readVarMutex);
}

void readUnlock() {
    pthread_mutex_lock(&readVarMutex);
    readCount--;
    pthread_mutex_unlock(&readVarMutex);

    if (readCount == 0) pthread_mutex_unlock(&lock);
}

void writeLock() {
    pthread_mutex_lock(&writeVarMutex);
    writeCount++;
    pthread_mutex_unlock(&writeVarMutex);
    pthread_mutex_lock(&lock);
}

void writeUnlock() {
    pthread_mutex_unlock(&lock);
    pthread_mutex_lock(&writeVarMutex);
    writeCount--;
    pthread_mutex_unlock(&writeVarMutex);
    sem_post(&semaphore);
}

#endif
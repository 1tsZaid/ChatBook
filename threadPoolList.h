#ifndef THREAD_POOL_LIST
#define THREAD_POOL_LIST

struct NodeFd {
    int fd;
    struct NodeFd* next;
};

struct ThreadPoolList {
    struct NodeFd* headFd;
    struct NodeFd* tailFd;
};

void initThreadPoolList(struct ThreadPoolList** listAddr) {
    *listAddr = malloc(sizeof(struct ThreadPoolList));
    (*listAddr)->headFd = NULL;
    (*listAddr)->tailFd = NULL;
}

void destThreadPoolList(struct ThreadPoolList* list) {
    free(list);
}

struct NodeFd* createNodeFd(int fd) {
    struct NodeFd* block = malloc( sizeof(struct NodeFd) );
    block->fd = fd;
    block->next = NULL;
    return block;
}

void pushInList(struct ThreadPoolList* list, int fd) {
    if (list->headFd == NULL) {
        list->headFd = list->tailFd = createNodeFd(fd);
    }
    else  {
        list->tailFd->next = createNodeFd(fd);
        list->tailFd = list->tailFd->next;
    }
}

int popInList(struct ThreadPoolList* list) {
    int fd;
    if (list->headFd == NULL) return -1;
    else if (list->headFd == list->tailFd) {
        fd = list->headFd->fd;
        free(list->headFd);
        list->headFd = list->tailFd = NULL;
    }
    else {
        struct NodeFd* block = list->headFd;
        fd = block->fd;
        list->headFd = list->headFd->next;
        free(block);
    }
    return fd; 
}

#endif